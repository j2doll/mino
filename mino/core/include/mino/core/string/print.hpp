#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include <initializer_list>
#include <utility>
#include <memory>

#include "mino/core/string/to_console_encoding.hpp"

// NOTE: 다음과 같이 람다를 사용하여 간단히 호출도 가능.
//
// #include "mino/core/string/string.hpp"
// namespace {
//     namespace mcs = mino::core::string;
//     namespace mcsp = mino::core::string::print;
// 
//     auto tce = mcs::to_console_encoding;
//     auto tcev = [](std::string_view sv) { return mcs::to_console_encoding(std::string(sv)); };
// 
//     auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
//     auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
//     std::ostream& (*endl)(std::ostream&) = std::endl;
// 
//     auto println = [](std::string_view fmt, auto&&... args) { mcsp::println(fmt, std::forward<decltype(args)>(args)...); };
//     auto eprintln = [](std::string_view fmt, auto&&... args) { mcsp::eprintln(fmt, std::forward<decltype(args)>(args)...); };
//
//     auto printss = [](std::string_view fmt, mcsp::named_args args) { mcsp::printss(fmt, args); };
//     auto printlnss = [](std::string_view fmt, mcsp::named_args args) { mcsp::printlnss(fmt, args); };
//     auto eprintss = [](std::string_view fmt, mcsp::named_args args) { mcsp::eprintss(fmt, args); };
//     auto eprintlnss = [](std::string_view fmt, mcsp::named_args args) { mcsp::eprintlnss(fmt, args); };
// }
// 

namespace mino::core::string::print {

    // =========================================================================
    // 1. 단일 인자를 스트림에 출력하는 헬퍼 및 타입별 특화 (인코딩 변환 포함)
    // =========================================================================

    // 기본 템플릿: int, double, float 등 << 연산자를 지원하는 모든 기본 타입
    template <typename T>
    void write_arg(std::ostream& os, const T& arg) {
        os << arg;
    }

    // std::string 특화
    template <>
    inline void write_arg<std::string>(std::ostream& os, const std::string& arg) {
        os << to_console_encoding(arg);
    }

    // std::string_view 특화
    template <>
    inline void write_arg<std::string_view>(std::ostream& os, const std::string_view& arg) {
        os << to_console_encoding(std::string(arg));
    }

    // 문자열 포인터(const char*) 특화
    template <>
    inline void write_arg<const char*>(std::ostream& os, const char* const& arg) {
        if (arg) {
            os << to_console_encoding(std::string(arg));
        }
    }

    // 문자열 리터럴 배열(const char[N]) 오버로드
    template <size_t N>
    inline void write_arg(std::ostream& os, const char(&arg)[N]) {
        os << to_console_encoding(std::string(arg));
    }

    // bool 특화: 1/0 대신 "true"/"false"로 출력
    template <>
    inline void write_arg<bool>(std::ostream& os, const bool& arg) {
        os << (arg ? "true" : "false");
    }

    // =========================================================================
    // 2. 다양한 타입(primitive, string, string_view 등)을 수용하는 타입 소거 래퍼
    // =========================================================================

    struct NamedArg {
        std::string_view key;
        const void* data = nullptr;
        void (*printer)(std::ostream&, const void*) = nullptr;

        // (1) 문자열 리터럴/포인터 전용 생성자
        NamedArg(std::string_view k, const char* v)
            : key(k), data(v),
            printer([](std::ostream& os, const void* ptr) {
            write_arg(os, static_cast<const char*>(ptr));
                }) {
        }

        // (2) 임의의 타입(int, double, bool, std::string, std::string_view 등) 생성자
        template <typename T>
        NamedArg(std::string_view k, const T& v)
            : key(k), data(std::addressof(v)),
            printer([](std::ostream& os, const void* ptr) {
            write_arg(os, *static_cast<const T*>(ptr));
                }) {
        }

        void write(std::ostream& os) const {
            if (printer && data) {
                printer(os, data);
            }
        }
    };

    using named_args = std::initializer_list<NamedArg>;

    // =========================================================================
    // 3. 포맷팅 내부 파싱 헬퍼 함수들
    // =========================================================================

    // (A) 순서 기반 플레이스홀더({}) 포맷 헬퍼
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

    // (B) 이름 기반 플레이스홀더({key}) 파싱 공통 로직 ({{, }} 및 불완전 괄호 처리 지원)
    template <typename LookupFn>
    void format_named_impl(std::ostream& os, std::string_view fmt, LookupFn&& lookup) {
        size_t i = 0;
        while (i < fmt.size()) {
            if (fmt[i] == '{') {
                // (1) "{{" 이스케이프 처리 -> "{" 단일 문자 출력
                if (i + 1 < fmt.size() && fmt[i + 1] == '{') {
                    os << '{';
                    i += 2;
                    continue;
                }

                // (2) 닫는 괄호 '}' 탐색
                size_t close_pos = fmt.find('}', i + 1);
                if (close_pos == std::string_view::npos) {
                    os << fmt.substr(i);
                    break;
                }

                // [수정된 부분] 닫는 괄호 '}' 전에 또 다른 '{'가 먼저 나온 경우
                // 현재의 '{'는 닫히지 않은 불완전한 괄호이므로 문자 그대로 출력 후 계속 진행
                size_t next_open = fmt.find('{', i + 1);
                if (next_open != std::string_view::npos && next_open < close_pos) {
                    os << '{';
                    i += 1;
                    continue;
                }

                // (3) 정상 플레이스홀더 {key} 치환
                std::string_view key = fmt.substr(i + 1, close_pos - i - 1);

                if (!lookup(key)) {
                    os << fmt.substr(i, close_pos - i + 1);
                }

                i = close_pos + 1;
            }
            else if (fmt[i] == '}') {
                // "}}" 이스케이프 처리 -> "}" 단일 문자 출력
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
                    os << '}';
                    i += 2;
                    continue;
                }

                // 단독 '}'는 문자 그대로 출력
                os << '}';
                i += 1;
            }
            else {
                // 다음 '{' 또는 '}'가 나오기 전까지의 일반 문자열을 일괄 출력
                size_t next_brace = fmt.find_first_of("{}", i);
                if (next_brace == std::string_view::npos) {
                    os << fmt.substr(i);
                    break;
                }
                os << fmt.substr(i, next_brace - i);
                i = next_brace;
            }
        }
    }

    // (C) initializer_list 전용 매핑 헬퍼
    inline void format_named_to(std::ostream& os, std::string_view fmt, named_args args) {
        format_named_impl(os, fmt, [&](std::string_view key) {
            for (const auto& arg : args) {
                if (arg.key == key || to_console_encoding(std::string(arg.key)) == key) {
                    arg.write(os);
                    return true;
                }
            }
            return false;
            });
    }

    // (D) std::map / std::unordered_map 등 컨테이너 전용 매핑 헬퍼
    template <typename KeyValueContainer>
    void format_named_to(std::ostream& os, std::string_view fmt, const KeyValueContainer& args) {
        format_named_impl(os, fmt, [&](std::string_view key) {
            for (const auto& [k, v] : args) {
                if (k == key || to_console_encoding(std::string(k)) == key) {
                    write_arg(os, v);
                    return true;
                }
            }
            return false;
            });
    }

    // =========================================================================
    // 4. 순서 기반 출력 인터페이스 (print / println / eprint / eprintln)
    // =========================================================================

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

    // =========================================================================
    // 5. 이름 기반 출력 인터페이스 (printss / printlnss / eprintss / eprintlnss)
    // =========================================================================

    // (1) std::initializer_list 지원 (예: { {"name", "John"}, {"age", 20} })
    inline void printss(std::string_view fmt, named_args args) {
        format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
    }

    inline void printlnss(std::string_view fmt, named_args args) {
        format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
        std::cout << '\n';
    }

    inline void eprintss(std::string_view fmt, named_args args) {
        format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
    }

    inline void eprintlnss(std::string_view fmt, named_args args) {
        format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << '\n';
    }

    // (2) 템플릿 지원 (std::map, std::unordered_map 등 컨테이너 직접 전달)
    template <typename KeyValueContainer>
    void printss(std::string_view fmt, const KeyValueContainer& args) {
        format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
    }

    template <typename KeyValueContainer>
    void printlnss(std::string_view fmt, const KeyValueContainer& args) {
        format_named_to(std::cout, to_console_encoding(std::string(fmt)), args);
        std::cout << '\n';
    }

    template <typename KeyValueContainer>
    void eprintss(std::string_view fmt, const KeyValueContainer& args) {
        format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
    }

    template <typename KeyValueContainer>
    void eprintlnss(std::string_view fmt, const KeyValueContainer& args) {
        format_named_to(std::cerr, to_console_encoding(std::string(fmt)), args);
        std::cerr << '\n';
    }

}
