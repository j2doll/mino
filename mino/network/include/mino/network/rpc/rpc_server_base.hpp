#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <chrono>

#include <nlohmann/json_fwd.hpp>

#include <spdlog/spdlog.h>

#include "mino/network/message_broker/message_broker.hpp"

#include "mino/network/rpc/rpc_protocol_util.hpp"

namespace mino::network::rpc {

    // RPC 기반 클래스
    class rpc_server_base {
    public:
        using json = nlohmann::json;
        using raw_handler = std::function<json(const json&)>;

    protected:
        mino::network::message_broker::publisher pub; // Publisher 멤버 변수
        mino::network::message_broker::subscriber sub; // Subscriber 멤버 변수
        std::unordered_map<std::string, raw_handler> service_registry; // 서비스 이름과 핸들러 함수를 매핑하는 레지스트리
        std::vector<std::string> service_topics; // 서비스 이름에 대응하는 구독 주제 목록
        std::mutex registry_mutex; // 서비스 레지스트리에 대한 동시 접근을 제어하는 뮤텍스
        std::shared_ptr<spdlog::logger> logger; // 로거 멤버 변수
        std::chrono::milliseconds startup_timeout; // 서버 시작 시 브로커 연결 대기 시간

    protected:
        void on_request_received( // 메시지 수신 시 호출되는 콜백 함수
            std::string_view topic, // 메시지가 속한 주제
            std::string_view msg_kind, // 메시지의 세부 종류
            std::string_view body, // 메시지 본문 (직렬화된 JSON 문자열)
            uint64_t timestamp); // 메세지 생성 시간의 타임스탬프 (밀리초 단위)

    public:
        rpc_server_base();
        virtual ~rpc_server_base() = default;

        void register_raw_service( // 서비스 등록 함수
            const std::string& name, // 서비스 이름
            raw_handler handler); // 서비스 이름과 핸들러 함수를 매핑하여 레지스트리에 등록

        void setup_logger(std::shared_ptr<spdlog::logger> custom_logger);  

        void set_broker( // 브로커 설정 
            const std::string& ip, // 브로커 IP 주소
            unsigned short port); // 브로커 포트 번호

        void set_startup_timeout( // 서버 시작 시 브로커 연결 대기 시간 설정
            std::chrono::milliseconds timeout); // 대기 시간 (밀리초 단위)

        bool start(std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60)); // RPC 서버 시작
        void stop(); // RPC 서버 중지
    };

} 