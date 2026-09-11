#include "mino/network/ssh/ssh_client.hpp"
#include <algorithm>
#include <sstream>

namespace mino::network::ssh2 {

    ssh_client::ssh_client() {
        platform_network_initializer::ensure_initialized();
    }

    ssh_client::~ssh_client() {
        stop_auto_reconnect();
        disconnect_ssh();
    }

    void ssh_client::set_on_connect(connect_callback cb) { on_ssh_connect_ = std::move(cb); }
    void ssh_client::set_on_disconnect(disconnect_callback cb) { on_ssh_disconnect_ = std::move(cb); }
    void ssh_client::set_on_error(error_callback cb) { on_ssh_error_ = std::move(cb); }
    void ssh_client::set_on_receive(receive_callback cb) { on_ssh_receive_ = std::move(cb); }

    void ssh_client::set_credentials(const ssh_credentials& creds) {
        credentials_ = creds;
    }

    void ssh_client::set_expected_fingerprint(const std::string& fingerprint, fingerprint_type type) {
        expected_fingerprint_ = fingerprint;
        expected_fingerprint_type_ = type;
    }

    void ssh_client::notify_error(int code, const std::string& message) {
        if (logger) logger->error("[ssh_client] Error ({}): {}", code, message);
        if (on_ssh_error_) {
            try { on_ssh_error_(code, message); }
            catch (...) {}
        }
    }

    void ssh_client::notify_disconnect(const std::string& reason) {
        if (logger) logger->warn("[ssh_client] Disconnected: {}", reason);
        if (on_ssh_disconnect_) {
            try { on_ssh_disconnect_(reason); }
            catch (...) {}
        }
    }

    std::string ssh_client::get_server_fingerprint_hex(fingerprint_type type) const {
        if (!session_) return {};
        int libssh2_type = (type == fingerprint_type::md5) ? LIBSSH2_HOSTKEY_HASH_MD5 :
            (type == fingerprint_type::sha1) ? LIBSSH2_HOSTKEY_HASH_SHA1 :
            LIBSSH2_HOSTKEY_HASH_SHA256;
        size_t len = (type == fingerprint_type::md5) ? 16 : (type == fingerprint_type::sha1) ? 20 : 32;
        const char* hash = libssh2_hostkey_hash(session_, libssh2_type);
        return hash ? bytes_to_hex(reinterpret_cast<const unsigned char*>(hash), len) : "";
    }

    std::string ssh_client::get_server_fingerprint_base64(fingerprint_type type) const {
        if (!session_) return {};
        int libssh2_type = (type == fingerprint_type::md5) ? LIBSSH2_HOSTKEY_HASH_MD5 :
            (type == fingerprint_type::sha1) ? LIBSSH2_HOSTKEY_HASH_SHA1 :
            LIBSSH2_HOSTKEY_HASH_SHA256;
        size_t len = (type == fingerprint_type::md5) ? 16 : (type == fingerprint_type::sha1) ? 20 : 32;
        const char* hash = libssh2_hostkey_hash(session_, libssh2_type);
        return hash ? bytes_to_base64(reinterpret_cast<const unsigned char*>(hash), len) : "";
    }

    bool ssh_client::verify_fingerprint() {
        if (expected_fingerprint_.empty()) return true;

        std::string actual_hex = get_server_fingerprint_hex(expected_fingerprint_type_);
        std::string actual_b64 = get_server_fingerprint_base64(expected_fingerprint_type_);

        std::string clean_expected = expected_fingerprint_;
        if (clean_expected.rfind("SHA256:", 0) == 0) {
            clean_expected = clean_expected.substr(7);
        }

        auto to_lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
            };

        if (actual_b64 != clean_expected && to_lower(actual_hex) != to_lower(clean_expected)) {
            notify_error(-10, "Host key fingerprint mismatch (expected: " + clean_expected + ", actual: " + actual_b64 + ")");
            return false;
        }
        return true;
    }

    bool ssh_client::perform_handshake() {
        int rc = libssh2_session_handshake(session_, static_cast<libssh2_socket_t>(socket_fd));
        if (rc != 0) {
            char* err_msg = nullptr;
            libssh2_session_last_error(session_, &err_msg, nullptr, 0);
            notify_error(rc, err_msg ? err_msg : "Handshake failed");
            return false;
        }
        return true;
    }

    bool ssh_client::authenticate() {
        int rc = 0;
        if (credentials_.type == auth_type::password) {
            rc = libssh2_userauth_password(session_, credentials_.username.c_str(), credentials_.password.c_str());
        }
        else {
            rc = libssh2_userauth_publickey_fromfile(
                session_, credentials_.username.c_str(),
                credentials_.public_key_path.empty() ? nullptr : credentials_.public_key_path.c_str(),
                credentials_.private_key_path.c_str(), credentials_.passphrase.c_str());
        }

        if (rc != 0) {
            char* err_msg = nullptr;
            libssh2_session_last_error(session_, &err_msg, nullptr, 0);
            notify_error(rc, err_msg ? err_msg : "Authentication failed");
            return false;
        }
        return true;
    }

    bool ssh_client::open_channel_internal() {
        channel_ = libssh2_channel_open_session(session_);
        if (!channel_) {
            notify_error(-20, "Failed to open SSH channel");
            return false;
        }
        return true;
    }

    bool ssh_client::connect_ssh_internal() {
        std::lock_guard<std::mutex> lock(ssh_mutex_);
        cleanup_ssh_session();

        socket_fd = ::socket(address_family, SOCK_STREAM, 0);
#ifdef _WIN32
        if (socket_fd == INVALID_SOCKET) {
            notify_error(WSAGetLastError(), "Socket creation failed");
            return false;
        }
#else
        if (socket_fd < 0) {
            notify_error(errno, "Socket creation failed");
            return false;
        }
#endif

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

        if (::connect(socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
#ifdef _WIN32
            notify_error(WSAGetLastError(), "TCP connect() failed");
#else
            notify_error(errno, "TCP connect() failed");
#endif
            close_connection();
            return false;
        }
        is_connected_flag = true;

        session_ = libssh2_session_init();
        if (!session_) {
            notify_error(-1, "Failed to initialize libssh2 session");
            close_connection();
            return false;
        }

        libssh2_session_set_blocking(session_, 1);

        if (!perform_handshake() || !verify_fingerprint() || !authenticate() || !open_channel_internal()) {
            cleanup_ssh_session();
            return false;
        }

        recv_stream_buffer_.clear();
        if (logger) logger->info("[ssh_client] Connected and authenticated successfully");
        if (on_ssh_connect_) {
            try { on_ssh_connect_(); }
            catch (...) {}
        }
        return true;
    }

    bool ssh_client::connect_ssh(const std::string& host, unsigned short port) {
        set_server(host, port);
        return connect_ssh_internal();
    }

    bool ssh_client::start_auto_reconnect(const std::string& host,
        unsigned short port,
        const reconnect_config& config) {
        set_server(host, port);
        reconnect_cfg_ = config;
        auto_reconnect_enabled_ = true;

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        worker_thread_ = std::thread(&ssh_client::worker_loop, this);
        return true;
    }

    void ssh_client::stop_auto_reconnect() {
        auto_reconnect_enabled_ = false;
        reconnect_cv_.notify_all();
        disconnect_ssh();

        if (worker_thread_.joinable()) {
            if (worker_thread_.get_id() != std::this_thread::get_id()) {
                worker_thread_.join();
            }
            else {
                worker_thread_.detach();
            }
        }
    }

    void ssh_client::worker_loop() {
        char chunk[1024];
        std::chrono::milliseconds current_interval = reconnect_cfg_.initial_interval;
        std::optional<std::chrono::steady_clock::time_point> reconnect_start_time = std::nullopt;

        while (auto_reconnect_enabled_) {
            if (!is_connected() || !session_ || !channel_) {
                auto now = std::chrono::steady_clock::now();

                if (!reconnect_start_time.has_value()) {
                    reconnect_start_time = now;
                }

                if (reconnect_cfg_.max_duration.has_value()) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *reconnect_start_time);
                    if (elapsed >= *reconnect_cfg_.max_duration) {
                        notify_error(-99, "Auto-reconnect timed out: exceeded max duration (" +
                            std::to_string(reconnect_cfg_.max_duration->count()) + " ms)");
                        auto_reconnect_enabled_ = false;
                        break;
                    }
                }

                if (logger) {
                    logger->info("[ssh_client] Attempting connection to {}:{} (Backoff interval: {} ms)...",
                        server_ip, server_port, current_interval.count());
                }

                if (connect_ssh_internal()) {
                    current_interval = reconnect_cfg_.initial_interval;
                    reconnect_start_time.reset();
                }
                else {
                    {
                        std::unique_lock<std::mutex> lock(reconnect_cv_mutex_);
                        reconnect_cv_.wait_for(lock, current_interval, [this]() {
                            return !auto_reconnect_enabled_;
                            });
                    }

                    if (!auto_reconnect_enabled_) break;

                    auto next_val = static_cast<long long>(current_interval.count() * reconnect_cfg_.backoff_multiplier);
                    current_interval = std::min(std::chrono::milliseconds(next_val), reconnect_cfg_.max_interval);
                    continue;
                }
            }

            ssize_t bytes_read = 0;
            {
                std::lock_guard<std::mutex> lock(ssh_mutex_);
                if (channel_) {
                    libssh2_session_set_timeout(session_, 200); // 200ms 타임아웃
                    bytes_read = libssh2_channel_read(channel_, chunk, sizeof(chunk));
                }
            }

            if (bytes_read > 0) {
                recv_stream_buffer_.append(chunk, static_cast<size_t>(bytes_read));
                size_t pos = 0;
                while ((pos = recv_stream_buffer_.find('\n')) != std::string::npos) {
                    std::string msg = recv_stream_buffer_.substr(0, pos);
                    recv_stream_buffer_.erase(0, pos + 1);
                    if (!msg.empty() && msg.back() == '\r') msg.pop_back();

                    if (on_ssh_receive_) {
                        try { on_ssh_receive_(msg); }
                        catch (...) {}
                    }
                }
            }
            else if (bytes_read == LIBSSH2_ERROR_EAGAIN) {
                continue;
            }
            else {
                notify_disconnect("Connection closed by peer or network error");
                {
                    std::lock_guard<std::mutex> lock(ssh_mutex_);
                    cleanup_ssh_session();
                }

                reconnect_start_time = std::chrono::steady_clock::now();
                current_interval = reconnect_cfg_.initial_interval;
            }
        }
    }

    bool ssh_client::send_json(const std::string& json_payload) {
        std::lock_guard<std::mutex> lock(ssh_mutex_);
        if (!channel_) {
            notify_error(-30, "Cannot send data: SSH channel is not open");
            return false;
        }

        std::string framed = json_payload;
        if (framed.empty() || framed.back() != '\n') {
            framed.push_back('\n');
        }

        size_t total_sent = 0;
        while (total_sent < framed.size()) {
            ssize_t sent = libssh2_channel_write(channel_, framed.c_str() + total_sent, framed.size() - total_sent);
            if (sent < 0) {
                notify_error(static_cast<int>(sent), "libssh2_channel_write failed");
                return false;
            }
            total_sent += static_cast<size_t>(sent);
        }
        return true;
    }

    std::optional<std::string> ssh_client::execute_command(const std::string& command) {
        std::lock_guard<std::mutex> lock(ssh_mutex_);
        if (!session_) return std::nullopt;

        LIBSSH2_CHANNEL* cmd_ch = libssh2_channel_open_session(session_);
        if (!cmd_ch) return std::nullopt;

        if (libssh2_channel_exec(cmd_ch, command.c_str()) != 0) {
            libssh2_channel_free(cmd_ch);
            return std::nullopt;
        }

        std::ostringstream oss;
        char buf[1024];
        ssize_t n = 0;
        while ((n = libssh2_channel_read(cmd_ch, buf, sizeof(buf))) > 0) {
            oss.write(buf, n);
        }

        libssh2_channel_close(cmd_ch);
        libssh2_channel_free(cmd_ch);
        return oss.str();
    }

    void ssh_client::cleanup_ssh_session() {
        if (channel_) {
            libssh2_channel_close(channel_);
            libssh2_channel_free(channel_);
            channel_ = nullptr;
        }
        if (session_) {
            libssh2_session_disconnect(session_, "Client shutdown");
            libssh2_session_free(session_);
            session_ = nullptr;
        }
        close_connection();
    }

    void ssh_client::disconnect_ssh() {
        std::lock_guard<std::mutex> lock(ssh_mutex_);
        cleanup_ssh_session();
    }

} // namespace mino::network::ssh
