#include <thread>
#include <iostream>
#include <cassert>

#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/crash_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/message_broker/publisher.hpp"

#include "mino/core/reflect/reflect.hpp"
#include "../reflect-sample.hpp"
#include "mino/core/encoding/encoding.hpp"

void clean_up_resources(mino::network::message_broker::publisher* publisher)
{
    publisher->disconnect();
    std::cerr << "Publisher resources cleaned up successfully." << std::endl;
}

int main(int argc, char* argv[]) {
    mino::network::sock mnsock; // 소켓 초기화

    // 크래시 핸들러 초기화
    namespace mcs = mino::core::system;
    using crash_handler = mcs::crash_handler;
    crash_handler::initialize([](const std::string& log_message) {
        std::cerr << "\n[User Callback] Crash Detected! Reporting to console...\n";
        std::cerr << log_message << std::endl;
        });

    // 로깅 설정
    namespace mclt = mino::core::log::tinylog;
    auto pub_console_sink = std::make_shared<mclt::console_sink>("pub_console");
    assert(pub_console_sink);
    auto pub_logger = std::make_shared<mclt::logger>("pub_logger");
    assert(pub_logger);
    pub_logger->add_sink(pub_console_sink);
    mclt::logger::register_logger(pub_logger);

    namespace mnmb = mino::network::message_broker;
    using publisher = mnmb::publisher;
    publisher pub(pub_logger); // publisher 객체

    // 종료 시그널(Ctrl+C) 처리기 초기화
    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize();
    handler.set_callback([&pub]() {
        clean_up_resources(&pub);
        std::exit(0);
    });

    // broker에 연결 시도
    pub.set_broker("127.0.0.1", 24321); 
    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!pub.connect(tcp_sleep_time)) { 
        pub_logger->critical("{}", "메시지 브로커에 연결 실패");
        return -1;
    }

    bool looping = true;
    pub_logger->info("{}", "메인 루핑 시작");
    while (looping) {
        std::this_thread::sleep_for(std::chrono::seconds(2));  

        if (!pub.is_connected()) {
            continue;
        }

        // reflect가 적용된 구조체
        point pt;
        pt.x = 100.1;
        pt.y = 200.2;
        pub_logger->info("[point] (<pink>{}</pink>, <pink>{}</pink>)", pt.x, pt.y);

        namespace mcr = mino::core::reflect;
        namespace mce = mino::core::encoding;
        using binary_writer = mcr::binary_writer;

        binary_writer writer;
        try {
            writer(pt);
        } catch (const std::bad_alloc& e) {
            std::cerr << "Failed to serialize: " << e.what() << std::endl;
            return 1;
        }
        auto pub_buffer = writer.get_buffer();
        auto pub_buffer_string = mce::base64_encode(pub_buffer);

        std::string topic1 = "point"; 
        std::string kind1 = "reflect";
        std::string message1 = pub_buffer_string;
        auto ret_sports = pub.publish(topic1, kind1, message1); 

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
    }

    pub.disconnect();
    return 0;
}
