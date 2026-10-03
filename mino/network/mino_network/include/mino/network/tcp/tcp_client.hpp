#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include <memory>
#include <algorithm>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network::tcp {

    // 재연결 설정 정보 구조체
    struct reconnect_config {
        std::chrono::milliseconds initial_interval{ 10000 }; // 최초 재연결 대기 시간 (기본 10초)
        std::chrono::milliseconds max_interval{ 60000 }; // 최대 대기 시간 상한선 (기본 60초, 도달 후 이 간격으로 계속 유지)
        int max_retries{ 0 }; // 처음 실패한 시점부터 누적된 '총 재시도 횟수' (0: 최대 시간에 도달해도 무제한 계속 재시도)
        double backoff_multiplier{ 2.0 }; // 지수 증가 배수 (기본 2.0배) (10s -> 20s -> 40s -> 60s -> 60s ...)
    };

    class tcp_client {
    public:
        using callback = std::function<void()>;
        using receive_callback = std::function<void(const std::string&)>;

    protected:
        std::string server_ip;
        unsigned short server_port;
        int address_family;

        socket_t socket_fd;
        bool is_connected_flag;

        std::thread client_thread;
        std::atomic<bool> stop_flag;
        std::atomic<bool> thread_running;
        std::mutex send_mutex;
        callback on_connect;
        callback on_close;
        receive_callback on_receive;

        std::shared_ptr<mino::core::log::tinylog::logger> logger;

        static constexpr int BUFFER_SIZE = 1024;

        reconnect_config recon_cfg;
        std::mutex cv_mutex;
        std::condition_variable stop_cv;

    public:
        tcp_client();
        ~tcp_client();

        void set_server(const std::string& ip, unsigned short port, int family = AF_INET);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);
        void set_on_connect(callback cb);
        void set_on_close(callback cb);
        void set_on_receive(receive_callback cb);

        void set_reconnect_config(const reconnect_config& config);
        const reconnect_config& get_reconnect_config() const;

        bool start();
        bool start(const reconnect_config& config);
        bool start(std::chrono::seconds sleep_time);
        bool is_connected() const;

        int send_data(const std::string& data);

        void close_connection();
        void stop();
        void shutdown_by_force();

    protected:
        void connect_to_server();
        void receive_loop();
    };

} // namespace mino::network::tcp
