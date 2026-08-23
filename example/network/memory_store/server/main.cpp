#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// network memory store server
#include "mino/network/ethernet.hpp"
#include "mino/network/memory_store/server.hpp"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    namespace mnm = mino::network::memory_store;
    using memory_store_server = mnm::memory_store_server;

    // 메모리 저장소 서버 인스턴스
    memory_store_server server;

    // 로거 생성 및 설정 (mino::core::log::tinylog 기반)
    namespace mclt = mino::core::log::tinylog;
    auto console_sink = std::make_shared<mclt::console_sink>("ms_server_console");
    assert(console_sink != nullptr);

    auto server_logger = std::make_shared<mclt::logger>("ms_server_logger");
    assert(server_logger != nullptr);
    server_logger->add_sink(console_sink);
    server_logger->set_level(mclt::log_level::debug);
    mclt::logger::register_logger(server_logger);

    server.set_logger(server_logger);

    // 서버 네트워크 환경
    auto server_ip = "127.0.0.1";
    auto server_port = 43322;
    server.set_network(server_ip, server_port);

    // 서버가 클라이언트로 송신하는 시간 사이에 sleep 시간 추가
    auto transmission_sleep_duration = std::chrono::milliseconds(50);
    server.set_sleep_for_transmission(transmission_sleep_duration);

    // 저장소 파일 경로 설정 (절대 경로나 상대 경로 모두 가능)
    auto storage_file_path = "server.msdb"; // 현재 경로
    // auto storage_file_path = "msdb/server.msdb"; // 상대 경로
    // auto storage_file_path = "C:\\Temp\\server.msdb"; // 절대 경로
    // auto storage_file_path = "/tmp/server.msdb"; // 절대 경로
    server.set_storage_file(storage_file_path);

    // [옵션] 주기적으로 메모리 내용을 로컬 파일로 자동 저장하는 기능
    // 기능을 끄고 싶다면 이 줄을 주석 처리하거나 std::chrono::seconds(0)을 주면 됩니다.
    // auto auto_save_interval = std::chrono::seconds(60);
    // server.set_auto_save(auto_save_interval);

    // [옵션] 서버 시작 시, 로컬 파일의 기존 데이터 파일 로드
    // server.load();
    // server_logger->info("[Server] Loading existing data from {}...", storage_file_path);

    // 서버 시작
    server_logger->info("[Server] Booting Up Memory Store Server...");
    if (!server.start()) {
        server_logger->critical("[Server] Initialization Failed!");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }
    server_logger->info("[Server] Server Status: <yellow>ACTIVE</yellow> (Listening on <pink>{}:{}</pink>)", server_ip, server_port);
    server_logger->info("[Server] Press <bright_yellow>Enter Key</bright_yellow> to safely terminate the process...");

    // 키 입력 대기 (서버가 켜진 상태에서 Enter 키를 누르면 종료)
    std::cin.get();
    // 데몬 형태로 사용 시는 무한 루프로 대체하여 활용할 것.
    // while(true) {
    //      std::this_thread::sleep_for(std::chrono::seconds(1));
    //      ...
    // }

    // 서버에 저장된 모든 키/값 출력 (옵션)
    server_logger->info("Printing all current key/value.");
    server.print_all();

    // 서버 종료
    server_logger->info("[Server] Shutting down systems...");
    server.stop();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
