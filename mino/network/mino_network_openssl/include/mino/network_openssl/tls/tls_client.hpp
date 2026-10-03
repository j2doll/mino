#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <algorithm>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network_openssl::tls {

    // 재연결 설정 정보 구조체
    struct reconnect_config {
        std::chrono::milliseconds initial_interval{ 10000 }; // 최초 재연결 대기 시간 (기본 10초)
        std::chrono::milliseconds max_interval{ 60000 };     // 최대 대기 시간 상한선 (기본 60초, 도달 후 이 간격 유지)
        int max_retries{ 0 };                                // 최대 시도 횟수 (0: 무제한 계속 재시도)
        double backoff_multiplier{ 2.0 };                    // 지수 증가 배수 (기본 2.0배)
    };

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

        // 재연결 설정 및 즉시 정지를 위한 동기화 객체
        reconnect_config recon_cfg;
        std::mutex cv_mutex;
        std::condition_variable stop_cv;

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

        // 재연결 설정
        void set_reconnect_config(const reconnect_config& config);
        const reconnect_config& get_reconnect_config() const;

        bool start();
        bool start(const reconnect_config& config);
        bool start(std::chrono::seconds sleep_time); // 기존 호환용 고정 간격 start

        bool is_connected() const;
        bool is_running() const; // 재연결 스레드 동작 여부 조회

        int send_data(const std::string& data);

        void close_connection();
        void stop();
        void shutdown_by_force();

    protected:
        bool init_ssl_context();
        void cleanup_ssl_context();
        void connect_to_server();
        void receive_loop();
    };

}
