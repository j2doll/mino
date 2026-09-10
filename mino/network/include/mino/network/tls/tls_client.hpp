#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network::tls {

    class tls_client {
    public:
        using callback = std::function<void()>;
        using receive_callback = std::function<void(const std::string&)>;

    protected:
        std::string server_ip;
        unsigned short server_port;
        int address_family;

        socket_t socket_fd;
        std::atomic<bool> is_connected_flag;

        std::thread client_thread;
        std::atomic<bool> stop_flag;
        std::atomic<bool> thread_running;

        // SSL* 객체의 모든 I/O 및 라이프사이클을 직렬화하는 단일 뮤텍스
        std::mutex ssl_mutex;

        callback on_connect;
        callback on_close;
        receive_callback on_receive;

        std::shared_ptr<mino::core::log::tinylog::logger> logger;

        SSL_CTX* ssl_ctx;
        SSL* ssl_handle;
        bool verify_peer;
        std::string ca_cert_file;
        std::string ca_cert_path;
        std::string sni_hostname;
        std::string client_cert_file;
        std::string client_key_file;

        static constexpr int BUFFER_SIZE = 16384;

    public:
        tls_client();
        virtual ~tls_client();

        void set_server(const std::string& ip, unsigned short port, int family = AF_INET);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);
        void set_on_connect(callback cb);
        void set_on_close(callback cb);
        void set_on_receive(receive_callback cb);

        void set_verify_peer(bool verify);
        void set_ca_cert(const std::string& ca_file, const std::string& ca_path = "");
        void set_sni_hostname(const std::string& hostname);
        bool set_client_certificate(const std::string& cert_file, const std::string& key_file);

        bool start(std::chrono::seconds sleep_time = std::chrono::seconds(60));
        bool is_connected() const;

        int send_data(const std::string& data);

        void close_connection();
        void stop();
        void shutdown_by_force();

    protected:
        bool init_ssl_context();
        void cleanup_ssl_context();
        void connect_to_server(std::chrono::seconds sleep_time);
        void receive_loop();
    };

}
