#include "mino/core/string/print/print.hpp"

#if defined(_WIN32) || defined(_WIN64)
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#      define NOMINMAX
#   endif
#   include <windows.h>
#endif

namespace mino::core::string::print {

    void enable_virtual_terminal_processing() {
#if defined(_WIN32) || defined(_WIN64)
        static const bool initialized = []() {
            HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE)
                return false;
            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode))
                return false;
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
            return true;
            }();
        (void)initialized;
#endif
    }

    // 이름 기반 일반 함수 구현부
    void printss(std::string_view fmt, mino::core::string::detail::named_args args) {
        namespace detail = mino::core::string::detail;
        detail::format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
    }

    void printlnss(std::string_view fmt, mino::core::string::detail::named_args args) {
        namespace detail = mino::core::string::detail;
        detail::format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
        std::cout << '\n';
    }

    void eprintss(std::string_view fmt, mino::core::string::detail::named_args args) {
        namespace detail = mino::core::string::detail;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << COLOR_RESET;
    }

    void eprintlnss(std::string_view fmt, mino::core::string::detail::named_args args) {
        namespace detail = mino::core::string::detail;
        enable_virtual_terminal_processing();
        std::cerr << COLOR_RED;
        detail::format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << COLOR_RESET << '\n';
    }

} // namespace mino::core::string::print
