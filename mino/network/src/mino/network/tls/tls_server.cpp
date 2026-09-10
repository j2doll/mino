#include <iostream>
#include <algorithm>
#include <cstring>
#include <system_error>

#include "mino/network/tls/tls_server.hpp"

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

    tls_server::tls_server()
        : is_running(false),
        logger(nullptr),
        ssl_ctx(nullptr),
        verify_client(false),
#ifdef _WIN32
        server_socket(INVALID_SOCKET)
#else
        server_socket(-1)
#endif
    {
    }

    tls_server::~tls_server() {
        quit();
        cleanup_ssl_context();
    }

    bool tls_server::set_certificate_and_key(const std::string& certificate_path, const std::string& private_key_path) {
        cert_file = certificate_path;
        key_file = private_key_path;
        return true;
    }

    void tls_server::set_client_verification(bool verify, const std::string& ca_cert_path) {
        verify_client = verify;
        ca_file = ca_cert_path;
    }

    bool tls_server::init_ssl_context() {
        if (ssl_ctx) return true;

        if (cert_file.empty() || key_file.empty()) {
            if (logger) logger->error("[tls_server] Certificate or key path is not set.");
            return false;
        }

        const SSL_METHOD* method = TLS_server_method();
        ssl_ctx = SSL_CTX_new(method);
        if (!ssl_ctx) return false;

        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

        if (SSL_CTX_use_certificate_file(ssl_ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0 ||
            !SSL_CTX_check_private_key(ssl_ctx)) {
            if (logger) logger->error("[tls_server] Key/Certificate error: <bright_red>{}</bright_red>", get_openssl_error_string());
            cleanup_ssl_context();
            return false;
        }

        if (verify_client) {
            SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
            if (!ca_file.empty()) {
                SSL_CTX_load_verify_locations(ssl_ctx, ca_file.c_str(), nullptr);
            }
        }

        return true;
    }

    void tls_server::cleanup_ssl_context() {
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
        }
    }

    tls_server::start_result tls_server::start(const std::string& ip, unsigned short port) {
        if (!init_ssl_context()) return start_result::ssl_init_failed;

        struct addrinfo hints {}, * res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        std::string port_str = std::to_string(port);
        if (getaddrinfo(ip.empty() ? nullptr : ip.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
            return start_result::socket_creation_failed;
        }

        for (auto* p = res; p != nullptr; p = p->ai_next) {
            server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#ifdef _WIN32
            if (server_socket == INVALID_SOCKET) continue;
#else
            if (server_socket < 0) continue;
#endif

            int opt = 1;
            setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

            if (bind(server_socket, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
                address_family = p->ai_family;
                break;
            }

#ifdef _WIN32
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
#else
            close(server_socket);
            server_socket = -1;
#endif
        }
        freeaddrinfo(res);

#ifdef _WIN32
        if (server_socket == INVALID_SOCKET) return start_result::bind_failed;
#else
        if (server_socket < 0) return start_result::bind_failed;
#endif

        if (listen(server_socket, SOMAXCONN) == -1) {
            return start_result::listen_failed;
        }

        is_running = true;
        server_thread = std::thread(&tls_server::accept_loop, this);
        return start_result::success;
    }

    void tls_server::set_on_connect_callback(callback cb) { on_connect = std::move(cb); }
    void tls_server::set_on_receive_callback(callback cb) { on_receive = std::move(cb); }
    void tls_server::set_on_close_callback(callback cb) { on_close = std::move(cb); }

    void tls_server::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    int tls_server::send_to_client(socket_t client_socket, const std::string& message) {
        std::shared_ptr<tls_session> session;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            auto it = client_sessions.find(client_socket);
            if (it != client_sessions.end()) session = it->second;
        }

        if (!session) return -1;

        std::lock_guard<std::mutex> lock(session->ssl_mutex);
        if (!session->ssl) return -1;

        int total_written = 0;
        int to_write = static_cast<int>(message.size());
        while (total_written < to_write) {
            int ret = SSL_write(session->ssl, message.data() + total_written, to_write - total_written);
            if (ret > 0) {
                total_written += ret;
            }
            else {
                int err = SSL_get_error(session->ssl, ret);
                if (err == SSL_ERROR_WANT_WRITE) {
                    if (!wait_socket_writable(client_socket, 50)) break;
                    continue;
                }
                break;
            }
        }
        return total_written;
    }

    std::vector<socket_t> tls_server::broadcast_to_clients(const std::string& message) {
        std::vector<std::shared_ptr<tls_session>> sessions;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            for (const auto& [_, s] : client_sessions) sessions.push_back(s);
        }

        std::vector<socket_t> failed;
        for (const auto& s : sessions) {
            std::lock_guard<std::mutex> lock(s->ssl_mutex);
            if (!s->ssl || SSL_write(s->ssl, message.data(), static_cast<int>(message.size())) <= 0) {
                failed.push_back(s->client_socket);
            }
        }
        return failed;
    }

    void tls_server::close_client(socket_t client_socket) {
        std::shared_ptr<tls_session> session;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            auto it = client_sessions.find(client_socket);
            if (it != client_sessions.end()) {
                session = it->second;
                client_sessions.erase(it);
            }
        }

        if (session) {
            {
                std::lock_guard<std::mutex> lock(session->ssl_mutex);
                if (session->ssl) {
                    SSL_shutdown(session->ssl);
                    SSL_free(session->ssl);
                    session->ssl = nullptr;
                }
            }

#ifdef _WIN32
            ::shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
#else
            ::shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
#endif
            if (on_close) on_close(client_socket, "Client disconnected");
        }
    }

    void tls_server::quit() {
        if (!is_running) return;
        is_running = false;

#ifdef _WIN32
        if (server_socket != INVALID_SOCKET) {
            ::shutdown(server_socket, SD_BOTH);
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
#else
        if (server_socket >= 0) {
            ::shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            server_socket = -1;
        }
#endif

        if (server_thread.joinable()) server_thread.join();

        std::unordered_map<socket_t, std::shared_ptr<tls_session>> sessions;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            sessions = std::move(client_sessions);
        }

        for (auto& [fd, s] : sessions) {
            if (s) {
                std::lock_guard<std::mutex> lock(s->ssl_mutex);
                if (s->ssl) {
                    SSL_shutdown(s->ssl);
                    SSL_free(s->ssl);
                    s->ssl = nullptr;
                }
            }
#ifdef _WIN32
            ::shutdown(fd, SD_BOTH);
            closesocket(fd);
#else
            ::shutdown(fd, SHUT_RDWR);
            close(fd);
#endif
        }
    }

    void tls_server::shutdown_by_force() {
        is_running = false;
        close_socket_without_timewait(server_socket);
#ifdef _WIN32
        server_socket = INVALID_SOCKET;
#else
        server_socket = -1;
#endif

        if (server_thread.joinable()) server_thread.join();

        std::unordered_map<socket_t, std::shared_ptr<tls_session>> sessions;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            sessions = std::move(client_sessions);
        }

        for (auto& [fd, s] : sessions) {
            if (s) {
                std::lock_guard<std::mutex> lock(s->ssl_mutex);
                if (s->ssl) {
                    SSL_free(s->ssl);
                    s->ssl = nullptr;
                }
            }
            close_socket_without_timewait(fd);
        }
    }

    std::vector<socket_t> tls_server::get_client_sockets() {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        std::vector<socket_t> sockets;
        for (const auto& [fd, _] : client_sessions) sockets.push_back(fd);
        return sockets;
    }

    void tls_server::accept_loop() {
        while (is_running) {
            sockaddr_storage client_addr{};
            socklen_t len = sizeof(client_addr);
            socket_t client_fd = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &len);

#ifdef _WIN32
            if (client_fd == INVALID_SOCKET) continue;
#else
            if (client_fd < 0) continue;
#endif

            std::thread(&tls_server::client_handler, this, client_fd).detach();
        }
    }

    void tls_server::client_handler(socket_t client_socket) {
        SSL* ssl = SSL_new(ssl_ctx);
        if (!ssl) {
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            return;
        }

        SSL_set_fd(ssl, static_cast<int>(client_socket));

        if (SSL_accept(ssl) <= 0) {
            if (logger && is_running) logger->warn("[tls_server] Client TLS handshake <bright_red>failed</bright_red>");
            SSL_free(ssl);
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            return;
        }

        // 서버 소켓도 핸드셰이크 직후 논블로킹 전환
        set_socket_nonblocking(client_socket);

        auto session = std::make_shared<tls_session>();
        session->client_socket = client_socket;
        session->ssl = ssl;

        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            client_sessions[client_socket] = session;
        }

        if (logger) {
            logger->info("[tls_server] Client connected: <bright_cyan>fd={}</bright_cyan>, Cipher: {}",
                client_socket, SSL_get_cipher(ssl));
        }

        if (on_connect) on_connect(client_socket, "Client handshake complete");

        char buffer[BUFFER_SIZE];
        while (is_running) {
            bool has_pending = false;
            {
                std::lock_guard<std::mutex> lock(session->ssl_mutex);
                if (session->ssl) {
                    has_pending = (SSL_pending(session->ssl) > 0);
                }
            }

            if (!has_pending && !wait_socket_readable(client_socket, 50)) {
                continue;
            }

            int bytes = 0;
            int ssl_err = SSL_ERROR_NONE;
            {
                std::lock_guard<std::mutex> lock(session->ssl_mutex);
                if (!session->ssl || !is_running) break;

                bytes = SSL_read(session->ssl, buffer, sizeof(buffer) - 1);
                if (bytes <= 0) {
                    ssl_err = SSL_get_error(session->ssl, bytes);
                }
            }

            if (bytes > 0) {
                if (on_receive) on_receive(client_socket, std::string(buffer, bytes));
            }
            else {
                if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                break;
            }
        }

        close_client(client_socket);
    }

}
