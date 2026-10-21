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
