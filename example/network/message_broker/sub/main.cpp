#include <iostream>
#include <mutex>
#include <memory>

#include "mino/core/datetime/datetime.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/crash_handler.hpp"

#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/message_broker/subscriber.hpp"

int g_total_messages_received = 0;
std::mutex g_resource_mutex;

void clean_up_resources(mino::network::message_broker::subscriber* sub)
{
    sub->disconnect();
    std::cerr << "Subscriber resources cleaned up successfully." << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    namespace mcs = mino::core::system;
    mcs::crash_handler::initialize([](const std::string& log_message) {
        std::cout
            << std::endl
            << "[User Callback] Crash Detected! Reporting to console..."
            << std::endl;
        std::cout << log_message << std::endl;
        });

    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize();

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto sub_console_sink = std::make_shared<mclt::console_sink>("sub_console");
    auto sub_logger = std::make_shared<mclt::logger>("sub_logger");
    sub_logger->add_sink(sub_console_sink);
    mclt::logger::register_logger(sub_logger);

    namespace mnmb = mino::network::message_broker;
    using subscriber = mnmb::subscriber;
    subscriber sub(sub_logger);

    handler.set_callback([&sub]() {
        clean_up_resources(&sub);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
        });

    sub.set_on_message_handler(
        [sub_logger](
            std::string_view topic,
            std::string_view msg_kind,
            std::string_view body,
            uint64_t timestamp)
        {
            std::string arg0 = mino::core::datetime::util::format_datetime(timestamp);
            std::string arg1 = std::string(topic);
            std::string arg2 = std::string(msg_kind);
            std::string arg3 = std::string(body);

            sub_logger->info(
                "[KST] {} | Topic: <pink>{}</pink> "
                "| Kind: <gray>{}</gray> -> Body: <magenta>{}</magenta>",
                arg0, arg1, arg2, arg3);

            {
                std::scoped_lock lock(g_resource_mutex);
                g_total_messages_received++;
            }
        });

    sub.set_broker("127.0.0.1", 54321);
    sub.set_topic({ "sports" });

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!sub.connect(tcp_sleep_time)) {
        sub_logger->error("{}", "브러커 연결 실패.");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    sub_logger->info("{}", "[Main Thread] 리스닝이 동적 작동 중. 엔터 키 입력시 종료.");
    std::cin.get();

    {
        std::scoped_lock lock(g_resource_mutex);
        std::string msg = std::string("[Main Thread] 종료 중. 처리된 메시지 수: ")
            + std::to_string(g_total_messages_received);
        sub_logger->info("{}", msg);
    }

    sub.disconnect();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
