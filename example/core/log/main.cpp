#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 1. 유틸리티 함수 테스트
void test_utility_functions() {
    namespace fs = std::filesystem;
    namespace tinylog = mino::core::log::tinylog;
    using log_level = tinylog::log_level;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;

    std::cout << "[Test 1] Utility Functions Test..." << std::endl;

    // log_level 관련 문자열 변환 함수 테스트
    assert(tinylog::to_string(log_level::trace) == "TRC");
    assert(tinylog::to_string(log_level::critical) == "CRT");

    assert(tinylog::to_full_string(log_level::debug) == "DEBUG");
    assert(tinylog::to_full_string(log_level::err) == "ERROR");

    assert(tinylog::to_korean_string(log_level::info, encoding_type::utf8) == "정보");
    assert(tinylog::to_korean_string(log_level::warn, encoding_type::utf8) == "경고");

    // eol_type 문자열 변환 함수 테스트
    assert(tinylog::to_string(eol_type::lf) == "\n");
    assert(tinylog::to_string(eol_type::crlf) == "\r\n");
    assert(tinylog::to_string(eol_type::cr) == "\r");

    // encoding 변환 함수 테스트 (UTF-8)
    std::string test_str = "Test UTF-8 String";
    assert(tinylog::convert_encoding(test_str, encoding_type::utf8) == test_str);

    std::cout << " -> Utility functions passed!\n\n";
}

// 2. console_sink 테스트
void test_console_sink() {
    namespace fs = std::filesystem;
    namespace tinylog = mino::core::log::tinylog;
    using log_level = tinylog::log_level;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;
    using console_sink_config = tinylog::console_sink_config;

    std::cout << "[Test 2] Console Sink Test..." << std::endl;

    console_sink_config config;
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::lf;

    // 생성자 및 name() 검증
    auto c_sink = std::make_shared<tinylog::console_sink>("console_test_sink", config);
    assert(c_sink->name() == "console_test_sink");

    // style_maps 접근 검증 및 색상 태그 출력 테스트
    std::cout << "Registered ANSI Styles count: " << c_sink->style_maps.size() << std::endl;

    // log() 메서드 직접 호출 및 태그 파싱 검증
    c_sink->log(log_level::info, "<red>Red Text</red> <green>Green Text</green> <bold>Bold Text</bold>");

    std::cout << " -> Console sink passed!\n\n";
}

// 3. rolling_file_sink 테스트
void test_rolling_file_sink() {
    namespace fs = std::filesystem;
    namespace tinylog = mino::core::log::tinylog;
    using log_level = tinylog::log_level;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;

    std::cout << "[Test 3] Rolling File Sink Test..." << std::endl;

    std::string test_filename = "test_roll.log";

    // 이전 잔여 파일 삭제
    fs::remove(test_filename);
    fs::remove(test_filename + ".1");

    tinylog::rolling_file_sink_config config;
    config.filename = test_filename;
    config.max_size = 100; // 롤링 테스트를 위한 작은 용량 설정
    config.max_files = 3;
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::lf;

    // static create() 검증
    auto file_sink = tinylog::rolling_file_sink::create("file_test_sink", config);
    assert(file_sink != nullptr);
    assert(file_sink->name() == "file_test_sink");

    // 잘못된 인자로 static create() 호출 시 nullptr 반환 검증
    tinylog::rolling_file_sink_config invalid_config = config;
    invalid_config.max_size = 0;
    assert(tinylog::rolling_file_sink::create("invalid", invalid_config) == nullptr);

    // 로그 전송 및 파일 롤링 동작 검증
    for (int i = 0; i < 10; ++i) {
        file_sink->log(tinylog::log_level::info, "<yellow>Log entry number</yellow> " + std::to_string(i));
    }

    // 파일 생성 검증 (롤링되어 .1 파일도 생성되었는지 확인)
    assert(fs::exists(test_filename));
    assert(fs::exists(test_filename + ".1"));

    std::cout << " -> Rolling file sink passed!\n\n";
}

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

int main() {
    try {
        std::cout << "===========================================" << std::endl;
        std::cout << "        Starting Tinylog Unit Tests        " << std::endl;
        std::cout << "===========================================\n" << std::endl;

        test_utility_functions();
        test_console_sink();
        test_rolling_file_sink();
        test_logger();

        std::cout << "===========================================" << std::endl;
        std::cout << "     All Public Member Tests Passed!       " << std::endl;
        std::cout << "===========================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
