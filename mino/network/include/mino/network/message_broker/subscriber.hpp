#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>

#include <spdlog/spdlog.h>

#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::message_broker {

    class subscriber {
    public:
        using message_callback = std::function<void(std::string_view topic, std::string_view msg_kind, std::string_view body, uint64_t timestamp)>;

    protected:
        mino::network::tcp::tcp_client client;
        std::string stream_buffer;
        message_callback app_callback{ nullptr };

        std::string broker_ip;
        unsigned short broker_port{ 0 };
        std::vector<std::string> sub_topics;
        std::chrono::seconds retry_interval{ std::chrono::seconds(2) };
        int max_retries{ 0 };
        std::shared_ptr<spdlog::logger> logger;

    public:
        subscriber(std::shared_ptr<spdlog::logger> custom_logger = nullptr);
        ~subscriber() = default;

        void set_broker(const std::string& ip, unsigned short port); // 브로커 IP와 포트 설정
        void set_logger(std::shared_ptr<spdlog::logger> custom_logger); // 로거 설정
        void set_topic(const std::vector<std::string>& topics); // 구독할 주제 설정
        void set_on_message_handler(message_callback cb); // 메시지 수신 시 애플리케이션에서 호출할 콜백 함수 설정
        void set_connection_option(std::chrono::seconds interval = std::chrono::seconds(2), int retries = 0); // 브로커와의 연결 재시도 간격과 최대 재시도 횟수 설정

        bool connect(std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60)); // 브로커에 연결 시도
        bool is_connected() const; // 현재 연결 상태 확인

        void disconnect(); // 브로커와의 연결을 정상적으로 종료 (구독 취소 메시지 전송 후 연결 종료)
        void shutdown_by_force(); // 브로커와의 연결을 강제로 종료 (구독 취소 메시지 없이 즉시 종료, 재연결 시 대기 불필요)

    };

} 