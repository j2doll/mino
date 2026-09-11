#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ssh2/ssh2.hpp"

//
// main() 을 실행하기 전에 다음을 수행한다.
// pip install paramiko
// ssh-keygen -t rsa -b 2048 -f server_key -N ""
// python ssh_server.py
//
int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mclt = mino::core::log::tinylog;
    namespace mns2 = mino::network::ssh2;

    using mnsock = mn::sock;
    using logger = mclt::logger;
    using log_level = mclt::log_level;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;

    mnsock sock_initializer; // 소켓 초기화

    console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = encoding_type::cp949;
    console_cfg.eol = eol_type::crlf;
#else
    console_cfg.encoding = encoding_type::utf8;
    console_cfg.eol = eol_type::lf;
#endif
    auto console_sink_instance = std::make_shared<console_sink>("console_sink", console_cfg);

    // 1. tinylog 로거 생성 및 콘솔 싱크 등록
    auto logger_instance = std::make_shared<logger>("ssh2_client");
    logger_instance->add_sink(console_sink_instance);
    logger_instance->set_level(log_level::debug);

    // 2. ssh_client 인스턴스 초기화 및 로거 연동
    mns2::ssh_client client;
    client.set_logger(logger_instance);

    // 3. 계정 인증 정보 설정
    mns2::ssh_credentials creds;
    creds.type = mns2::auth_type::password;
    creds.username = "admin";
    creds.password = "secret123";
    client.set_credentials(creds);

    // 4. (선택) 서버 호스트 키 핑거프린트 사전 검증 설정
    // client.set_expected_fingerprint("uD8v1XJ...=", mns2::fingerprint_type::sha256);

    // 5. 콜백 등록 (연결, 수신, 단절, 에러)
    client.set_on_connect([&logger_instance, &client]() {
        logger_instance->info("[callback] SSH Server connection established!");

        // 서버 핑거프린트 확인
        std::string fp = client.get_server_fingerprint_base64(mns2::fingerprint_type::sha256);
        logger_instance->info("[callback] Verified Host Fingerprint: SHA256:{}", fp);
        });

    client.set_on_receive([&logger_instance](const std::string& data) {
        logger_instance->info("[callback] Received payload from server: {}", data);
        });

    client.set_on_disconnect([&logger_instance](const std::string& reason) {
        logger_instance->warn("[callback] SSH Server disconnected. Reason: {}", reason);
        });

    client.set_on_error([&logger_instance](int error_code, const std::string& message) {
        logger_instance->error("[callback] SSH client error occurred [code: {}]: {}", error_code, message);
        });

    // 6. 지수 백오프 기반 재연결 정책 설정
    mns2::reconnect_config r_cfg;
    r_cfg.initial_interval = std::chrono::milliseconds(1000); // 첫 대기: 1초
    r_cfg.backoff_multiplier = 2.0;                          // 실패 시 2배씩 증가
    r_cfg.max_interval = std::chrono::milliseconds(15000);    // 최대 대기 상한선: 15초
    r_cfg.max_duration = std::nullopt;                       // 무한대 재연결 시도

    // 7. 백그라운드 자동 재연결 및 데이터 수신 워커 시작
    const std::string target_ip = "127.0.0.1";
    const unsigned short target_port = 2222;

    logger_instance->info("Starting auto-reconnect worker targeting {}:{}", target_ip, target_port);
    client.start_auto_reconnect(target_ip, target_port, r_cfg);

    // 8. 주기적 JSON 전송 및 통신 루프
    int sequence_id = 0;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        if (client.is_connected()) {
            std::string json_payload = "{\"seq\":" + std::to_string(++sequence_id) +
                ",\"status\":\"active\",\"data\":\"sample_telemetry\"}";

            logger_instance->debug("Sending JSON message: {}", json_payload);
            if (!client.send_json(json_payload)) {
                logger_instance->error("Failed to send JSON message");
            }
        }
    }

    // 9. 종료 처리
    logger_instance->info("Shutting down SSH client...");
    client.stop_auto_reconnect();

    return 0;
}
