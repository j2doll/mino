#include <iostream>
#include <string>
#include <csignal>
#include <memory>
#include <cstdint>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "mino/network/tcp/tcp.hpp"

using mino::network::tcp::tcp_server;

static tcp_server* g_server_ptr = nullptr;

static void signal_handler(int /*signum*/) {
    if (g_server_ptr) {
        spdlog::info("Signal received: shutting down server...");
        g_server_ptr->quit();
        spdlog::info("Server shutdown complete.");
    }
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

    // Logger: vcpkg/compiled spdlog 환경에서 안정적으로 동작하도록 sink로 직접 생성
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(spdlog::level::info);
    auto logger = std::make_shared<spdlog::logger>("mino_tcp_example", sink);
    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::info);

    tcp_server server;
    server.set_logger(logger);

    // 전역 포인터에 설정하여 시그널에서 접근 가능하게 함
    g_server_ptr = &server;

    // 시그널 핸들러 등록 (Ctrl-C)
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 콜백 설정 — 소켓 타입은 전역 네임스페이스의 `socket_t` 사용
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
    auto res = server.start("", 18080);
    if (res != tcp_server::start_result::success) {
        logger->error("Failed to start tcp_server (code={})", static_cast<int>(res));
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    logger->info("TCP server started on port 8080. Press Enter to stop, or Ctrl-C to quit.");

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
