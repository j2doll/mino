#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <chrono>
#include <memory>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network::tcp {

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
        std::atomic<bool> thread_running; // 스레드 실행 상태 추적
        std::mutex send_mutex;
        callback on_connect;
        callback on_close;
        receive_callback on_receive;

        std::shared_ptr<mino::core::log::tinylog::logger> logger;

        static constexpr int BUFFER_SIZE = 1024;

    public:
        tcp_client();
        ~tcp_client();

        void set_server(const std::string& ip, unsigned short port, int family = AF_INET);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);
        void set_on_connect(callback cb);
        void set_on_close(callback cb);
        void set_on_receive(receive_callback cb);

        bool start(std::chrono::seconds sleep_time = std::chrono::seconds(60));
        bool is_connected() const;

        int send_data(const std::string& data);

        void close_connection(); // close connection gracefully (TIME_WAIT 발생, 재연결 시 잠시 대기 필요)
        void stop(); // stop tcp client thread and close connection gracefully
        void shutdown_by_force(); // force shutdown (SO_LINGER 설정으로 TIME_WAIT 없이 즉시 종료, 재연결 시 대기 불필요)

    protected:
        void connect_to_server(std::chrono::seconds sleep_time = std::chrono::seconds(1));
        void receive_loop();
    };

}
