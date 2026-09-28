#include "mino/network_openssl/ftps/ftps_client.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <algorithm>
#include <cctype>

namespace mino::network_openssl::ftps {

    namespace {
        bool is_ip_address(const std::string& host) {
            if (host.empty()) return false;
            return std::all_of(host.begin(), host.end(), [](char c) {
                return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ':';
                });
        }

        bool wait_socket_readable(socket_t fd, int timeout_ms) {
            if (fd
#ifdef _WIN32
                == INVALID_SOCKET
#else
                < 0
#endif
                ) return false;

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);

            timeval tv{};
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int ret = ::select(static_cast<int>(fd) + 1, &read_fds, nullptr, nullptr, &tv);
            return (ret > 0 && FD_ISSET(fd, &read_fds));
        }
    }

    ftps_client::ftps_client() = default;

    ftps_client::~ftps_client() {
        close_control_socket();
        cleanup_ssl();
    }

    void ftps_client::set_tls_verification(bool verify, const std::string& ca_file, const std::string& ca_path) {
        verify_peer_ = verify;
        ca_file_ = ca_file;
        ca_path_ = ca_path;
    }

    void ftps_client::set_data_protection_mode(ftps_data_protection mode) {
        data_prot_ = mode;
    }

    bool ftps_client::init_ssl_ctx() {
        if (ssl_ctx_) return true;

        const SSL_METHOD* method = TLS_client_method();
        ssl_ctx_ = SSL_CTX_new(method);
        if (!ssl_ctx_) {
            last_error = "Failed to create SSL_CTX.";
            return false;
        }

        SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);
        SSL_CTX_set_session_cache_mode(ssl_ctx_, SSL_SESS_CACHE_CLIENT);

        if (verify_peer_) {
            SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
            if (!ca_file_.empty() || !ca_path_.empty()) {
                const char* file = ca_file_.empty() ? nullptr : ca_file_.c_str();
                const char* path = ca_path_.empty() ? nullptr : ca_path_.c_str();
                SSL_CTX_load_verify_locations(ssl_ctx_, file, path);
            }
            else {
                SSL_CTX_set_default_verify_paths(ssl_ctx_);
            }
        }
        else {
            SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr);
        }

        return true;
    }

    void ftps_client::cleanup_ssl() {
        if (ssl_ctx_) {
            SSL_CTX_free(ssl_ctx_);
            ssl_ctx_ = nullptr;
        }
    }

    void ftps_client::close_control_socket() {
        stop_flag_ = true;
        is_connected_ = false;

        {
            std::lock_guard<std::mutex> lock(ssl_mutex_);
            if (ssl_control_) {
                SSL_shutdown(ssl_control_);
                SSL_free(ssl_control_);
                ssl_control_ = nullptr;
            }
#ifdef _WIN32
            if (control_socket_ != INVALID_SOCKET) {
                ::shutdown(control_socket_, SD_BOTH);
                closesocket(control_socket_);
                control_socket_ = INVALID_SOCKET;
            }
#else
            if (control_socket_ >= 0) {
                ::shutdown(control_socket_, SHUT_RDWR);
                close(control_socket_);
                control_socket_ = -1;
            }
#endif
        }

        if (receive_thread_.joinable()) {
            if (receive_thread_.get_id() != std::this_thread::get_id()) {
                receive_thread_.join();
            }
            else {
                receive_thread_.detach();
            }
        }
    }

    std::string ftps_client::read_plain_response(std::chrono::seconds timeout) {
        auto end_time = std::chrono::steady_clock::now() + timeout;
        std::string buffer;
        char chunk[1024];

        while (std::chrono::steady_clock::now() < end_time) {
            if (!wait_socket_readable(control_socket_, 100)) {
                continue;
            }

#ifdef _WIN32
            int bytes = ::recv(control_socket_, chunk, sizeof(chunk), 0);
#else
            ssize_t bytes = ::recv(control_socket_, chunk, sizeof(chunk), 0);
#endif
            if (bytes <= 0) break;

            buffer.append(chunk, bytes);
            if (buffer.size() >= 4) {
                size_t last_line = buffer.rfind("\r\n", buffer.size() - 3);
                size_t check_pos = (last_line == std::string::npos) ? 0 : last_line + 2;
                if (buffer.size() >= check_pos + 4) {
                    if (std::isdigit(buffer[check_pos]) &&
                        std::isdigit(buffer[check_pos + 1]) &&
                        std::isdigit(buffer[check_pos + 2]) &&
                        buffer[check_pos + 3] == ' ') {
                        return buffer;
                    }
                }
            }
        }
        return buffer;
    }

    bool ftps_client::send_plain_command(const std::string& cmd, const std::string& arg) {
        std::string full_cmd = cmd + (arg.empty() ? "" : " " + arg) + "\r\n";
#ifdef _WIN32
        int ret = ::send(control_socket_, full_cmd.data(), static_cast<int>(full_cmd.size()), 0);
#else
        ssize_t ret = ::send(control_socket_, full_cmd.data(), full_cmd.size(), 0);
#endif
        return ret > 0;
    }

    void ftps_client::receive_loop() {
        char buffer[16384];

        while (!stop_flag_ && is_connected_) {
            bool has_pending = false;
            {
                std::lock_guard<std::mutex> lock(ssl_mutex_);
                if (ssl_control_) {
                    has_pending = (SSL_pending(ssl_control_) > 0);
                }
            }

            if (!has_pending && !wait_socket_readable(control_socket_, 50)) {
                continue;
            }

            int bytes = 0;
            {
                std::lock_guard<std::mutex> lock(ssl_mutex_);
                if (!ssl_control_ || !is_connected_) break;
                bytes = SSL_read(ssl_control_, buffer, sizeof(buffer) - 1);
            }

            if (bytes > 0) {
                std::lock_guard<std::mutex> lock(control_mutex_);
                control_buffer_.append(buffer, bytes);
                if (control_buffer_.size() >= 4) {
                    size_t last_line = control_buffer_.rfind("\r\n", control_buffer_.size() - 3);
                    size_t check_pos = (last_line == std::string::npos) ? 0 : last_line + 2;
                    if (control_buffer_.size() >= check_pos + 4) {
                        if (std::isdigit(control_buffer_[check_pos]) &&
                            std::isdigit(control_buffer_[check_pos + 1]) &&
                            std::isdigit(control_buffer_[check_pos + 2]) &&
                            control_buffer_[check_pos + 3] == ' ') {
                            has_control_response_ = true;
                            control_cv_.notify_one();
                        }
                    }
                }
            }
            else {
                int err = 0;
                {
                    std::lock_guard<std::mutex> lock(ssl_mutex_);
                    if (ssl_control_) err = SSL_get_error(ssl_control_, bytes);
                }
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                break;
            }
        }
        is_connected_ = false;
    }

    std::string ftps_client::read_control_response(std::chrono::seconds timeout) {
        std::unique_lock<std::mutex> lock(control_mutex_);
        bool success = control_cv_.wait_for(lock, timeout, [this] { return has_control_response_; });

        if (!success) {
            last_error = "TLS Control channel response timeout.";
            return "";
        }

        std::string res = control_buffer_;
        control_buffer_.clear();
        has_control_response_ = false;
        return res;
    }

    bool ftps_client::send_command(const std::string& cmd, const std::string& arg) {
        if (!is_connected_) {
            last_error = "TLS Control channel is not connected.";
            return false;
        }

        std::string full_cmd = cmd + (arg.empty() ? "" : " " + arg) + "\r\n";
        {
            std::lock_guard<std::mutex> lock(control_mutex_);
            has_control_response_ = false;
        }

        std::lock_guard<std::mutex> lock(ssl_mutex_);
        if (!ssl_control_ || SSL_write(ssl_control_, full_cmd.data(), static_cast<int>(full_cmd.size())) <= 0) {
            last_error = "Failed to send command over TLS: " + cmd;
            return false;
        }
        return true;
    }

    std::int64_t ftps_client::get_remote_file_size(const std::string& remote_file) {
        if (!send_command("SIZE", remote_file)) return 0;
        std::string resp = read_control_response();
        if (resp.rfind("213", 0) == 0) {
            try {
                return std::stoll(resp.substr(4));
            }
            catch (...) { return 0; }
        }
        return 0;
    }

    std::unique_ptr<tls_data_connection> ftps_client::establish_tls_data_connection() {
        if (!send_command("PASV")) return nullptr;
        std::string resp = read_control_response();

        std::regex pasv_regex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
        std::smatch match;
        if (!std::regex_search(resp, match, pasv_regex) || match.size() < 7) {
            last_error = "Failed to parse PASV response: " + resp;
            return nullptr;
        }

        std::string data_ip = match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
        unsigned short data_port = static_cast<unsigned short>((std::stoi(match[5].str()) << 8) + std::stoi(match[6].str()));

        auto conn = std::make_unique<tls_data_connection>();
        conn->sock = ::socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
        if (conn->sock == INVALID_SOCKET) {
#else
        if (conn->sock < 0) {
#endif
            last_error = "Failed to create data socket.";
            return nullptr;
        }

        sockaddr_in data_addr{};
        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(data_port);
        inet_pton(AF_INET, data_ip.c_str(), &data_addr.sin_addr);

        if (::connect(conn->sock, reinterpret_cast<sockaddr*>(&data_addr), sizeof(data_addr)) != 0) {
            last_error = "Failed to connect to data port: " + data_ip + ":" + std::to_string(data_port);
            return nullptr;
        }

        // PROT P 모드일 경우: 제어 채널의 TLS 세션 티켓을 재사용(Resumption)하여 핸드셰이크 수행
        if (data_prot_ == ftps_data_protection::private_ssl) {
            conn->ssl = SSL_new(ssl_ctx_);
            if (!conn->ssl) {
                last_error = "Failed to create SSL instance for data connection.";
                return nullptr;
            }

            // [핵심] 제어 채널의 세션(TLS 1.3 티켓) 주입
            {
                std::lock_guard<std::mutex> lock(ssl_mutex_);
                if (ssl_control_) {
                    SSL_SESSION* sess = SSL_get1_session(ssl_control_);
                    if (sess) {
                        SSL_set_session(conn->ssl, sess);
                        SSL_SESSION_free(sess);
                    }
                }
            }

            SSL_set_fd(conn->ssl, static_cast<int>(conn->sock));
            if (!is_ip_address(host_name)) {
                SSL_set_tlsext_host_name(conn->ssl, host_name.c_str());
            }

            if (SSL_connect(conn->ssl) <= 0) {
                last_error = "Data channel TLS Handshake failed (Session Resumption rejected).";
                return nullptr;
            }
        }

        return conn;
        }

    bool ftps_client::connect(const std::string & host, int p, const std::string & user, const std::string & pass) {
        close_control_socket();
        host_name = host;
        port = p;
        user_name = user;
        password = pass;
        last_error.clear();

        if (!init_ssl_ctx()) return false;

        control_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
        if (control_socket_ == INVALID_SOCKET) {
#else
        if (control_socket_ < 0) {
#endif
            last_error = "Failed to create control socket.";
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<uint16_t>(port));
        inet_pton(AF_INET, host_name.c_str(), &server_addr.sin_addr);

        if (::connect(control_socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
            last_error = "Failed to connect to FTP server (TCP).";
            close_control_socket();
            return false;
        }

        std::string greeting = read_plain_response();
        if (greeting.rfind("220", 0) != 0) {
            last_error = "Invalid FTP welcome greeting: " + greeting;
            close_control_socket();
            return false;
        }

        if (!send_plain_command("AUTH", "TLS")) {
            last_error = "Failed to send AUTH TLS command.";
            close_control_socket();
            return false;
        }

        std::string auth_resp = read_plain_response();
        if (auth_resp.rfind("234", 0) != 0) {
            last_error = "Server rejected AUTH TLS: " + auth_resp;
            close_control_socket();
            return false;
        }

        ssl_control_ = SSL_new(ssl_ctx_);
        if (!ssl_control_) {
            last_error = "Failed to create SSL instance.";
            close_control_socket();
            return false;
        }

        SSL_set_fd(ssl_control_, static_cast<int>(control_socket_));
        if (!is_ip_address(host_name)) {
            SSL_set_tlsext_host_name(ssl_control_, host_name.c_str());
        }

        if (SSL_connect(ssl_control_) <= 0) {
            last_error = "Explicit TLS Handshake failed after AUTH TLS.";
            close_control_socket();
            return false;
        }

        is_connected_ = true;
        stop_flag_ = false;
        receive_thread_ = std::thread(&ftps_client::receive_loop, this);

        if (!send_command("PBSZ", "0")) return false;
        std::string resp = read_control_response();
        if (resp.rfind("200", 0) != 0) {
            last_error = "PBSZ command rejected: " + resp;
            return false;
        }

        std::string prot_arg = (data_prot_ == ftps_data_protection::private_ssl) ? "P" : "C";
        if (!send_command("PROT", prot_arg)) return false;
        resp = read_control_response();
        if (resp.rfind("200", 0) != 0) {
            last_error = "PROT command rejected: " + resp;
            return false;
        }

        if (!send_command("USER", user_name)) return false;
        resp = read_control_response();

        if (resp.rfind("331", 0) == 0) {
            if (!send_command("PASS", password)) return false;
            resp = read_control_response();
        }

        if (resp.rfind("230", 0) != 0) {
            last_error = "FTPS Login rejected: " + resp;
            return false;
        }

        return true;
        }

    bool ftps_client::download(const std::string & remote_file, const std::string & local_file) {
        std::int64_t dltotal = get_remote_file_size(remote_file);

        if (!send_command("TYPE", "I")) return false;
        read_control_response();

        auto data_conn = establish_tls_data_connection();
        if (!data_conn) return false;

        if (!send_command("RETR", remote_file)) {
            data_conn->close_connection();
            return false;
        }

        std::string resp = read_control_response();
        if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
            last_error = "RETR rejected: " + resp;
            data_conn->close_connection();
            return false;
        }

        std::ofstream ofs(local_file, std::ios::binary);
        if (!ofs.is_open()) {
            last_error = "Failed to open local file: " + local_file;
            data_conn->close_connection();
            return false;
        }

        char buffer[16384];
        std::int64_t dlnow = 0;
        while (true) {
            int bytes = data_conn->read_data(buffer, sizeof(buffer));
            if (bytes > 0) {
                ofs.write(buffer, bytes);
                dlnow += bytes;
                if (progress_listener) {
                    progress_listener->on_progress(dlnow, dltotal, 0, 0);
                }
            }
            else {
                break;
            }
        }

        ofs.close();
        data_conn->close_connection();

        resp = read_control_response();
        return (resp.rfind("226", 0) == 0 || resp.rfind("250", 0) == 0);
    }

    bool ftps_client::upload(const std::string & local_file, const std::string & remote_file) {
        std::ifstream ifs(local_file, std::ios::binary | std::ios::ate);
        if (!ifs.is_open()) {
            last_error = "Failed to open local file: " + local_file;
            return false;
        }
        std::int64_t ultotal = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        if (!send_command("TYPE", "I")) return false;
        read_control_response();

        auto data_conn = establish_tls_data_connection();
        if (!data_conn) return false;

        if (!send_command("STOR", remote_file)) {
            data_conn->close_connection();
            return false;
        }

        std::string resp = read_control_response();
        if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
            last_error = "STOR rejected: " + resp;
            data_conn->close_connection();
            return false;
        }

        char buffer[16384];
        std::int64_t ulnow = 0;
        while (ifs.good()) {
            ifs.read(buffer, sizeof(buffer));
            std::streamsize bytes = ifs.gcount();
            if (bytes > 0) {
                data_conn->write_data(buffer, static_cast<int>(bytes));
                ulnow += bytes;
                if (progress_listener) {
                    progress_listener->on_progress(0, 0, ulnow, ultotal);
                }
            }
        }

        data_conn->close_connection();

        resp = read_control_response();
        return (resp.rfind("226", 0) == 0);
    }

    std::vector<file_info> ftps_client::list_directory(const std::string & path) {
        if (!send_command("TYPE", "A")) return {};
        read_control_response();

        auto data_conn = establish_tls_data_connection();
        if (!data_conn) return {};

        if (!send_command("LIST", path)) {
            data_conn->close_connection();
            return {};
        }

        std::string resp = read_control_response();
        if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
            last_error = "LIST rejected: " + resp;
            data_conn->close_connection();
            return {};
        }

        std::string raw_list;
        char buffer[8192];
        while (true) {
            int bytes = data_conn->read_data(buffer, sizeof(buffer));
            if (bytes > 0) {
                raw_list.append(buffer, bytes);
            }
            else {
                break;
            }
        }

        data_conn->close_connection();
        read_control_response();

        std::vector<file_info> results;
        std::istringstream iss(raw_list);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) continue;

            file_info info;
            info.is_directory = (line[0] == 'd');

            std::istringstream line_tokens(line);
            std::string perms, links, owner, group, size_str, month, day, time_year, name;
            if (line_tokens >> perms >> links >> owner >> group >> size_str >> month >> day >> time_year) {
                std::getline(line_tokens, name);
                if (!name.empty() && name[0] == ' ') name = name.substr(1);
                if (!name.empty() && name.back() == '\r') name.pop_back();

                info.name = name;
                try {
                    info.size = info.is_directory ? 0 : std::stoll(size_str);
                }
                catch (...) {
                    info.size = 0;
                }

                if (!info.name.empty() && info.name != "." && info.name != "..") {
                    results.push_back(info);
                }
            }
        }
        return results;
    }

    bool ftps_client::delete_file(const std::string & remote_file) {
        if (!send_command("DELE", remote_file)) return false;
        return (read_control_response().rfind("250", 0) == 0);
    }

    bool ftps_client::create_directory(const std::string & path) {
        if (!send_command("MKD", path)) return false;
        return (read_control_response().rfind("257", 0) == 0);
    }

    bool ftps_client::remove_directory(const std::string & path) {
        if (!send_command("RMD", path)) return false;
        return (read_control_response().rfind("250", 0) == 0);
    }

    } // namespace mino::network_openssl::ftps
