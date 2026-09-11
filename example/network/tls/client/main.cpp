#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/tls/tls_client.hpp"

int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mnt = mino::network::tls;
    namespace mclt = mino::core::log::tinylog;

    using mnsock = mn::sock;
    using tls_client = mnt::tls_client;
    using logger = mclt::logger;
    using log_level = mclt::log_level;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;

    mnsock sock_initializer; // 소켓 포기화

    // 콘솔 싱크 및 로거 인스턴스 구성
console_cfg_init:
    console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = encoding_type::cp949;
    console_cfg.eol = eol_type::crlf;
#else
    console_cfg.encoding = encoding_type::utf8;
    console_cfg.eol = eol_type::lf;
#endif
    auto console_sink_instance = std::make_shared<console_sink>("console_sink", console_cfg);

    auto logger_instance = std::make_shared<logger>("TLS_CLIENT");
    logger_instance->add_sink(console_sink_instance);
    logger_instance->set_level(log_level::trace);

    logger::register_logger(logger_instance);

    logger_instance->info("<bold><bright_cyan>=== TLS Client Execution Demo ===</bright_cyan></bold>");

    // tls server 및 tls client를 위한 IP, 포트 설정
    const auto server_ip = std::string("127.0.0.1");
    const auto server_port = static_cast<unsigned short>(9443);

    // ----------------------------------------------------
    // TLS 클라이언트 초기화 및 구동
    // ----------------------------------------------------
    tls_client client;
    client.set_logger(logger_instance);
    client.set_server(server_ip, server_port);
    client.set_verify_peer(false); // 인증서 검증 여부: false로 설정하면 자체 서명 인증서도 허용
    // client.set_ca_cert("ca.crt"); // 클라이언트 측에 자체 서명 인증서 설정. mTLS 서버에서 자체 서명 인증서를 사용하는 경우 필요. 

    ////////////////////////////////////////////////
    // 클라이언트에서 인증서 검증 
    // ////////////////////////////////////////////////
    // // (1) 사설 CA 또는 자체 생성 CA를 사용할 때
    // client.set_verify_peer(true); // 1. 피어 검증 활성화
    // client.set_ca_cert("ca.crt"); // 2. 서버 인증서를 서명한 루트/중간 CA 인증서(PEM) 경로 지정
    // client.set_sni_hostname("myserver.local"); // 3. 서버 인증서의 CN/SAN에 등록된 호스트명 지정 (호스트명 불일치 방지)
    // client.set_server("127.0.0.1", 9443);
    // client.start();
    // 
    ////////////////////////////////////////////////
    // // (2) Let's Encrypt 등 공인 CA 인증서를 사용할 때
    // client.set_verify_peer(true);
    // // ca_file을 지정하지 않으면 OS의 시스템 기본 신뢰 저장소(Default Verify Paths)를 조회합니다.
    // // (Linux의 /etc/ssl/certs, Windows의 기본 CA 등)
    // client.set_ca_cert(""); 
    // client.set_sni_hostname("api.example.com");
    // client.set_server("api.example.com", 443);
    // client.start();
    ////////////////////////////////////////////////

    client.set_on_connect([&logger_instance]() {
        logger_instance->info("<bright_green>[Client Callback] Connected securely via TLS!</bright_green>");
        });

    client.set_on_receive([&logger_instance](const std::string& data) {
        logger_instance->info("[Client Callback] Server Reply: <pink>{}</pink>", data);
        });

    client.set_on_close([&logger_instance]() {
        logger_instance->info("[Client Callback] Disconnected.");
        });

    auto retry_interval = std::chrono::seconds(1);
    client.start(retry_interval);

    // 연결 수립 대기 (최대 3초)
    for (auto attempt = 0; attempt < 30 && !client.is_connected(); ++attempt) {
        auto wait_tick = std::chrono::milliseconds(100);
        std::this_thread::sleep_for(wait_tick);
    }

    // ----------------------------------------------------
    // 보안 데이터 전송 테스트
    // ----------------------------------------------------
    if (client.is_connected()) {
        auto payload = std::string("Hello Secure Tinylog TLS World!");
        client.send_data(payload);
    }
    else {
        std::cerr << "Client failed to connect to server. Exiting demo." << std::endl;
        return -1;
    }

    // 응답 수신 대기
    auto response_wait_time = std::chrono::milliseconds(500);
    std::this_thread::sleep_for(response_wait_time);

    // ----------------------------------------------------
    // 리소스 정리 및 정상 종료
    // ----------------------------------------------------
    logger_instance->info("Shutting down client...");
    client.stop();

    logger_instance->info("<bright_cyan>Demo completed successfully.</bright_cyan>");
    return 0;
}
