#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <future>
#include <chrono>
#include <atomic>
#include <utility>

#include <nlohmann/json.hpp>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/message_broker/message_broker.hpp"
#include "mino/network/rpc/rpc_protocol_util.hpp"
#include "mino/network/rpc/rpc_status.hpp"

namespace mino::network::rpc {

    class rpc_client_base {
    public:
        using json = nlohmann::json;

    protected:
        mino::network::message_broker::publisher pub; // Publisher 멤버 변수
        mino::network::message_broker::subscriber sub; // Subscriber 멤버 변수
        std::string client_id; // 클라이언트 고유 ID (응답 토픽 생성에 사용)
        std::atomic<uint64_t> sequence_id{ 0 }; // 요청 ID 생성을 위한 시퀀스 번호 (원자적으로 증가)
        std::unordered_map<std::string, std::promise<json>> response_map; // 요청 ID와 응답을 매핑하는 맵 (요청 ID -> 응답 JSON)
        std::mutex map_mutex; // response_map에 대한 동시 접근을 제어하는 뮤텍스
        std::shared_ptr<mino::core::log::tinylog::logger> logger; // 로거 멤버 변수
        std::chrono::milliseconds connection_timeout{ 3000 }; // 브로커 연결 타임아웃 (밀리초 단위)

    protected:
        void on_message_received( // 메시지 수신 시 호출되는 콜백 함수
            std::string_view topic, // 메시지가 속한 주제
            std::string_view msg_kind, // 메시지의 세부 종류
            std::string_view body, // 메시지 본문 (직렬화된 JSON 문자열)
            uint64_t timestamp); // 메세지 생성 시간의 타임스탬프 (밀리초 단위)

        void handle_disconnection(); // 브로커와의 연결이 끊어졌을 때 호출되는 함수 (연결 끊김 상태로 전환)

    public:
        rpc_client_base();
        virtual ~rpc_client_base();

        // id 설정 
        void set_id(
            std::string unique_id); // 클라이언트 고유 ID 설정 (응답 토픽 생성에 사용)

        // tinylog 로거 설정
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger);

        // 브로커 설정
        void set_broker(
            const std::string& ip, // 브로커 IP 주소
            unsigned short port); // 브로커 포트 번호

        void set_connection_timeout( // 브로커 연결 타임아웃 설정
            std::chrono::milliseconds timeout); // 타임아웃 시간 (밀리초 단위)

        bool connect(std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60)); // 브로커에 연결 시도 (성공 시 true, 실패 시 false 반환)
        void disconnect(); // 브로커와의 연결을 정상적으로 종료

        std::pair<rpc_status, json> call_raw( // RPC 호출 함수
            std::string_view service_name, // 호출할 서비스 이름
            const json& raw_argument, // 서비스에 전달할 인자 JSON
            std::chrono::seconds timeout); // RPC 호출 타임아웃 (초 단위, 기본값 5초)

    };

}
