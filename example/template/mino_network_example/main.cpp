#include <iostream>
#include <string>
#include <csignal>
#include <memory>
#include <cstdint>

#include "mino/core/daemon/daemon.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/tcp/tcp.hpp"

void clean_up_resources(mino::network::tcp::tcp_server* tcp_server)
{
    if (tcp_server) {
        tcp_server->quit();
    }
    std::cout << std::endl << "Resources cleaned up. Exiting." << std::endl;
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed, Error Code: " << result << std::endl;
        return 1;
    }
#endif

    using tcp_server = mino::network::tcp::tcp_server;
    using termination_handler = mino::core::daemon::termination_handler;

    auto& handler = termination_handler::get_instance();
    handler.initialize();

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto console_sink = std::make_shared<mclt::console_sink>("tcp_example_console");
    auto logger = std::make_shared<mclt::logger>("mino_tcp_example");
    logger->add_sink(console_sink);
    logger->set_level(mclt::log_level::info);
    mclt::logger::register_logger(logger);

    tcp_server server;

    handler.set_callback([&server]() {
        clean_up_resources(&server);
        std::exit(0);
        });

    server.set_logger(logger);

    // 콜백 설정 — 소켓 타입은 전역 네임스페이스의 socket_t 사용
    server.set_on_connect_callback([&logger](socket_t s, const std::string& msg) {
        logger->info("Client connected (socket={}): {}", static_cast<uint64_t>(s), msg);
        });

    server.set_on_receive_callback([&server, &logger](socket_t s, const std::string& data) {
        logger->info("Received from {}: {}", static_cast<uint64_t>(s), data);
        // 간단 에코 응답
        std::string reply = "Echo: " + data;
        server.send_to_client(s, reply);
        });

    server.set_on_close_callback([&logger](socket_t s, const std::string& reason) {
        logger->info("Client closed (socket={}): {}", static_cast<uint64_t>(s), reason);
        });

    // 서버 시작 (모든 인터페이스 바인드, 포트 18080)
    auto port_number = 18080;
    auto res = server.start("", port_number);
    if (res != tcp_server::start_result::success) {
        logger->error("Failed to start tcp_server (code={})", static_cast<int>(res));
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    logger->info("TCP server started on port {}. Press Enter to stop, or Ctrl-C to quit.", port_number);

    // 간단한 종료 대기: Enter 입력 시 종료
    std::cin.get();

    logger->info("Stopping server...");
    server.quit();
    logger->info("Server stopped.");

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
