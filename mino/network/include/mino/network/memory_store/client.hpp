#pragma once

#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <vector>
#include <utility>

#include <spdlog/spdlog.h>

#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::memory_store {

    class  memory_store_client {
    protected:
        mino::network::tcp::tcp_client client_;
        std::mutex response_mutex_;
        std::condition_variable response_cv_;
        std::string current_response_;
        bool has_response_;
        std::string remote_ip_;
        unsigned short remote_port_;
        std::shared_ptr<spdlog::logger> logger_;
        std::chrono::seconds timeout_;

    protected:
        std::string send_and_wait(const std::string& command, std::chrono::seconds timeout);

    public:
        memory_store_client();
        ~memory_store_client();

        void set_server_env(const std::string& ip, unsigned short port);
        void set_logger(std::shared_ptr<spdlog::logger> logger_ptr);
        void set_timeout(std::chrono::seconds timeout);

        bool connect(std::chrono::seconds tcp_timeout);
        void stop();

        bool set(const std::string& key, const std::string& value);
        std::optional<std::string> get(const std::string& key);
        int del(const std::string& key);

        bool request_server_save();
        bool request_server_load();

        std::vector<std::pair<std::string, std::string>> request_server_dump(std::chrono::seconds timeout);

        // 서버의 sleep_for_transmission_ 값을 밀리초 단위로 요청하여 반환합니다.
        // 성공 시 0 이상의 밀리초 값을 반환, 실패 시 -1 반환.
        long request_server_sleep_ms();

        // 서버가 현재 키 수와 sleep_for_transmission_을 곱한 값을 밀리초 단위 정수로 반환합니다.
        // 성공 시 0 이상의 밀리초 값을 반환, 실패 시 -1 반환.
        long request_server_latency_ms();
    };

}  
