#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 3. rolling_file_sink 테스트
void test_rolling_file_sink() {
    namespace fs = std::filesystem;
    namespace tinylog = mino::core::log::tinylog;
    using log_level = tinylog::log_level;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;
    using rolling_file_sink_config = tinylog::rolling_file_sink_config;

    std::cout << "[Test 3] Rolling File Sink Test..." << std::endl;

    std::string test_filename = "test_roll.log";

    // 이전 잔여 파일 삭제
    fs::remove(test_filename);
    fs::remove(test_filename + ".1");

    rolling_file_sink_config config;
    config.filename = test_filename;
    config.max_size = 100; // 롤링 테스트를 위한 작은 용량 설정 (100 bytes)
    config.max_files = 3; // 최대 3개의 롤링 파일 유지
    config.encoding = encoding_type::utf8; // UTF-8를 로깅 파일 인코딩 방식으로 설정
    config.eol = eol_type::lf; // LF(Line Feed)를 EOL으로 설정

    // static create() 검증
    auto file_sink = tinylog::rolling_file_sink::create("file_test_sink", config);
    assert(file_sink != nullptr);
    assert(file_sink->name() == "file_test_sink");

    // 잘못된 인자로 static create() 호출 시 nullptr 반환 검증
    rolling_file_sink_config invalid_config = config;
    invalid_config.max_size = 0;
    assert(tinylog::rolling_file_sink::create("invalid", invalid_config) == nullptr);

    // 로그 전송 및 파일 롤링 동작 검증
    for (int i = 0; i < 10; ++i) {
        file_sink->log(log_level::info, "<yellow>Log entry number</yellow> " + std::to_string(i));
    }

    // 파일 생성 검증 (롤링되어 .1 파일도 생성되었는지 확인)
    assert(fs::exists(test_filename));
    assert(fs::exists(test_filename + ".1"));

    std::cout << " -> Rolling file sink passed!\n\n";
}

