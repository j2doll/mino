#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/tls/tls_server.hpp"

//
// main()을 구동하기 전,
// 현재 소스코드가 있는 경로에 openssl로
// 사설 인증서 파일을 생성하여야 함.
// 
// openssl req -x509 -newkey rsa:2048 \
//  -keyout server.key -out server.crt \
//  -days 365 -nodes -subj "/CN=localhost"
int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mnt = mino::network::tls;
    namespace mclt = mino::core::log::tinylog;

    using mnsock = mn::sock;
    using tls_server = mnt::tls_server;
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

    auto logger_instance = std::make_shared<logger>("TLS_SERVER");
    logger_instance->add_sink(console_sink_instance);
    logger_instance->set_level(log_level::trace);

    logger::register_logger(logger_instance);

    logger_instance->info("<bold><bright_cyan>=== TLS Server Execution Demo ===</bright_cyan></bold>");

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

    ////////////////////////////////////////////////
    // 1) 루트 CA 생성
    // openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 365 -subj "/CN=MyRootCA"
    ////////////////////////////////////////////////
    // 2) 서버 개인키 및 CSR 생성
    // openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr -subj "/CN=localhost"
    ////////////////////////////////////////////////
    // 3) SAN 확장 설정을 포함하여 CA로 서명 (server.crt 생성)
    // openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 \
    //   -extfile <(printf "subjectAltName=DNS:localhost,IP:127.0.0.1")
    ////////////////////////////////////////////////

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
        logger_instance->info("[Server Callback] Client fd: <bright_yellow>{}</bright_yellow> <gray>connected</gray> ({})", fd, msg);
        });

    server.set_on_receive_callback([&server, &logger_instance](socket_t fd, const std::string& data) {
        // 클라이이언트에서 메시지 수신 시, 에코로 응답
        logger_instance->info("[Server Callback] Echoing back: <bright_green>{}</bright_green>", data);
        server.send_to_client(fd, "ACK: " + data);
        });

    server.set_on_close_callback([&logger_instance](socket_t fd, const std::string& reason) {
        logger_instance->info("[Server Callback] Client fd: {} <gray>closed</gray> ({})", fd, reason);
        });

    auto server_start_res = server.start(server_ip, server_port);
    if (server_start_res != tls_server::start_result::success) {
        logger_instance->error("Server failed to start on {}:{}", server_ip, server_port);
        return -1;
    }
    logger_instance->info("Server listening on <bright_white>{}:{}</bright_white>", server_ip, server_port);

    logger_instance->info("Server is <green>running</green>. Press <yellow>Enter</yellow> to stop...");
    std::cin.get(); // 사용자 입력 대기

    // ----------------------------------------------------
    // 리소스 정리 및 정상 종료
    // ----------------------------------------------------
    logger_instance->info("Shutting down server...");
    server.quit();

    logger_instance->info("<bright_cyan>Server stopped successfully.</bright_cyan>");
    return 0;
}
