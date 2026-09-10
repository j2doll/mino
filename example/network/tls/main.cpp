#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/tls/tls_client.hpp"
#include "mino/network/tls/tls_server.hpp"

int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mnt = mino::network::tls;
    namespace mclt = mino::core::log::tinylog;

    using mnsock = mn::sock;
    using tls_server = mnt::tls_server;
    using tls_client = mnt::tls_client;
    using logger = mclt::logger;
    using log_level = mclt::log_level;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;

    mnsock sock_initializer; // 소켓 포기화

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

    auto logger_instance = std::make_shared<logger>("TLS_DEMO");
    logger_instance->add_sink(console_sink_instance);
    logger_instance->set_level(log_level::trace);

    logger::register_logger(logger_instance);

    logger_instance->info("<bold><bright_cyan>=== TLS Server / Client Execution Demo ===</bright_cyan></bold>");

    // tls server 및 tls client를 위한 IP, 포트, 인증서 경로 설정
    const auto server_ip = std::string("127.0.0.1");
    const auto server_port = static_cast<unsigned short>(9443);

    std::string cmake_path = CMAKE_CURRENT_PATH;
    std::filesystem::path cert_path = std::filesystem::path(cmake_path) / "server.crt";
    std::filesystem::path key_path = std::filesystem::path(cmake_path) / "server.key";
    // 인증서는 다음 명령으로 생성 가능함.
    // 
    // openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/CN=localhost"
    // 
    // openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "/CN=localhost" -config "D:\vcpkg\installed\x64-windows\Program Files\Common Files\SSL\openssl.cnf"
    // 

    // ----------------------------------------------------
    // TLS 서버 초기화 및 구동
    // ----------------------------------------------------
    tls_server server;
    server.set_logger(logger_instance);

    if (!server.set_certificate_and_key(cert_path.string(), key_path.string())) {
        logger_instance->error("Failed to load server cert/key. Check server.crt and server.key files.");
        return -1;
    }

    server.set_on_connect_callback([&logger_instance](socket_t fd, const std::string& msg) {
        logger_instance->info("[Server Callback] Client fd: <bright_yellow>{}</bright_yellow> connected ({})", fd, msg);
        });

    server.set_on_receive_callback([&server, &logger_instance](socket_t fd, const std::string& data) {
        logger_instance->info("[Server Callback] Echoing back: <bright_green>{}</bright_green>", data);
        server.send_to_client(fd, "ACK: " + data);
        });

    server.set_on_close_callback([&logger_instance](socket_t fd, const std::string& reason) {
        logger_instance->info("[Server Callback] Client fd: {} closed ({})", fd, reason);
        });

    auto server_start_res = server.start(server_ip, server_port);
    if (server_start_res != tls_server::start_result::success) {
        logger_instance->error("Server failed to start on {}:{}", server_ip, server_port);
        return -1;
    }
    logger_instance->info("Server listening on <bright_white>{}:{}</bright_white>", server_ip, server_port);

    // ----------------------------------------------------
    // TLS 클라이언트 초기화 및 구동
    // ----------------------------------------------------
    tls_client client;
    client.set_logger(logger_instance);
    client.set_server(server_ip, server_port);
    client.set_verify_peer(false); // 자체 서명 인증서 테스트용

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

    // 응답 수신 대기
    auto response_wait_time = std::chrono::milliseconds(500);
    std::this_thread::sleep_for(response_wait_time);

    // ----------------------------------------------------
    // 리소스 정리 및 정상 종료
    // ----------------------------------------------------
    logger_instance->info("Shutting down client and server...");
    client.stop();
    server.quit();

    logger_instance->info("<bright_cyan>Demo completed successfully.</bright_cyan>");
    return 0;
}
