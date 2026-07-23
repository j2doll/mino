#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <spdlog/spdlog.h>

#include "mino/network/tcp/tcp_server.hpp"

namespace mino::network::memory_store {

    class memory_store_server {
    protected:
        mino::network::tcp::tcp_server server_;
        std::unordered_map<std::string, std::string> storage_;
        mutable std::shared_mutex storage_mutex_;
        std::string bind_ip_;
        std::shared_ptr<spdlog::logger> logger_;
        std::thread auto_save_thread_;
        std::condition_variable auto_save_cv_;
        std::mutex auto_save_mutex_;
        int bind_port_;
        std::string db_filepath_;
        std::atomic<bool> is_auto_save_running_;
        std::chrono::seconds auto_save_interval_{ 0 }; // 0 이면 비활성화
        std::chrono::milliseconds sleep_for_transmission_;

    protected:
        void handle_on_receive(socket_t client_socket, const std::string& data);
        bool save_to_file(const std::string& filename);
        bool load_from_file(const std::string& filename);
        void auto_save_loop();

    public:
        memory_store_server();
        ~memory_store_server();

        void set_network(const std::string& ip, int port);
        void set_storage_file(const std::string& filepath);
        void set_logger(std::shared_ptr<spdlog::logger> logger_ptr);
        void set_auto_save(std::chrono::seconds interval);
        void set_sleep_for_transmission(std::chrono::milliseconds sleep_duration);

        bool start();
        void stop();

        bool load();
        void print_all() const;

    };

}  
