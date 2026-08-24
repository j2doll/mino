#include <thread>
#include <iostream>
#include <cassert>

#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/crash_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/message_broker/publisher.hpp"

void clean_up_resources(mino::network::message_broker::publisher* publisher)
{
    publisher->disconnect();
    std::cerr << "Publisher resources cleaned up successfully." << std::endl;
}

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    namespace mcs = mino::core::system;
    using crash_handler = mcs::crash_handler;

    crash_handler::initialize([](const std::string& log_message) {
        std::cerr << "\n[User Callback] Crash Detected! Reporting to console...\n";
        std::cerr << log_message << std::endl;
    });

    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize();

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto pub_console_sink = std::make_shared<mclt::console_sink>("pub_console");
    assert(pub_console_sink);
    auto pub_logger = std::make_shared<mclt::logger>("pub_logger");
    assert(pub_logger);
    pub_logger->add_sink(pub_console_sink);
    mclt::logger::register_logger(pub_logger);

    namespace mnmb = mino::network::message_broker;
    using publisher = mnmb::publisher;
    publisher pub(pub_logger);

    handler.set_callback([&pub]() {
        clean_up_resources(&pub);
        std::exit(0);
    });

    pub.set_broker("127.0.0.1", 54321);

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!pub.connect(tcp_sleep_time)) { // broker 와 연결 시도
        pub_logger->critical("{}", "메시지 브로커에 연결 실패");
        return -1;
    }

    pub_logger->info("{}", "메인 루핑 시작");

    int loop_count = 1;
    bool looping = true;
    while (looping) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait a moment

        if (!pub.is_connected()) {
            continue;
        }

        std::string topic1 = "sports";
        std::string kind1 = "alert";
        std::string message1
            = std::string("[UTF-8 한글] Goal Scored! Count: ") + std::to_string(loop_count);
        auto ret_sports = pub.publish(topic1, kind1, message1); // 발행(publish) 시도

        if (!ret_sports.first) {
            std::string error_msg = ret_sports.second.empty() ? "Unknown error" : ret_sports.second;
            pub_logger->error(
                "발행(pub) <bright_yellow>실패</bright_yellow>:"
                " topic: <pink>{}</pink>,"
                " kind: <gray>{}</gray>,"
                " error: <bright_yellow>{}</bright_yellow>",
                topic1, kind1, error_msg);
        }
        else {
            pub_logger->info(
                "발행(pub) <green>성공</green>:"
                " topic: <pink>{}</pink>,"
                " kind: <gray>{}</gray>,"
                " message: <magenta>{}</magenta>",
                topic1, kind1, message1);
        }

        loop_count++;
    }

    pub.disconnect(); // broker 와의 연결 해제

    return 0;
}
