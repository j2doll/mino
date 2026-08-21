#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "mino/core/string/string.hpp"

// network log manager 
#include "mino/network/log/log.hpp"

// CMake 매크로 미정의 시 기본 경로 fallback
#ifndef CMAKE_SOURCE_DIR_PATH
#  define CMAKE_SOURCE_DIR_PATH "."
#endif

//
// NOTE: logger_manager 과 hybrid_logger_manager 비교
// 
// 콘솔 출력 (Console Logging)
// -----------------------------------------------------
// - logger_manager: spdlog::sinks::stdout_color_sink_mt를 사용하여
// 로그 레벨에 따른 기본 색상 출력을 지원합니다.
// - hybrid_logger_manager: auto_color_sink를 사용하며
// AUTO_COLOR_KEYWORDS_JSON 설정을 통해 특정 단어/키워드별 맞춤 색상 강조
// 기능을 지원합니다.
// 
// 파일 출력 및 롤링 (File Logging & Rotation)
// -----------------------------------------------------
// - logger_manager: spdlog::sinks::rotating_file_sink_mt를 사용하여
// 용량 및 개수 기반의 표준 롤링 파일 싱크를 제공합니다.
// - hybrid_logger_manager: encoding_rotating_zipping_sink_mt를 사용하여
// 롤링 시 .zip 압축(ALL_COMPRESSION_LEVEL, ALL_MAX_ZIP_COUNT),
// 시간대 설정(ALL_TIMEZONE), 실패 시 파일 삭제 옵션(ALL_DELETE_ON_FAILURE)을 추가로
// 지원합니다.
//
// 파일 인코딩 및 포맷 (Encoding & Formatting)
// -----------------------------------------------------
// - logger_manager: 별도의 인코딩 변환 없이 시스템 기본 인코딩을 사용하며
// 파일 포맷터로 spdlog::pattern_formatter를 사용합니다.
// - hybrid_logger_manager: 파일 인코딩(UTF-8, CP949), 줄바꿈 형식(LF, CRLF, CR),
// BOM 추가 여부 설정을 지원하며 파일 저장 시 컬러 태그를 제거하는 strip_tags_formatter를
// 적용합니다.
// 
// 공통 지원 기능 (Common Features)
// -----------------------------------------------------
// - INI 설정 파일 기반 초기화(init) 및 핫 리로드(reloadIfChanged, startAutoReload,
// stopAutoReload) 지원.
// - 디스크 잔여 용량 감시 및 부족 시 파일 싱크 임시 분리 기능(DISK_GUARD_ENABLE) 지원.
// - 디스크 부족 알림 등을 위한 UDP 메시지 전송 기능 지원.
// 

int main(int argc, char* argv[]) {

    // 1. INI 설정 파일 경로 구성
    namespace fs = std::filesystem;

    fs::path basePath(CMAKE_SOURCE_DIR_PATH);
    fs::path configPath = basePath / "logger_manager_config.ini";
    std::string configPathStr = configPath.string();

    std::string sectionName = "Log"; // "logger_manager_config.ini" 의 INI 섹션 이름
    std::string envName = ""; // 환경 변수 미사용 시 빈 문자열 전달 (예: "LOG_CONFIG_PATH")

    std::cout << "=== Logger Manager Initialization Start ===" << std::endl;

    // =========================================================================
    // 2. mino::network::log::manager::logger_manager 초기화 및 퍼블릭 멤버 활용
    // =========================================================================
    using logger_manager = mino::network::log::manager::logger_manager;
    logger_manager std_mgr;

    // 표준 로거 매니저 초기화 (INI 설정 파일 기반)
    constexpr const char* default_logger_name = "app_logger";
    if (!std_mgr.init(
            configPathStr, // INI 경로
            sectionName, // INI 섹션 이름. "Log"
            default_logger_name, // 생성할 로거 이름
            envName)) // 환경 변수 이름 (빈 문자열 시 미사용)
    {
        std::cerr
            << "[Error] Failed to initialize standard logger_manager with config: "
            << configPathStr << std::endl;
        return 1;
    }

    // [Public] getLogger() 호출
    auto std_logger = std_mgr.getLogger();
    if (!std_logger) {
        std_logger = ::spdlog::get(default_logger_name);
    }

    // 수동 호출 테스트
    std_mgr.reloadIfChanged(); // INI 변경 감지 및 로거 설정 재적용
    std_mgr.startAutoReload(60); // INI 변경 감지 스레드 실행 (60초 주기)
    // NOTE: 위의 함수들은 반드시 호출할 필요는 없습니다. 

    // =========================================================================
    // 3. mino::network::log::manager::hybrid_logger_manager 초기화 및 퍼블릭 멤버 활용
    // =========================================================================
    using hybrid_logger_manager = mino::network::log::manager::hybrid_logger_manager;
    hybrid_logger_manager hybrid_mgr;

    // 하이브리드 매니저 초기화 (압축/인코딩 싱크 포함)
    constexpr const char* hybrid_logger_name = "hybrid_logger";
    if (!hybrid_mgr.init( // 초기화
            configPathStr, // INI 경로
            sectionName, // INI 섹션 이름. "Log"
            hybrid_logger_name, // 생성할 로거 이름
            envName)) // 환경 변수 이름 (빈 문자열 시 미사용)
    {
        std::cerr
            << "[Error] Failed to initialize hybrid_logger_manager with config: "
            << configPathStr << std::endl;
        return 1;
    }

    // [Public] getLogger() 호출
    auto hybrid_logger = hybrid_mgr.getLogger();
    if (!hybrid_logger) {
        hybrid_logger = ::spdlog::get(hybrid_logger_name);
    }

    // 수동 호출 테스트
    hybrid_mgr.reloadIfChanged(); // INI 변경 감지 및 로거 설정 재적용
    hybrid_mgr.startAutoReload(60); // INI 변경 감지 스레드 실행 (60초 주기)
    // NOTE: 위의 함수들은 반드시 호출할 필요는 없습니다. 

    std::cout << "=== Logging Loop Start (Press Ctrl+C to terminate) ===" << std::endl;

    // =========================================================================
    // 4. 로그 메시지 출력 루프 (레벨별 메시지 출력)
    // =========================================================================
    int loopCount = 0;
    while (true) {
        ++loopCount;

        // Standard Logger 출력
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

        // Hybrid Logger 출력 (압축/인코딩 싱크 동작)
        if (hybrid_logger) {
            hybrid_logger->trace("[Hybrid] Trace log output: {}", loopCount);
            hybrid_logger->info("[Hybrid] Info log with encoding/zipping sink check");
            hybrid_logger->warn("[Hybrid] Alert file targeted warning log");
            hybrid_logger->error("[Hybrid] Alert file targeted error log");
        }

        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // 예시 종료 조건(필요 시 무한 루프로 두거나 종료 시 stopAutoReload 호출)
        if (loopCount >= 120) { // 10분간 테스트 후 종료 예시
            break;
        }
    }

    // =========================================================================
    // 5. 스레드 정리 및 종료 (Public stopAutoReload 호출)
    // =========================================================================
    std::cout << "=== Stopping Auto Reload Threads ===" << std::endl;
    std_mgr.stopAutoReload(); // 스탠다드 로거 매니저 자동 리로드 스레드 종료
    hybrid_mgr.stopAutoReload(); // 하이브리드 로거 매니저 자동 리로드 스레드 종료

    std::cout << "=== Logger Manager Shutdown Complete ===" << std::endl;
    return 0;
}
