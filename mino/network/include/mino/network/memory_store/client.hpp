#pragma once

#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <vector>
#include <utility>
#include <chrono>
#include <queue>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::memory_store {

    class memory_store_client {
    protected:
        mino::network::tcp::tcp_client client_;
        std::mutex response_mutex_;
        std::condition_variable response_cv_;

        // 스트림 수신 버퍼 및 큐
        std::string rx_buffer_;
        std::queue<std::string> response_queue_;

        // 다중 스레드 동시 요청을 직렬화하는 트랜잭션 락
        std::recursive_mutex request_mutex_;

        std::string remote_ip_;
        unsigned short remote_port_;
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;
        std::chrono::seconds timeout_;

    protected:
        std::optional<std::string> send_and_wait(const std::string& command, std::chrono::seconds timeout);
        std::optional<std::string> send_and_wait_locked(const std::string& command, std::chrono::seconds timeout);
        int del_locked(const std::string& key);

    public:
        memory_store_client();
        ~memory_store_client();

        void set_server_env(const std::string& ip, unsigned short port);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);
        void set_timeout(std::chrono::seconds timeout);

        bool connect(std::chrono::seconds tcp_timeout);
        void stop();

        bool set(const std::string& key, const std::string& value);
        std::optional<std::string> get(const std::string& key);
        int del(const std::string& key);
        int delete_all();

        bool request_server_save();
        bool request_server_load();

        std::vector<std::pair<std::string, std::string>> request_server_dump(std::chrono::seconds timeout);
        long request_server_sleep_ms();
        long request_server_latency_ms();
    };

}
