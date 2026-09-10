#include <cerrno>
#include <cstring>
#include <sstream>
#include <system_error>
#include <algorithm>

#include "mino/network/tls/tls_client.hpp"

namespace mino::network::tls {

    namespace {
        std::string get_openssl_error_string() {
            BIO* bio = BIO_new(BIO_s_mem());
            if (!bio) return "Failed to allocate memory BIO for OpenSSL error";
            ERR_print_errors(bio);
            char* buf = nullptr;
            long len = BIO_get_mem_data(bio, &buf);
            std::string result(buf ? buf : "", len > 0 ? static_cast<size_t>(len) : 0);
            BIO_free(bio);
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            return result.empty() ? "No OpenSSL error details" : result;
        }

        // 크로스 플랫폼 논블로킹 설정
        bool set_socket_nonblocking(socket_t fd) {
#ifdef _WIN32
            u_long mode = 1;
            return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1) return false;
            return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
        }

        bool wait_socket_readable(socket_t fd, int timeout_ms) {
            if (fd
#ifdef _WIN32
                == INVALID_SOCKET
#else
                < 0
#endif
                ) {
                return false;
            }

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);

            timeval tv{};
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int ret = ::select(static_cast<int>(fd) + 1, &read_fds, nullptr, nullptr, &tv);
            return (ret > 0 && FD_ISSET(fd, &read_fds));
        }

        bool wait_socket_writable(socket_t fd, int timeout_ms) {
            if (fd
#ifdef _WIN32
                == INVALID_SOCKET
#else
                < 0
#endif
                ) {
                return false;
            }

            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(fd, &write_fds);

            timeval tv{};
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int ret = ::select(static_cast<int>(fd) + 1, nullptr, &write_fds, nullptr, &tv);
            return (ret > 0 && FD_ISSET(fd, &write_fds));
        }
    }

    static void close_socket_without_timewait(socket_t fd) {
        if (fd
#ifdef _WIN32
            == INVALID_SOCKET
#else
            < 0
#endif
            ) {
            return;
        }

        struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = 0;

#ifdef _WIN32
        setsockopt(fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&so_linger), static_cast<int>(sizeof(so_linger)));
        closesocket(fd);
#else
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, static_cast<socklen_t>(sizeof(so_linger)));
        close(fd);
#endif
    }

    tls_client::tls_client()
        : address_family(AF_INET),
#ifdef _WIN32
        socket_fd(INVALID_SOCKET),
#else
        socket_fd(-1),
#endif
        is_connected_flag(false),
        stop_flag(false),
        thread_running(false),
        server_port(0),
        ssl_ctx(nullptr),
        ssl_handle(nullptr),
        verify_peer(false) {
    }

    tls_client::~tls_client() {
        stop();
        cleanup_ssl_context();
    }

    void tls_client::set_server(const std::string& ip, unsigned short port, int family) {
        server_ip = ip;
        server_port = port;
        address_family = family;
    }

    void tls_client::set_on_connect(callback cb) { on_connect = std::move(cb); }
    void tls_client::set_on_close(callback cb) { on_close = std::move(cb); }
    void tls_client::set_on_receive(receive_callback cb) { on_receive = std::move(cb); }

    void tls_client::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    void tls_client::set_verify_peer(bool verify) {
        verify_peer = verify;
    }

    void tls_client::set_ca_cert(const std::string& ca_file, const std::string& ca_path) {
        ca_cert_file = ca_file;
        ca_cert_path = ca_path;
    }

    void tls_client::set_sni_hostname(const std::string& hostname) {
        sni_hostname = hostname;
    }

    bool tls_client::set_client_certificate(const std::string& cert_file, const std::string& key_file) {
        client_cert_file = cert_file;
        client_key_file = key_file;
        return true;
    }

    bool tls_client::init_ssl_context() {
        if (ssl_ctx) return true;

        const SSL_METHOD* method = TLS_client_method();
        ssl_ctx = SSL_CTX_new(method);
        if (!ssl_ctx) {
            if (logger) logger->error("[tls_client] Failed to create SSL_CTX: <bright_red>{}</bright_red>", get_openssl_error_string());
            return false;
        }

        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

        if (verify_peer) {
            SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, nullptr);
            if (!ca_cert_file.empty() || !ca_cert_path.empty()) {
                const char* file = ca_cert_file.empty() ? nullptr : ca_cert_file.c_str();
                const char* path = ca_cert_path.empty() ? nullptr : ca_cert_path.c_str();
                if (SSL_CTX_load_verify_locations(ssl_ctx, file, path) <= 0) {
                    if (logger) logger->warn("[tls_client] Failed to load CA verify locations: {}", get_openssl_error_string());
                }
            }
            else {
                SSL_CTX_set_default_verify_paths(ssl_ctx);
            }
        }
        else {
            SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);
        }

        if (!client_cert_file.empty() && !client_key_file.empty()) {
            if (SSL_CTX_use_certificate_file(ssl_ctx, client_cert_file.c_str(), SSL_FILETYPE_PEM) <= 0 ||
                SSL_CTX_use_PrivateKey_file(ssl_ctx, client_key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
                if (logger) logger->error("[tls_client] Failed to load client credentials: <bright_red>{}</bright_red>", get_openssl_error_string());
                return false;
            }
        }

        return true;
    }

    void tls_client::cleanup_ssl_context() {
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
        }
    }

    bool tls_client::start(std::chrono::seconds sleep_time) {
        if (server_ip.empty() || server_port == 0) return false;
        if (!init_ssl_context()) return false;

        stop_flag = false;
        if (client_thread.joinable()) {
            try {
                std::thread tmp = std::move(client_thread);
                if (tmp.joinable()) tmp.detach();
            }
            catch (...) {}
        }

        client_thread = std::thread(&tls_client::connect_to_server, this, sleep_time);
        return true;
    }

    void tls_client::stop() {
        stop_flag = true;
        close_connection();

        if (client_thread.joinable()) {
            if (client_thread.get_id() != std::this_thread::get_id()) {
                client_thread.join();
            }
            else {
                client_thread.detach();
            }
        }
    }

    int tls_client::send_data(const std::string& data) {
        if (!is_connected() || data.empty()) return -1;

        std::lock_guard<std::mutex> lock(ssl_mutex);
        if (!ssl_handle) return -1;

        int total_written = 0;
        int to_write = static_cast<int>(data.size());

        while (total_written < to_write) {
            int ret = SSL_write(ssl_handle, data.data() + total_written, to_write - total_written);
            if (ret > 0) {
                total_written += ret;
            }
            else {
                int err = SSL_get_error(ssl_handle, ret);
                if (err == SSL_ERROR_WANT_WRITE) {
                    if (!wait_socket_writable(socket_fd, 50)) {
                        break;
                    }
                    continue;
                }
                if (logger && !stop_flag) {
                    logger->error("[tls_client] SSL_write error: <bright_red>{}</bright_red> ({})", err, get_openssl_error_string());
                }
                return -1;
            }
        }
        return total_written;
    }

    void tls_client::close_connection() {
        bool should_notify = false;
        {
            std::lock_guard<std::mutex> lock(ssl_mutex);
            if (!is_connected_flag && !ssl_handle && socket_fd ==
#ifdef _WIN32
                INVALID_SOCKET
#else
                - 1
#endif
                ) {
                return;
            }

            is_connected_flag = false;
            should_notify = true;

            if (ssl_handle) {
                SSL_shutdown(ssl_handle);
                SSL_free(ssl_handle);
                ssl_handle = nullptr;
            }

#ifdef _WIN32
            if (socket_fd != INVALID_SOCKET) {
                ::shutdown(socket_fd, SD_BOTH);
                closesocket(socket_fd);
                socket_fd = INVALID_SOCKET;
            }
#else
            if (socket_fd >= 0) {
                ::shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                socket_fd = -1;
            }
#endif
        }

        if (should_notify && on_close) {
            on_close();
        }
    }

    bool tls_client::is_connected() const {
        return is_connected_flag;
    }

    void tls_client::connect_to_server(std::chrono::seconds sleep_time) {
        thread_running = true;

        while (!stop_flag) {
            socket_t tmp_fd = ::socket(address_family, SOCK_STREAM, 0);
#ifdef _WIN32
            if (tmp_fd == INVALID_SOCKET) {
#else
            if (tmp_fd < 0) {
#endif
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            sockaddr_storage server_addr_storage{};
            socklen_t addr_len = 0;

            if (address_family == AF_INET) {
                auto* addr4 = reinterpret_cast<sockaddr_in*>(&server_addr_storage);
                addr4->sin_family = AF_INET;
                addr4->sin_port = htons(server_port);
                inet_pton(AF_INET, server_ip.c_str(), &addr4->sin_addr);
                addr_len = sizeof(sockaddr_in);
            }
            else if (address_family == AF_INET6) {
                auto* addr6 = reinterpret_cast<sockaddr_in6*>(&server_addr_storage);
                addr6->sin6_family = AF_INET6;
                addr6->sin6_port = htons(server_port);
                inet_pton(AF_INET6, server_ip.c_str(), &addr6->sin6_addr);
                addr_len = sizeof(sockaddr_in6);
            }

            if (::connect(tmp_fd, reinterpret_cast<sockaddr*>(&server_addr_storage), addr_len) == 0) {
                SSL* tmp_ssl = SSL_new(ssl_ctx);
                if (!tmp_ssl) {
#ifdef _WIN32
                    closesocket(tmp_fd);
#else
                    close(tmp_fd);
#endif
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    continue;
                }

                SSL_set_fd(tmp_ssl, static_cast<int>(tmp_fd));

                if (!sni_hostname.empty()) {
                    SSL_set_tlsext_host_name(tmp_ssl, sni_hostname.c_str());
                    if (verify_peer) SSL_set1_host(tmp_ssl, sni_hostname.c_str());
                }

                // 핸드셰이크 수행
                int handshake = SSL_connect(tmp_ssl);
                if (handshake <= 0) {
                    if (logger && !stop_flag) {
                        logger->warn("[tls_client] SSL Handshake failed: <bright_yellow>{}</bright_yellow>", get_openssl_error_string());
                    }
                    SSL_free(tmp_ssl);
#ifdef _WIN32
                    closesocket(tmp_fd);
#else
                    close(tmp_fd);
#endif
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    continue;
                }

                // 핸드셰이크 완료 즉시 논블로킹 모드로 전환
                set_socket_nonblocking(tmp_fd);

                {
                    std::lock_guard<std::mutex> lock(ssl_mutex);
                    socket_fd = tmp_fd;
                    ssl_handle = tmp_ssl;
                    is_connected_flag = true;
                }

                if (logger) {
                    logger->info("[tls_client] <bright_green>Connected</bright_green> to {}:{} (Cipher: {})",
                        server_ip, server_port, SSL_get_cipher(ssl_handle));
                }

                if (on_connect) on_connect();

                receive_loop();
            }
            else {
#ifdef _WIN32
                closesocket(tmp_fd);
#else
                close(tmp_fd);
#endif
            }

            auto total_wait = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time);
            auto elapsed = std::chrono::milliseconds(0);
            while (!stop_flag && elapsed < total_wait) {
                auto chunk = std::min(std::chrono::milliseconds(100), total_wait - elapsed);
                std::this_thread::sleep_for(chunk);
                elapsed += chunk;
            }
            }

        thread_running = false;
        }

    void tls_client::receive_loop() {
        char buffer[BUFFER_SIZE];

        while (!stop_flag && is_connected_flag) {
            bool has_pending = false;
            {
                std::lock_guard<std::mutex> lock(ssl_mutex);
                if (ssl_handle) {
                    has_pending = (SSL_pending(ssl_handle) > 0);
                }
            }

            if (!has_pending && !wait_socket_readable(socket_fd, 50)) {
                continue;
            }

            int bytes = 0;
            int ssl_err = SSL_ERROR_NONE;
            {
                std::lock_guard<std::mutex> lock(ssl_mutex);
                if (!ssl_handle || !is_connected_flag) break;

                // 논블로킹이므로 제어 패킷 처리 후 데이터가 없으면 즉시 WANT_READ 반환
                bytes = SSL_read(ssl_handle, buffer, sizeof(buffer) - 1);
                if (bytes <= 0) {
                    ssl_err = SSL_get_error(ssl_handle, bytes);
                }
            }

            if (bytes > 0) {
                if (on_receive) on_receive(std::string(buffer, bytes));
            }
            else {
                if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
                    continue; // 락을 해제하고 다음 폴링으로 진행
                }
                break;
            }
        }

        close_connection();
    }

    void tls_client::shutdown_by_force() {
        stop_flag = true;
        {
            std::lock_guard<std::mutex> lock(ssl_mutex);
            is_connected_flag = false;
            if (ssl_handle) {
                SSL_free(ssl_handle);
                ssl_handle = nullptr;
            }
            close_socket_without_timewait(socket_fd);
#ifdef _WIN32
            socket_fd = INVALID_SOCKET;
#else
            socket_fd = -1;
#endif
        }

        if (client_thread.joinable()) {
            if (client_thread.get_id() != std::this_thread::get_id()) {
                client_thread.join();
            }
            else {
                client_thread.detach();
            }
        }
        if (on_close) on_close();
    }

    }
