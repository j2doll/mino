#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "mino/core/string/string.hpp"

// core tinylog
#include "mino/core/log/tinylog/logger.hpp"

// network log manager 
#include "mino/network/log/log.hpp"

// CMake 매크로 미정의 시 기본 경로 fallback
#ifndef CMAKE_SOURCE_DIR_PATH
#  define CMAKE_SOURCE_DIR_PATH "."
#endif

//
// NOTE: logger_manager 과 hybrid_logger_manager 비교
// 
// 백엔드 엔진 (Backend Engine)
// -----------------------------------------------------
// - logger_manager: spdlog 엔진 기반 로거 매니저.
// - hybrid_logger_manager: 경량 자체 구현 로깅 라이브러리인 mino::core::log::tinylog 기반 로거 매니저.
// 
// 콘솔 출력 (Console Logging)
// -----------------------------------------------------
// - logger_manager: spdlog::sinks::stdout_color_sink_mt를 사용하여
//   로그 레벨에 따른 기본 색상 출력을 지원합니다.
// - hybrid_logger_manager: tinylog::console_sink를 사용하여
//   태그 기반(<red>, <bright_green>, <bold> 등) 맞춤형 ANSI 스타일 및 색상 출력을 지원합니다.
// 
// 파일 출력 및 롤링 (File Logging & Rotation)
// -----------------------------------------------------
// - logger_manager: spdlog::sinks::rotating_file_sink_mt를 사용하여
//   용량 및 개수 기반의 표준 롤링 파일 싱크를 제공합니다.
// - hybrid_logger_manager: tinylog::rolling_file_sink를 사용하여
//   용량/개수 기반 롤링을 지원하며, 파일 기록 시 서식 태그(<\/?...>)를 자동으로 제거하여 저장합니다.
//
// 파일 인코딩 및 포맷 (Encoding & Formatting)
// -----------------------------------------------------
// - logger_manager: 별도의 인코딩 변환 없이 시스템 기본 인코딩을 사용합니다.
// - hybrid_logger_manager: 파일 인코딩(UTF-8, CP949), 개행 문자 형식(LF, CRLF, CR) 변환을 기본 지원합니다.
// 
// 공통 지원 기능 (Common Features)
// -----------------------------------------------------
// - INI 설정 파일 기반 초기화(init) 및 핫 리로드(reloadIfChanged, startAutoReload, stopAutoReload) 지원.
// - 디스크 잔여 용량 감시 및 부족 시 파일 싱크 임시 분리 기능(DISK_GUARD_ENABLE) 지원.
// - 디스크 부족 알림 등을 위한 UDP 메시지 전송 기능 지원.
// 

int main(int argc, char* argv[]) {

    // 1. INI 설정 파일 경로 구성
    namespace fs = std::filesystem;

    fs::path basePath(CMAKE_SOURCE_DIR_PATH);
    fs::path configPath = basePath / "logger_manager_config.ini";
    std::string configPathStr = configPath.string();

    std::string sectionName = "Log"; // "logger_manager_config.ini"의 INI 섹션 이름
    std::string envName = "";        // 환경 변수 미사용 시 빈 문자열 전달 (예: "LOG_CONFIG_PATH")

    std::cout << "=== Logger Manager Initialization Start ===" << std::endl;

    // =========================================================================
    // 2. mino::network::log::manager::logger_manager 초기화 (spdlog 기반)
    // =========================================================================
    using logger_manager = mino::network::log::manager::logger_manager;
    logger_manager std_mgr;

    constexpr const char* default_logger_name = "app_logger";
    if (!std_mgr.init(
        configPathStr,       // INI 경로
        sectionName,         // INI 섹션 이름 ("Log")
        default_logger_name, // 생성할 로거 이름
        envName))            // 환경 변수 이름
    {
        std::cerr
            << "[Error] Failed to initialize standard logger_manager with config: "
            << configPathStr << std::endl;
        return 1;
    }

    // [Public] getLogger() 호출 (spdlog::logger 반환)
    auto std_logger = std_mgr.getLogger();
    if (!std_logger) {
        std_logger = ::spdlog::get(default_logger_name);
    }

    std_mgr.reloadIfChanged();   // INI 변경 감지 및 로거 설정 재적용
    std_mgr.startAutoReload(60); // INI 변경 감지 스레드 실행 (60초 주기)

    // =========================================================================
    // 3. mino::network::log::manager::hybrid_logger_manager 초기화 (tinylog 기반)
    // =========================================================================
    using hybrid_logger_manager = mino::network::log::manager::hybrid_logger_manager;
    hybrid_logger_manager hybrid_mgr;

    constexpr const char* hybrid_logger_name = "hybrid_logger";
    if (!hybrid_mgr.init(
        configPathStr,      // INI 경로
        sectionName,        // INI 섹션 이름 ("Log")
        hybrid_logger_name, // 생성할 로거 이름
        envName))           // 환경 변수 이름
    {
        std::cerr
            << "[Error] Failed to initialize hybrid_logger_manager with config: "
            << configPathStr << std::endl;
        return 1;
    }

    // [Public] getLogger() 호출 (mino::core::log::tinylog::logger 반환)
    auto hybrid_logger = hybrid_mgr.getLogger();
    if (!hybrid_logger) {
        hybrid_logger = mino::core::log::tinylog::logger::get(hybrid_logger_name);
    }

    hybrid_mgr.reloadIfChanged();   // INI 변경 감지 및 로거 설정 재적용
    hybrid_mgr.startAutoReload(60); // INI 변경 감지 스레드 실행 (60초 주기)

    std::cout << "=== Logging Loop Start (Press Ctrl+C to terminate) ===" << std::endl;

    // =========================================================================
    // 4. 로그 메시지 출력 루프 (레벨별 메시지 출력)
    // =========================================================================
    int loopCount = 0;
    while (true) {
        ++loopCount;

        // Standard Logger (spdlog) 출력
        if (std_logger) {
            std_logger->trace("[Standard] Trace message (iteration: {})", loopCount);
            std_logger->debug("[Standard] Debug message (iteration: {})", loopCount);
            std_logger->info("[Standard] Info message: Status OK, code={}", 200);
            std_logger->warn("[Standard] Warning message: High memory usage threshold reached");
            std_logger->error("[Standard] Error message: Connection timeout on port {}", 10514);
            std_logger->critical("[Standard] Critical alert message: System check required");
        }

        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Hybrid Logger (tinylog) 출력 (태그 서식 및 파일 인코딩 처리)
        if (hybrid_logger) {
            hybrid_logger->trace("<gray>[Hybrid]</gray> Trace log output: {}", loopCount);
            hybrid_logger->debug("<cyan>[Hybrid]</cyan> Debug log output (iteration: {})", loopCount);
            hybrid_logger->info("<bright_green>[Hybrid]</bright_green> Info log with <bold>tinylog</bold> formatting check");
            hybrid_logger->warn("<bright_yellow>[Hybrid]</bright_yellow> Alert file targeted warning log");
            hybrid_logger->error("<bright_red>[Hybrid]</bright_red> Alert file targeted error log (port: {})", 10514);
            hybrid_logger->critical("<pink>[Hybrid]</pink> Critical alert log: Emergency system check required");
        }

        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        if (loopCount >= 120) {
            break;
        }
    }

    // =========================================================================
    // 5. 스레드 정리 및 종료
    // =========================================================================
    std::cout << "=== Stopping Auto Reload Threads ===" << std::endl;
    std_mgr.stopAutoReload();
    hybrid_mgr.stopAutoReload();

    std::cout << "=== Logger Manager Shutdown Complete ===" << std::endl;
    return 0;
}
