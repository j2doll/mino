#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <cassert>

#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/network/mqtt/mqtt_client.hpp"

// main() 구동 전에 다음을 실행한다.
//  python mqtt_broker.py
int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mclt = mino::core::log::tinylog;
    namespace mnm = mino::network::mqtt;

    using log_level = mclt::log_level;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;
    using logger = mclt::logger;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using mnsock = mn::sock;

    mnsock sock_initializer;

    console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = encoding_type::cp949;
    console_cfg.eol = eol_type::crlf;
#else
    console_cfg.encoding = encoding_type::utf8;
    console_cfg.eol = eol_type::lf;
#endif
    auto console = std::make_shared<console_sink>("console", console_cfg);
    auto app_logger = std::make_shared<logger>("sub_app");
    app_logger->add_sink(console);
    app_logger->set_level(log_level::debug);

    mnm::mqtt_client client;
    client.set_logger(app_logger)
          .set_broker("127.0.0.1", 1883);

    client.set_client_id("sub_client_1");

    client.set_keep_alive(10) // Keep Alive 메시지 송신 주기
        .set_reconnect_backoff(std::chrono::seconds(5), std::chrono::seconds(60), 2.0) // 재연결 지수 백오프: 5초부터 시작하여 2배씩 증가, 최대 60초 상한 (5 -> 10 -> 20 -> 40 -> 60 -> 60 -> ...)
        .set_max_reconnect_duration(mnm::mqtt_client::infinite_reconnect); // 최대 재연결 시도 시간: 무한대 재시도 (또는 std::chrono::seconds(60) 처럼 지정 가능)

    client.on_message([app_logger](std::string_view topic, std::string_view payload) {
            auto msg =
                std::string("[MQTT SUB] Received message on topic: ") +
                std::string("<yellow>") + std::string(topic) + std::string("</yellow>") +
                std::string(", payload: ") +
                std::string("<bright_yellow>") + std::string(payload) + std::string("</bright_yellow>");
            app_logger->log(log_level::info, msg);
        });

    // MQTT 계정 인증 설정 (사용자명, 비밀번호)
    // client.set_credentials("admin_user", "secret_pass_1234") 

    client.start(std::chrono::seconds(1));

    // 브로커 핸드셰이크(CONNACK) 완료 대기
    // 로컬 브로커 (127.0.0.1) 핸드세이크는 100ms ~ 200ms (0.1 ~ 0.2초) 정도 소요 예상.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!client.is_connected()) {
        app_logger->log(log_level::err,
            "[MQTT SUB] MQTT client <red>failed</red> to connect to broker!");
        return -1;
    }

    std::string topic_name = "test/topic";
    bool ret_sub = client.subscribe(topic_name);
    if (ret_sub) {
        app_logger->log(log_level::info,
            "[MQTT SUB] <green>Successfully</green> subscribed to topic: <cyan>{}</cyan>", topic_name);
    }
    else {
        app_logger->log(log_level::err,
            "[MQTT SUB] <red>Failed</red> to subscribe to topic: <cyan>{}</cyan>", topic_name);
        return -1;
    }

    // main loop: 메시지 수신 대기
    bool isLoop = true;
    while(isLoop) {
        if (!client.is_connected()) {
            app_logger->log(log_level::err,
                "[MQTT SUB] MQTT client <red>failed</red> to connect to broker!");
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 잠시 대기
    }
    
    // [중요] 마지막 메시지가 브로커를 거쳐 전달될 시간(1초)을 확보한 후 종료
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client.stop();

    app_logger->log(log_level::info, "[MQTT SUB] Terminated.");
    return 0;
}
