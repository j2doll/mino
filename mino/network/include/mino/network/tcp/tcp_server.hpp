#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"

namespace mino::network::tcp {

    class tcp_server {
    public:
        using callback = std::function<void(socket_t, const std::string&)>;

    protected:
        socket_t server_socket;
        std::vector<socket_t> client_sockets;

        int address_family = AF_UNSPEC;
        std::atomic<bool> is_running;
        std::thread server_thread;
        std::mutex send_mutex;

        callback on_connect;
        callback on_receive;
        callback on_close;

        std::shared_ptr<mino::core::log::tinylog::logger> logger;

        static constexpr int BUFFER_SIZE = 1024;

    public:
        tcp_server();
        ~tcp_server();

        enum class start_result {
            success,
            socket_creation_failed,
            bind_failed,
            listen_failed
        };

        start_result start(const std::string& ip, int port);

        void set_on_connect_callback(callback cb);
        void set_on_receive_callback(callback cb);
        void set_on_close_callback(callback cb);

        // tinylog 로거 등록
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);

        std::vector<socket_t> get_client_sockets();

        int send_to_client(socket_t client_socket, const std::string& message);
        std::vector<socket_t> broadcast_to_clients(const std::string& message);

        void close_client(socket_t client_socket);
        void quit();
        void shutdown_by_force();

    protected:
        void accept_loop();
        void client_handler(socket_t client_socket);
    };

}
