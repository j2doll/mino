#include <iostream>
#include <mutex>
#include <memory>

#include "mino/core/datetime/datetime.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/crash_handler.hpp"

#include "mino/external/log/spd/auto_color_sink.hpp"

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

    namespace mels = mino::external::log::spd;
    auto sub_console_sink = std::make_shared<mels::auto_color_sink<std::mutex>>();
    std::vector<spdlog::sink_ptr> sinks{ sub_console_sink };
    auto sub_logger = std::make_shared<spdlog::logger>("sub_logger", sinks.begin(), sinks.end());

    namespace mnmb = mino::network::message_broker;
    using subscriber = mnmb::subscriber;
    subscriber sub(sub_logger);
    // sub.set_logger(sub_logger);

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

            // 포맷 리터럴을 그대로 사용하고, 변환된 값들을 포맷 인자로 전달
            sub_logger->info(
                "[KST] {0} | Topic: <pink>{1}</pink> "
                "| Kind: <grey>{2}</grey> -> Body: <purple>{3}</purple>",
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
        sub_logger->error("{0}", "브러커 연결 실패.");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    sub_logger->info("{0}", "[Main Thread] 리스닝이 동적 작동 중. 엔터 키 입력시 종료.");
    std::cin.get();

    {
        std::scoped_lock lock(g_resource_mutex);
        std::string msg = std::string("[Main Thread] 종료 중. 처리된 메시지 수: ")
            + std::to_string(g_total_messages_received);
        sub_logger->info("{0}", msg);
    }

    sub.disconnect();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
