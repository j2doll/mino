#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <cassert>

#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/network/mqtt/mqtt_client.hpp"

// main() 구동 전에 다음을 실행한다.
//  python mqtt_broker.py
//  python subscriber.py 
int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mclt = mino::core::log::tinylog;
    namespace mnm = mino::network::mqtt;

    mn::sock sock_initializer;

    mclt::console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = mclt::encoding_type::cp949;
    console_cfg.eol = mclt::eol_type::crlf;
#else
    console_cfg.encoding = mclt::encoding_type::utf8;
    console_cfg.eol = mclt::eol_type::lf;
#endif
    auto console = std::make_shared<mclt::console_sink>("console", console_cfg);
    auto app_logger = std::make_shared<mclt::logger>("app");
    app_logger->add_sink(console);
    app_logger->set_level(mclt::log_level::debug);

    mnm::mqtt_client client;
    client.set_logger(app_logger)
        .set_broker("127.0.0.1", 1883)
        .set_client_id("logged_client")
        .set_keep_alive(10) // Keep Alive 메시지 송신 주기
        .set_reconnect_backoff(std::chrono::seconds(1), std::chrono::seconds(10), 2.0) // 재연결 지수 백오프: 1초부터 시작하여 2배씩 증가, 최대 10초 상한
        .set_max_reconnect_duration(mnm::mqtt_client::infinite_reconnect); // 최대 재연결 시도 시간: 무한대 재시도 (또는 std::chrono::seconds(60) 처럼 지정 가능)

    // MQTT 계정 인증 설정 (사용자명, 비밀번호)
    // client.set_credentials("admin_user", "secret_pass_1234") 

    client.start(std::chrono::seconds(1));

    // 브로커 핸드셰이크(CONNACK) 완료 대기
    // 로컬 브로커 (127.0.0.1) 핸드세이크는 100ms ~ 200ms (0.1 ~ 0.2초) 정도 소요 예상.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!client.is_connected()) {
        std::cerr << "MQTT client failed to connect to broker!" << std::endl;
        return -1;
    }

    // 5회 반복 발행 테스트 (subscriber.py에서 실시간 수신 확인)
    for (int i = 1; i <= 5; ++i) {
        std::string msg = "Message from main.cpp [seq: " + std::to_string(i) + "]";

        bool ok = client.publish("test/topic", msg);
        if (ok) {
            std::cout << "Successfully published: " << msg << std::endl;
        }
        else {
            std::cerr << "Publish failed!" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2)); // 잠시 대기 필요
        // NOTE: 수천 건 이상의 초고속 루프 전송 시: while 문 등으로 1초에 수천~수만 건을 딜레이 없이 쏘아 올리면 브로커(특히 amqtt 같은 경량 브로커)의 이벤트 루프가 밀려 지연이 발생하거나 TCP 버퍼가 가득 찰 수 있습니다. 이런 대량 전송 상황에서는 루프 사이에 1ms ~ 10ms 정도의 짧은 양보(Yield/Sleep)를 두는 것이 브로커 부하 분산에 유리.
    }

    // [중요] 마지막 메시지가 브로커를 거쳐 전달될 시간(1초)을 확보한 후 종료
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client.stop();

    std::cout << "Terminated.\n";
    return 0;
}
