#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/log/manager/hybrid_logger_manager.hpp"

// CMake 매크로 미정의 시 기본 경로 fallback
#ifndef CMAKE_SOURCE_DIR_PATH
#  define CMAKE_SOURCE_DIR_PATH "."
#endif

//
// hybrid_logger_manager (mino::core::log::tinylog 기반)
// -----------------------------------------------------
// - 백엔드 엔진: 경량 자체 구현 로깅 라이브러리인 mino::core::log::tinylog 기반.
// - 콘솔 출력: tinylog::console_sink를 사용하여 태그 기반(<red>, <bright_green>, <bold> 등) ANSI 스타일 및 색상 출력을 지원.
// - 파일 출력 및 롤링: tinylog::rolling_file_sink를 사용하여 용량/개수 기반 롤링을 지원하며 파일 기록 시 서식 태그(<\/?...>)를 자동 제거.
// - 파일 인코딩 및 포맷: 파일 인코딩(UTF-8, CP949), 개행 문자 형식(LF, CRLF, CR) 변환을 지원.
// - 동적 관리: INI 설정 기반 핫 리로드, 디스크 잔여 용량 감시(Disk Guard), UDP 임계치 알림 지원.
//

namespace {
    const std::string hln = "hybrid_logger";

    // 짧은 이름의 람다 함수를 만들어서 로그 레벨별로 메시지를 출력하도록 함.
    auto ht = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->trace(fmt, std::forward<decltype(args)>(args)...); }
        };
    auto hd = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->debug(fmt, std::forward<decltype(args)>(args)...); }
        };
    auto hi = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->info(fmt, std::forward<decltype(args)>(args)...); }
        };
    auto hw = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->warn(fmt, std::forward<decltype(args)>(args)...); }
        };
    auto he = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->error(fmt, std::forward<decltype(args)>(args)...); }
        };
    auto hc = [](const char* fmt, auto&&... args) {
        auto hlg = mino::core::log::tinylog::logger::get(hln);
        if (hlg) { hlg->critical(fmt, std::forward<decltype(args)>(args)...); }
        };
} // namespace

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    // 1. INI 설정 파일 경로 구성
    namespace fs = std::filesystem;

    // 1-1. 로거의 환경설정 정보가 저장된 .ini 파일 경로를 설정
    fs::path basePath(CMAKE_SOURCE_DIR_PATH);
    fs::path configPath = basePath / "logger_manager_config.ini";
    std::string configPathStr = configPath.string();

    // 1-2. .ini 파일에 로거 정보가 설정된 섹션 이름을 지정
    std::string sectionName = "Log"; 
    // "... logger_manager_config.ini" 파일의 [Log] 섹션에서 로거 설정을 읽어옴.

    // 1-3. 운영체제 환경변수에 특정한 값이 있는 경우,
    // 해당 변수를 .ini 경로로 우선적으로 사용하도록 설정 가능.
    std::string envName = ""; 
    // 
    // 예> envName = "MY_LOGGER_CONFIG_PATH"; 이고,
    // OS 환경변수 "MY_LOGGER_CONFIG_PATH"가 "C:\test\my_logger_config.ini" 이면,
    // 해당 경로의 .ini를 우선 사용함.

    std::cout << "=== Hybrid Logger Manager Initialization Start ===" << std::endl;

    // =========================================================================
    // 2. mino::network::log::manager::hybrid_logger_manager 초기화 (tinylog 기반)
    // =========================================================================
    namespace mnlm = mino::network::log::manager;
    using hybrid_logger_manager = mnlm::hybrid_logger_manager;

    // 2-1. hybrid_logger_manager 객체 생성
    hybrid_logger_manager hybrid_mgr;

    // 2-2. hybrid_logger_manager 초기화
    constexpr const char* hybrid_logger_name = "hybrid_logger";
    if (!hybrid_mgr.init(
        configPathStr,      // [1-1] 로거 설정정보가 있는 .ini 파일 경로
        sectionName,        // [1-2] .ini 파일의 섹션 이름: [Log] 등
        hybrid_logger_name, // 생성할 로거 이름
        envName))           // [1-3] 환경 변수 이름 (없을 수도 있음)
    {
        std::cerr
            << "[Error] Failed to initialize hybrid_logger_manager with config: "
            << configPathStr << std::endl;
        return 1;
    }

    // 2-3. 로거 객체 가져오기 (tinylog 기반)
    // auto hybrid_logger = hybrid_mgr.getLogger();
    // 
    // if (!hybrid_logger) {
    //     namespace mclt = mino::core::log::tinylog;
    //     using logger = mclt::logger;
    //     hybrid_logger = logger::get(hybrid_logger_name); // 로거 이름으로 로거 얻기
    // }

    hybrid_mgr.reloadIfChanged();   // .ini 파일이 변경된 경우, 설정을 다시 읽어 적용하도록 시도
    hybrid_mgr.startAutoReload(60); // .ini 자동 읽기 (60초 주기로 .ini 파일을 다시 읽음)

    std::cout << "=== Logging Loop Start (Press Ctrl+C to terminate) ===" << std::endl;

    // =========================================================================
    // 3. 로그 메시지 출력 루프 (레벨별 메시지 출력)
    // =========================================================================

    int loopCount = 0;
    while (true) {
        ++loopCount;

        // Hybrid Logger (tinylog) 출력 (태그 서식 및 파일 인코딩 처리)
        ht("<gray>[Hybrid]</gray> Trace log: {}", loopCount);
        hd("<cyan>[Hybrid]</cyan> Debug log: {}", loopCount);
        hi("<bright_green>[Hybrid]</bright_green> <bold>Info</bold> log: {}", loopCount);
        hw("<bright_yellow>[Hybrid]</bright_yellow> Warning log: {}", loopCount);
        he("<bright_red>[Hybrid]</bright_red> Error log (port: {}): {}", 10514, loopCount);
        hc("<pink>[Hybrid]</pink> Critical alert log: {}", loopCount);

        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (loopCount >= 120) {
            break;
        }
    }

    // =========================================================================
    // 4. 스레드 정리 및 종료
    // =========================================================================
    std::cout << "=== Stopping Auto Reload Thread ===" << std::endl;
    hybrid_mgr.stopAutoReload(); // 안전하게 종료 (Close gracefully)
    // 종료시까지 시간이 걸림

    std::cout << "=== Logger Manager Shutdown Complete ===" << std::endl;

    return 0;
}
