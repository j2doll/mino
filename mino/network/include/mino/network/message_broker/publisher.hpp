#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <utility>
#include <memory>

#include <spdlog/spdlog.h>

#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::message_broker {

    class publisher {
    protected: 
        mino::network::tcp::tcp_client client;
        std::string broker_ip;
        unsigned short broker_port{ 0 };
        std::chrono::seconds retry_interval{ std::chrono::seconds(2) };
        int max_retries{ 0 };
        std::shared_ptr<spdlog::logger> logger;

    public:
        publisher(std::shared_ptr<spdlog::logger> custom_logger = nullptr);
        ~publisher() = default;

        void set_logger(std::shared_ptr<spdlog::logger> custom_logger); // 로거 설정
        void set_broker(const std::string& ip, unsigned short port); // 브로커 IP와 포트 설정
        void set_connection_option(std::chrono::seconds interval = std::chrono::seconds(2), int retries = 0); // 브로커와의 연결 재시도 간격과 최대 재시도 횟수 설정

        bool connect(std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60)); // 브로커에 연결 시도
        bool is_connected() const; // 현재 연결 상태 확인
        void disconnect(); // 브로커와의 연결을 정상적으로 종료
        void shutdown_by_force(); // 브로커와의 연결을 강제로 종료 (TIME_WAIT 없이 즉시 종료, 재연결 시 대기 불필요)

        // 메시지 발행 함수 (성공 여부와 오류 메시지를 반환)
        std::pair<bool, std::string> publish(
            std::string_view topic, // 메시지가 속한 주제
            std::string_view msg_kind, // 메시지의 세부 종류
            std::string_view message); // 메시지 본문

    };

}  
