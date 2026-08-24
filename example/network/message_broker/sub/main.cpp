#include <iostream>
#include <mutex>
#include <memory>
#include <cassert>

#include "mino/core/datetime/datetime.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/crash_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/string.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/message_broker/subscriber.hpp"

int g_total_messages_received = 0; // 테스트를 위한 수신 메시지 갯수 카운터
std::mutex g_resource_mutex; // g_total_messages_received 접근 보호를 위한 mutex

// 비정상 종료(Ctrl+C 등) 시 호출되는 함수 등록
void clean_up_resources(mino::network::message_broker::subscriber* sub)
{
    sub->disconnect();
    std::cerr << "Subscriber resources cleaned up successfully." << std::endl;
}

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    namespace mcs = mino::core::system;
    mcs::crash_handler::initialize(
        [](const std::string& log_message) { // 크래시 발생 시 호출되는 사용자 정의 콜백
            std::cout
                << std::endl
                << "[User Callback] Crash Detected! Reporting to console..."
                << std::endl;
            std::cout << log_message << std::endl;
        }
    );

    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize(); // 종료 핸들러 초기화 및 브로커 종료 로직 등록

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto sub_console_sink = std::make_shared<mclt::console_sink>("sub_console");
    assert(sub_console_sink);
    auto sub_logger = std::make_shared<mclt::logger>("sub_logger");
    assert(sub_logger);
    sub_logger->add_sink(sub_console_sink);
    mclt::logger::register_logger(sub_logger);

    namespace mnmb = mino::network::message_broker;
    using subscriber = mnmb::subscriber;
    subscriber sub(sub_logger); // subscriber 객체 

    // 비정상 종료(Ctrl+C 등) 시 호출되는 함수 등록 
    handler.set_callback([&sub]() { 
        clean_up_resources(&sub);
        std::exit(0);
    });

    // 메시지 수신 시 호출되는 콜백 함수 설정
    sub.set_on_message_handler(
        [sub_logger](
            std::string_view topic, // 토픽 
            std::string_view msg_kind, // 종류. 종류는 필수 자료는 아님.
            std::string_view body, // 본문
            uint64_t timestamp) // 1970-01-01 00:00:00 UTC 이후 경과된 밀리초 단위 시간
        {
            // arg0: YYYY-MM-DD HH:MM:SS.mmm 형식 문자열 (local time zone)
            std::string arg0 = mino::core::datetime::util::format_datetime(timestamp);
            std::string arg1 = std::string(topic);
            std::string arg2 = std::string(msg_kind);
            std::string arg3 = std::string(body);

            sub_logger->info(
                "[KST] {} | "
                "Topic: <pink>{}</pink> | "
                "Kind: <gray>{}</gray> -> "
                "Body: <magenta>{}</magenta>",
                arg0, arg1, arg2, arg3);

            {
                std::scoped_lock lock(g_resource_mutex); // g_total_messages_received 접근 보호
                g_total_messages_received++;
            }
        });

    sub.set_broker("127.0.0.1", 54321); // broker IP와 포트 설정
    sub.set_topic({ "sports" }); // 구독할 토픽 설정. 토픽은 복수 개를 등록도 가능.

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!sub.connect(tcp_sleep_time)) { // broker 와 연결 시도
        sub_logger->error("{}", "브러커 연결 실패.");
        return -1;
    }

    sub_logger->info("{}", "[Main Thread] 리스닝이 동적 작동 중. 엔터 키 입력시 종료.");
    std::cin.get(); // 키 입력 대기
    // NOTE: 키 입력 대신, loop 처리를 추가하여 종료 조건을 구현할 수도 있습니다.

    {
        std::scoped_lock lock(g_resource_mutex); // g_total_messages_received 접근 보호
        std::string msg =
            std::string("[Main Thread] 종료 중. 처리된 메시지 수: ") +
            std::to_string(g_total_messages_received);
        sub_logger->info("{}", msg);
    }

    sub.disconnect(); // broker와 연결 종료

    return 0;
}
