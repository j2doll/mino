#pragma once

#include <string>

//  C++20(char8_t 지원) 환경이라면 reinterpret_cast를 수행하고,
// C++17 환경이라면 그대로 둡니다.
//  소문자 u8은 예약어이며, 대문자 U8을 사용하여 UTF-8 문자열 리터럴을 표현.
#if defined(__cpp_char8_t)
#   define U8(literal) reinterpret_cast<const char*>(literal)
#else
#   define U8(literal) literal
#endif
