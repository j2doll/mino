#include <iostream>
#include <memory>
#include <string>

#include "mino/core/system/crash_handler.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/command_line.hpp"

#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/message_broker/broker.hpp"

void clean_up_resources(mino::network::message_broker::broker* broker)
{
    // broker->shutdown_by_force();
    broker->quit();
    std::cerr << "Broker resources cleaned up successfully." << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    // 크래시 핸들러 초기화 및 사용자 정의 콜백 등록
    mino::core::system::crash_handler::initialize([](const std::string& log_message) {
        std::cout << "\n[User Callback] Crash Detected! Reporting to console...\n";
        std::cout << log_message << std::endl;
        });

    // 종료 핸들러 초기화 및 브로커 종료 로직 등록
    auto& handler = mino::core::daemon::termination_handler::get_instance();
    handler.initialize();

    // tinylog 설정
    namespace mclt = mino::core::log::tinylog;

    // 1. 콘솔 싱크
    mclt::console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = mclt::encoding_type::cp949;
#else
    console_cfg.encoding = mclt::encoding_type::utf8;
#endif
    auto console_sink = std::make_shared<mclt::console_sink>("broker_console", console_cfg);

    // 2. 롤링 파일 싱크
    mclt::rolling_file_sink_config file_cfg;
    file_cfg.filename = "logs/rotating.log"; // 로깅 파일명
    file_cfg.max_size = 100 * 1024 * 1024;   // 최대 파일 크기 (100 MB)
    file_cfg.max_files = 3;                  // 최대 보관 백업 개수
#ifdef _WIN32
    file_cfg.encoding = mclt::encoding_type::cp949;
    file_cfg.eol = mclt::eol_type::crlf;     // CRLF 줄바꿈
#else
    file_cfg.encoding = mclt::encoding_type::utf8;
    file_cfg.eol = mclt::eol_type::lf;       // LF 줄바꿈
#endif

    auto rotating_file_sink = mclt::rolling_file_sink::create("broker_file", file_cfg);

    // 3. 로거 생성 및 싱크 등록
    auto broker_logger = std::make_shared<mclt::logger>("broker_logger");
    broker_logger->add_sink(console_sink);
    broker_logger->add_sink(rotating_file_sink);
    broker_logger->set_level(mclt::log_level::debug); // 레벨 설정
    mclt::logger::register_logger(broker_logger);

    using broker = mino::network::message_broker::broker;
    broker server_broker(broker_logger); // 브로커 생성

    handler.set_callback([&server_broker]() { // 비정상 종료(Ctrl+C 등) 시 호출되는 함수 등록
        clean_up_resources(&server_broker);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
        });

    // 명령행 인자 파서 설정
    mino::core::system::command_line cmd;
    cmd.add_option("ip", 'i', true, "Broker IP address (default: 127.0.0.1)");
    cmd.add_option("port", 'p', true, "Broker port (default: 54321)");
    cmd.set_version("1.0");

    if (!cmd.parse(argc, argv)) {
        std::cout << cmd.usage() << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (cmd.has("help")) {
        std::cout << cmd.usage() << std::endl; // 도움말 출력
#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    }

    std::string ip = cmd.get("ip", "127.0.0.1");
    std::string port_str = cmd.get("port", "54321");
    int port = 54321;
    try {
        port = std::stoi(port_str);
        if (port < 1 || port > 65535) {
            throw std::out_of_range("Port number must be between 1 and 65535");
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Invalid port: " << port_str << "\n\n" << cmd.usage() << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    server_broker.start_broker(ip, port); // 브로커 시작
    broker_logger->info("<bright_green>브로커 시작:</bright_green> {}:{}", ip, port);

    broker_logger->info("브로커 인스턴스 작동 중. <bright_yellow>엔터 키</bright_yellow>를 누르면 종료됨...");
    std::cin.get(); // 사용자가 키를 누를 때까지 브로커가 계속 실행됩니다.

    server_broker.quit(); // 브로커 정상 종료

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
