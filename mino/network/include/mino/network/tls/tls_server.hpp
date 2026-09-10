#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network::tls {

    struct tls_session {
        socket_t client_socket;
        SSL* ssl{ nullptr };
        std::mutex ssl_mutex; // 세션별 SSL* 전용 단일 뮤텍스
    };

    class tls_server {
    public:
        using callback = std::function<void(socket_t, const std::string&)>;

    protected:
        socket_t server_socket;
        std::unordered_map<socket_t, std::shared_ptr<tls_session>> client_sessions;
        std::mutex sessions_mutex;

        int address_family = AF_UNSPEC;
        std::atomic<bool> is_running;
        std::thread server_thread;

        callback on_connect;
        callback on_receive;
        callback on_close;

        std::shared_ptr<mino::core::log::tinylog::logger> logger;

        SSL_CTX* ssl_ctx;
        std::string cert_file;
        std::string key_file;
        std::string ca_file;
        bool verify_client;

        static constexpr int BUFFER_SIZE = 16384;

    public:
        tls_server();
        virtual ~tls_server();

        enum class start_result {
            success,
            ssl_init_failed,
            socket_creation_failed,
            bind_failed,
            listen_failed
        };

        bool set_certificate_and_key(const std::string& certificate_path, const std::string& private_key_path);
        void set_client_verification(bool verify, const std::string& ca_cert_path = "");

        start_result start(const std::string& ip, unsigned short port);

        void set_on_connect_callback(callback cb);
        void set_on_receive_callback(callback cb);
        void set_on_close_callback(callback cb);

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);

        std::vector<socket_t> get_client_sockets();
        int send_to_client(socket_t client_socket, const std::string& message);
        std::vector<socket_t> broadcast_to_clients(const std::string& message);

        void close_client(socket_t client_socket);
        void quit();
        void shutdown_by_force();

    protected:
        bool init_ssl_context();
        void cleanup_ssl_context();
        void accept_loop();
        void client_handler(socket_t client_socket);
    };

}
