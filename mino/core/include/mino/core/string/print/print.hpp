#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include <initializer_list>
#include <utility>
#include <memory>

#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/string/detail/detail.hpp"

namespace mino::core::string::print {

    // =========================================================================
    // ANSI 이스케이프 색상 코드 및 콘솔 초기화 유틸리티
    // =========================================================================
    inline constexpr std::string_view COLOR_RED = "\033[31m";
    inline constexpr std::string_view COLOR_RESET = "\033[0m";

    void enable_virtual_terminal_processing();

    // =========================================================================
    //  순서 기반 출력 인터페이스 (print / println / eprint / eprintln)
    // =========================================================================

    template <typename... Args>
    void print(std::string_view fmt, const Args&... args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        detail::format_to(std::cout, to_console_encoding(std::string(fmt)), args...);
    }

    template <typename... Args>
    void println(std::string_view fmt, const Args&... args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        detail::format_to(std::cout, to_console_encoding(std::string(fmt)), args...);
        std::cout << '\n';
    }

    template <typename... Args>
    void eprint(std::string_view fmt, const Args&... args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_to(std::cerr, to_console_encoding(std::string(fmt)), args...);
        std::cerr << COLOR_RESET;
    }

    template <typename... Args>
    void eprintln(std::string_view fmt, const Args&... args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_to(std::cerr, to_console_encoding(std::string(fmt)), args...);
        std::cerr << COLOR_RESET << '\n';
    }

    // =========================================================================
    // 이름 기반 출력 인터페이스 (printss / printlnss / eprintss / eprintlnss)
    // =========================================================================

    // (1) std::initializer_list 지원 (선언만 유지)
    void printss(std::string_view fmt, mino::core::string::detail::named_args args);
    void printlnss(std::string_view fmt, mino::core::string::detail::named_args args);
    void eprintss(std::string_view fmt, mino::core::string::detail::named_args args);
    void eprintlnss(std::string_view fmt, mino::core::string::detail::named_args args);

    //------------------------------------------------------------------------

    // (2) 템플릿 지원 (헤더에 구현 유지)
    template <typename KeyValueContainer>
    void printss(std::string_view fmt, const KeyValueContainer& args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        detail::format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
    }

    template <typename KeyValueContainer>
    void printlnss(std::string_view fmt, const KeyValueContainer& args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        detail::format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
        std::cout << '\n';
    }

    template <typename KeyValueContainer>
    void eprintss(std::string_view fmt, const KeyValueContainer& args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << COLOR_RESET;
    }

    template <typename KeyValueContainer>
    void eprintlnss(std::string_view fmt, const KeyValueContainer& args) {
        namespace detail = mino::core::string::detail;
        auto to_console_encoding = mino::core::string::to_console_encoding;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << COLOR_RESET << '\n';
    }

} // namespace mino::core::string::print
