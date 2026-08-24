#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/tcp/tcp_server.hpp"

namespace mino::network::message_broker {

    class broker {
    protected:
        mino::network::tcp::tcp_server server;
        std::unordered_map<std::string, std::vector<socket_t>> topic_registry;
        std::unordered_map<socket_t, std::string> stream_buffers;
        std::mutex registry_mutex;
        std::shared_ptr<mino::core::log::tinylog::logger> logger;

    public:
        broker(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger = nullptr);
        ~broker() = default;

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger); // 로거 설정

        bool start_broker(const std::string& ip, int port); // 브로커 시작
        void quit(); // 브로커 정상 종료
        bool shutdown_by_force(); // 브로커 강제 종료
    };

}
