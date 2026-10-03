#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>
#include <iomanip>
#include <sstream>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network_openssl/tls/tls_client.hpp"

int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mnt = mino::network_openssl::tls;
    namespace mclt = mino::core::log::tinylog;

    using mnsock = mn::sock;
    using tls_client = mnt::tls_client;
    using logger = mclt::logger;
    using log_level = mclt::log_level;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;

    mnsock sock_initializer; // 소켓 초기화

    // 콘솔 싱크 및 로거 인스턴스 구성
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
    client.set_verify_peer(false); // 인증서 검증 여부 (자체 서명 인증서 허용)

    // Ctrl+C 또는 시그널 수신 시 안전하게 정지
    auto& term_handler = mino::core::daemon::termination_handler::get_instance();
    term_handler.initialize();
    term_handler.set_callback([&client]() {
        client.stop();
        std::exit(0);
        });

    client.set_on_connect([&logger_instance]() {
        logger_instance->info("<bright_green>[Client Callback] Connected securely via TLS!</bright_green>");
        });

    client.set_on_receive([&logger_instance](const std::string& data) {
        // 개행 정리 후 출력
        std::string clean_data = data;
        while (!clean_data.empty() && (clean_data.back() == '\r' || clean_data.back() == '\n')) {
            clean_data.pop_back();
        }
        logger_instance->info("[Client Callback] Server Reply: <pink>{}</pink>", clean_data);
        });

    client.set_on_close([&logger_instance]() {
        logger_instance->info("<bright_yellow>[Client Callback] Disconnected.</bright_yellow>");
        });

    // 지수 백오프 재연결 설정: 10초 시작, 최대 60초 상한선, 60초 도달 후 무제한 계속 재시도 (max_retries = 0)
    mnt::reconnect_config recon_cfg;
    recon_cfg.initial_interval = std::chrono::seconds(10);
    recon_cfg.max_interval = std::chrono::seconds(60);
    recon_cfg.max_retries = 0; // 0: 무한 재시도
    // recon_cfg.max_retries = 5; // 최대 5회까지 연결 실패 시 종료함
    recon_cfg.backoff_multiplier = 2.0; // 재연결 시도 시간 간격: 10s -> 20s -> 40s -> 60s(max) -> 60s -> ...

    if (!client.start(recon_cfg)) {
        logger_instance->error("Failed to start TLS client");
        return -1;
    }

    logger_instance->info("TLS client is running. Observing reconnection loop (Press Ctrl+C to terminate)...");

    // ----------------------------------------------------
    // 메인 루프 (연결 상태 모니터링 및 주기적 데이터 전송)
    // ----------------------------------------------------
    bool isLoop = true;
    while (isLoop) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (client.is_connected()) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &now_c);
#else
            localtime_r(&now_c, &tm);
#endif
            std::ostringstream oss;
            oss << "Hello Secure Tinylog TLS World! " << std::put_time(&tm, "%H:%M:%S");

            if (client.send_data(oss.str()) > 0) {
                logger_instance->info("Sent: {}", oss.str());
            }
            else {
                logger_instance->error("Failed to send TLS data.");
            }
        } // if (client.is_connected()) ...

        bool isRunning = client.is_running();
        if (!isRunning) {
            logger_instance->warn("TLS client thread is not running. Exit loop...");
            break; // exit loop
        }

    }

    client.close_connection();

    return 0;
}
