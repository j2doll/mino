#pragma once

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "mino/core/string/string.hpp"

// 단언 헬퍼 매크로
#define TEST_CHECK(expr) \
    do { \
        if (!(expr)) { \
            eprint(tce("[FAIL] Line "), __LINE__, tce(": " #expr)); \
            std::abort(); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    print(tce(">>> Running Test Suite: "), tce(name), tce("..."))

// std::endl 등 ostream 조작자 함수 포인터 타입
using IoManipulator = std::ostream& (*)(std::ostream&);

// 콘솔 인코딩 변환 헬퍼
inline auto tce(const char* str) {
    return mino::core::string::to_console_encoding(str);
}

inline auto tce(const std::string& str) {
    return mino::core::string::to_console_encoding(str);
}

// 표준 출력 헬퍼 (std::cout)
template <typename... Args>
inline void print(const Args&... args) {
    (std::cout << ... << args) << std::endl;
}

// 조작자(std::endl 등)가 첫 번째 인자로 전달되는 경우를 위한 오버로드
template <typename... Args>
inline void print(IoManipulator manip, const Args&... args) {
    std::cout << manip;
    (std::cout << ... << args) << std::endl;
}

// 표준 에러 출력 헬퍼 (std::cerr)
template <typename... Args>
inline void eprint(const Args&... args) {
    (std::cerr << ... << args) << std::endl;
}

// 조작자(std::endl 등)가 첫 번째 인자로 전달되는 경우를 위한 오버로드
template <typename... Args>
inline void eprint(IoManipulator manip, const Args&... args) {
    std::cerr << manip;
    (std::cerr << ... << args) << std::endl;
}
