#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 4. logger 및 Registry 기능 테스트
void test_logger() {
    namespace fs = std::filesystem;
    namespace tinylog = mino::core::log::tinylog;
    using log_level = tinylog::log_level;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;
    using logger = tinylog::logger;
    using console_sink_config = tinylog::console_sink_config;

    std::cout << "[Test 4] Logger & Registry Test..." << std::endl;

    // 기존 등록 객체 제거
    logger::drop_all();

    // 생성자 및 name() 검증
    auto my_logger = std::make_shared<logger>("app_logger");
    assert(my_logger->name() == "app_logger");

    // 레벨 설정 (set_level, level) 검증
    my_logger->set_level(log_level::warn);
    assert(my_logger->level() == log_level::warn);

    // 레지스트리 (register_logger, get, drop_all) 검증
    assert(logger::register_logger(my_logger) == true);
    assert(logger::register_logger(my_logger) == false); // 중복 등록은 실패함.
    assert(logger::get("app_logger") == my_logger); // 등록된 로거 객체 반환
    assert(logger::get("non_existent") == nullptr); // 존재하지 않는 이름은 nullptr 반환

    // 싱크 추가 (add_sink)
    console_sink_config c_config;
    auto console = std::make_shared<tinylog::console_sink>("console_name", c_config);
    my_logger->add_sink(console);

    // 로그 레벨 필터링 및 포맷팅 메시지 출력 검증
    std::cout << "--- Below log entries should only show WARN, ERROR, CRITICAL ---" << std::endl;

    // 매크로 및 개별 로깅 메서드 호출 검증 (포맷팅 스트링 테스트 포함)
    my_logger->trace("Trace message: {}", 1);                      // 출력 안 됨 (레벨 낮음)
    my_logger->debug("<blue>Debug</blue> message: {}", 2);                      // 출력 안 됨
    my_logger->info("Info message: {}", 3);                        // 출력 안 됨
    my_logger->warn("<bright_red>Warn</bright_red> message: {} - <yellow>Warning!</yellow>", "arg1");  // 출력 됨
    my_logger->error("<magenta>Error</magenta> code: {0}, msg: {1}", 404, "Not Found");         // 순서 지정 포맷팅
    my_logger->critical("<red>Critical</red> <bold>failure</bold> <italic>occurred!</italic>");                     // 출력 됨

    // 일반 log 메서드 및 가변 인자 포맷팅 테스트
    my_logger->log(log_level::err, "Direct log call with arg: {}", 999);

    // Registry 정리 검증
    logger::drop_all();
    assert(logger::get("app_logger") == nullptr);

    std::cout << " -> Logger passed!\n\n";
}


