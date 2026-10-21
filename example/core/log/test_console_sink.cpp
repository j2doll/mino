#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

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
#ifdef _WIN32
    config.encoding = encoding_type::cp949;
    config.eol = eol_type::crlf;
#else 
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::lf;
#endif

    // 생성자 및 name() 검증
    auto c_sink = std::make_shared<tinylog::console_sink>("console_test_sink", config);
    assert(c_sink->name() == "console_test_sink");

    // style_maps 접근 검증 및 색상 태그 출력 테스트
    std::cout << "Registered ANSI Styles count: " << c_sink->style_maps.size() << std::endl;

    // log() 메서드 직접 호출 및 태그 파싱 검증
    c_sink->log(log_level::info, "<red>Red Text</red> <green>Green Text</green> <bold>Bold Text</bold>");

    std::cout << " -> Console sink passed!\n\n";
}


