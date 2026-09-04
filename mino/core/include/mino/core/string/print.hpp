#pragma once

#include <iostream>
#include <string_view>
#include <sstream>

#include "mino/core/string/to_console_encoding.hpp"

// NOTE: 다음과 같이 람다를 사용하여 간단히 호출도 가능.
// 
// namespace mcsp = ::mino::core::string::print;
// auto print = [](std::string_view fmt, auto&&... args) {
//     mcsp::print(fmt, std::forward<decltype(args)>(args)...);
//     };
// auto println = [](std::string_view fmt, auto&&... args) {
//     mcsp::println(fmt, std::forward<decltype(args)>(args)...);
//     };
// 
// 람다로 간단하게 호출   println("TEST");
// 또는 직접 호출도 가능  mcsp::println("TEST");

namespace mino::core::string::print {

    // 1. 단일 인자를 출력 스트림으로 보내는 헬퍼 (인코딩 적용)
    template <typename T>
    void write_arg(std::ostream& os, const T& arg) {
        os << arg;
    }

    // 문자열 특화: 콘솔 인코딩 변환 적용
    template <>
    inline void write_arg<std::string>(std::ostream& os, const std::string& arg) {
        os << to_console_encoding(arg);
    }

    // std::string_view 특화: 콘솔 인코딩 변환 적용
    template <>
    inline void write_arg<std::string_view>(std::ostream& os, const std::string_view& arg) {
        os << to_console_encoding(std::string(arg));
    }

    // 2. 가변 인자를 순차적으로 포맷 스트링에 대입하는 헬퍼
    template <typename... Args>
    void format_to(std::ostream& os, std::string_view fmt, const Args&... args) {
        size_t last_pos = 0;
        auto replace_next = [&](const auto& arg) {
            size_t pos = fmt.find("{}", last_pos);
            if (pos != std::string_view::npos) {
                os << fmt.substr(last_pos, pos - last_pos);
                write_arg(os, arg);
                last_pos = pos + 2;
            }
        };

        (replace_next(args), ...);

        if (last_pos < fmt.size()) {
            os << fmt.substr(last_pos);
        }
    }

    // 3. 포맷 스트링도 인코딩 변환
    template <typename... Args>
    void print(std::string_view fmt, const Args&... args) {
        format_to(std::cout, to_console_encoding(std::string(fmt)), args...);
    }

    template <typename... Args>
    void println(std::string_view fmt, const Args&... args) {
        format_to(std::cout, to_console_encoding(std::string(fmt)), args...);
        std::cout << '\n';
    }

    template <typename... Args>
    void eprint(std::string_view fmt, const Args&... args) {
        format_to(std::cerr, to_console_encoding(std::string(fmt)), args...);
    }

    template <typename... Args>
    void eprintln(std::string_view fmt, const Args&... args) {
        format_to(std::cerr, to_console_encoding(std::string(fmt)), args...);
        std::cerr << '\n';
    }

}
