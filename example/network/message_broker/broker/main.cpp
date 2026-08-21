#include <iostream>
#include <memory>
#include <string>

#include "mino/core/system/crash_handler.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/system/command_line.hpp"

#include "mino/external/log/spd/spd.hpp"

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

    // 콘솔 싱크
    namespace spd = mino::external::log::spd;
    auto console_sink = std::make_shared<spd::auto_color_sink<std::mutex>>();

    // 파일 싱크
    using encoding_file_sink_mt = spd::encoding_file_sink_mt;
    using encoding_rotating_zipping_sink_mt = spd::encoding_rotating_zipping_sink_mt;
    using encoding_daily_zipping_sink_mt = spd::encoding_daily_zipping_sink_mt;
    using log_encoding = spd::log_encoding;
    using line_ending = spd::line_ending;
    using time_zone_type = spd::time_zone_type;
#ifdef _WIN32 // Windows
    auto encoding_type = log_encoding::cp949; // 윈도우에서 log_encoding::utf8을 사용해도 됨.
    auto line_type = line_ending::crlf; // CRLF 줄바꿈
    auto bom_flag = false;
#else  // Linux/macOS
    auto encoding_type = log_encoding::utf8; // UTF-8
    auto line_type = line_ending::lf; // LF 줄바꿈
    auto bom_flag = false;
#endif
    auto time_zone = time_zone_type::local_time; // 시간을 적용하는 기준 타임 존
    auto step2_filename = "logs/rotating.log"; // 로깅 파일명
    auto step2_max_size = 100 * 1024 * 1024; // 로깅 파일이 최대 크기를 넘으면 파일 회전
    auto step2_max_files = 3; // 최대 3개의 회전된 파일을 유지
    auto step2_delete_on_failure = true; // 회전 실패 시 기존 파일 삭제 여부
    auto step2_compression_level = 6; // 가장 오래된 로깅 파일을 zip 으로 압축 시 압축 레벨 (1-9, 1이 가장 빠르고 9가 가장 높은 압축률)
    auto step2_max_zip_count = 4; // 최대 zip 파일 개수 (회전된 파일이 zip 으로 압축될 때, 최대 몇 개의 zip 파일을 유지할지 설정)
    auto rotating_zip_sink = std::make_shared<encoding_rotating_zipping_sink_mt>(
        step2_filename, // 로그 파일명
        step2_max_size, // 최대 파일 크기
        step2_max_files, // 최대 파일 개수
        encoding_type, // 인코딩 타입
        line_type, // 줄바꿈 타입
        bom_flag, // BOM 작성 여부
        step2_delete_on_failure, // 회전 실패 시 기존 파일 삭제 여부
        step2_compression_level, // 압축 레벨
        step2_max_zip_count, // 최대 zip 파일 개수
        time_zone //= time_zone_type::local_time
    );
    rotating_zip_sink->set_formatter(std::make_unique<spd::strip_tags_formatter>()); // auto_color_sink 와 함께 사용 시, 설정 필수

    // 로거 생성
    std::vector<spdlog::sink_ptr> sinks{ console_sink, rotating_zip_sink }; // 콘솔 싱크 + 파일 싱크
    auto broker_logger = std::make_shared<spdlog::logger>("broker_logger", sinks.begin(), sinks.end());
    broker_logger->set_level(spdlog::level::debug); // 레벨 설정

    using broker = mino::network::message_broker::broker;
    broker server_broker(broker_logger); // 브로커 생성
    // server_broker.set_logger(broker_logger); // 나중에 로거 설정해도 됨.

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
    broker_logger->info("보로커 시작: {}:{}", ip, port);

    broker_logger->info("브로커 인스턴스 작동 중. 엔터 키를 누르면 종료됨...");
    std::cin.get(); // 사용자가 키를 누를 때까지 브로커가 계속 실행됩니다. Block 상태로 대기합니다.

    server_broker.quit(); // 브로커 정상 종료

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
