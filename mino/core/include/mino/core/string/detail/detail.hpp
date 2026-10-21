#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <list>
#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <type_traits>

#include "mino/core/string/to_console_encoding.hpp"

namespace mino::core::string::detail {

    // 1. 반복 가능한 컨테이너 판별 (std::begin, std::end 존재 여부)
    template <typename T, typename = void>
    struct is_iterable : std::false_type {};

    template <typename T>
    struct is_iterable<T, std::void_t<
        decltype(std::begin(std::declval<const T&>())),
        decltype(std::end(std::declval<const T&>()))
        >> : std::true_type {};

    // 2. std::pair 판별 트레이트
    template <typename T>
    struct is_pair : std::false_type {};

    template <typename T1, typename T2>
    struct is_pair<std::pair<T1, T2>> : std::true_type {};

    // CV-qualified pair support (handles const/volatile pairs from map/set)
    template <typename T1, typename T2>
    struct is_pair<const std::pair<T1, T2>> : std::true_type {};

    template <typename T1, typename T2>
    struct is_pair<volatile std::pair<T1, T2>> : std::true_type {};

    template <typename T1, typename T2>
    struct is_pair<const volatile std::pair<T1, T2>> : std::true_type {};

    // 3. std::tuple 판별 트레이트
    template <typename T>
    struct is_tuple : std::false_type {};

    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    // 전방 선언
    template <typename T>
    std::string to_string_helper(const T& value) noexcept;

    // std::pair 지원 함수
    template <typename T1, typename T2>
    std::string format_pair(const std::pair<T1, T2>& p) noexcept {
        try {
            return "(" + to_string_helper(p.first) + ": " + to_string_helper(p.second) + ")";
        }
        catch (...) {
            return "(error: pair)";
        }
    }

    // std::tuple 지원 함수
    template <typename Tuple, std::size_t... Is>
    std::string format_tuple_impl(const Tuple& t, std::index_sequence<Is...>) noexcept {
        try {
            std::string result = "(";
            std::size_t idx = 0;
            ((result += (idx++ > 0 ? ", " : "") + to_string_helper(std::get<Is>(t))), ...);
            result += ")";
            return result;
        }
        catch (...) {
            return "(error: tuple)";
        }
    }

    template <typename... Ts>
    std::string format_tuple(const std::tuple<Ts...>& t) noexcept {
        return format_tuple_impl(t, std::index_sequence_for<Ts...>{});
    }

    // 타입별 문자열 변환 통합 헬퍼
    template <typename T>
    std::string to_string_helper(const T& value) noexcept {
        try {
            if constexpr (std::is_convertible_v<T, std::string_view>) {
                return std::string(std::string_view(value));
            }
            else if constexpr (is_pair<T>::value) {
                return format_pair(value);
            }
            else if constexpr (is_tuple<T>::value) {
                return format_tuple(value);
            }
            else if constexpr (is_iterable<T>::value) {
                std::string result = "[";
                bool first = true;
                for (const auto& elem : value) {
                    if (!first) {
                        result += ", ";
                    }
                    result += to_string_helper(elem);
                    first = false;
                }
                result += "]";
                return result;
            }
            else {
                std::ostringstream oss;
                oss << value;
                return oss.str();
            }
        }
        catch (...) {
            return "{format_error}";
        }
    }

    // 가변 인자 수집 헬퍼
    void collect_format_args(std::vector<std::string>&) noexcept;

    template <typename First, typename... Rest>
    void collect_format_args(std::vector<std::string>& args_out, const First& first, const Rest&... rest) noexcept {
        try {
            args_out.push_back(to_string_helper(first));
            collect_format_args(args_out, rest...);
        }
        catch (...) {
        }
    }

    // 포맷 문자열 파싱 엔진 (.cpp 구현)
    std::string format_impl(std::string_view fmt_str, const std::vector<std::string>& args) noexcept;

    // =========================================================================
    // 1. 단일 인자를 스트림에 출력하는 헬퍼 및 템플릿 특화
    // =========================================================================

    // Helper trait: types treated as strings, not containers
    template <typename T>
    struct is_string_like : std::false_type {};
    template <> struct is_string_like<std::string> : std::true_type {};
    template <> struct is_string_like<std::string_view> : std::true_type {};

    // Generic fallback: only for non-iterable types, or string-like types
    template <typename T>
    std::enable_if_t<!is_iterable<T>::value || is_string_like<T>::value>
    write_arg(std::ostream& os, const T& arg) {
        os << arg;
    }

    template <>
    inline void write_arg<std::string>(std::ostream& os, const std::string& arg) {
        os << mino::core::string::to_console_encoding(arg);
    }

    template <>
    inline void write_arg<std::string_view>(std::ostream& os, const std::string_view& arg) {
        os << mino::core::string::to_console_encoding(std::string(arg));
    }

    template <>
    inline void write_arg<const char*>(std::ostream& os, const char* const& arg) {
        if (arg) {
            os << mino::core::string::to_console_encoding(std::string(arg));
        }
    }

    template <size_t N>
    inline void write_arg(std::ostream& os, const char(&arg)[N]) {
        os << mino::core::string::to_console_encoding(std::string(arg));
    }

    template <>
    inline void write_arg<bool>(std::ostream& os, const bool& arg) {
        os << (arg ? "true" : "false");
    }

    // Specialization for iterable container types (vector, map, set, deque,
    // filesystem::path, etc.) — excludes string-like and pair/tuple types
    template <typename T>
    std::enable_if_t<
        is_iterable<T>::value &&
        !is_string_like<T>::value &&
        !is_pair<T>::value &&
        !is_tuple<T>::value>
    write_arg(std::ostream& os, const T& arg) {
        os << to_string_helper(arg);
    }

    // =========================================================================
    // 2. 타입 소거 래퍼
    // =========================================================================

    struct NamedArg {
        std::string_view key;
        const void* data = nullptr;
        void (*printer)(std::ostream&, const void*) = nullptr;

        // .cpp 구현
        NamedArg(std::string_view k, const char* v);

        template <typename T>
        NamedArg(std::string_view k, const T& v)
            : key(k), data(std::addressof(v)),
            printer([](std::ostream& os, const void* ptr) {
            write_arg(os, *static_cast<const T*>(ptr));
                }) {
        }

        // .cpp 구현
        void write(std::ostream& os) const;
    };

    using named_args = std::initializer_list<NamedArg>;

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

    // (B) 이름 기반 플레이스홀더({key}) 파싱 공통 로직
    template <typename LookupFn>
    void format_named_impl(std::ostream& os, std::string_view fmt, LookupFn&& lookup) {
        size_t i = 0;
        while (i < fmt.size()) {
            if (fmt[i] == '{') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '{') {
                    os << '{';
                    i += 2;
                    continue;
                }

                size_t close_pos = fmt.find('}', i + 1);
                if (close_pos == std::string_view::npos) {
                    os << fmt.substr(i);
                    break;
                }

                size_t next_open = fmt.find('{', i + 1);
                if (next_open != std::string_view::npos && next_open < close_pos) {
                    os << '{';
                    i += 1;
                    continue;
                }

                std::string_view key = fmt.substr(i + 1, close_pos - i - 1);

                if (!lookup(key)) {
                    os << fmt.substr(i, close_pos - i + 1);
                }

                i = close_pos + 1;
            }
            else if (fmt[i] == '}') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
                    os << '}';
                    i += 2;
                    continue;
                }

                os << '}';
                i += 1;
            }
            else {
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

    // (C) initializer_list 전용 매핑 헬퍼 (.cpp 구현)
    void format_named_to(std::ostream& os, std::string_view fmt, named_args args);

    // Helper to check if a type is an iterable key-value container (has key_type and mapped_type)
    template <typename T, typename = void>
    struct is_key_value_container : std::false_type {};

    template <typename T>
    struct is_key_value_container<T, std::void_t<
        typename T::key_type,
        typename T::mapped_type>> : std::true_type {};

    // (D1) Single pair specialization (not iterable)
    template <typename K, typename V>
    inline void
    format_named_to(std::ostream& os, std::string_view fmt, const std::pair<K, V>& args) {
        format_named_impl(os, fmt, [&](std::string_view key) {
            std::string key_str = to_string_helper(args.first);
            if (key_str == key) {
                write_arg(os, args.second);
                return true;
            }
            return false;
            });
    }

    // (D2) Iterable key-value containers (maps, etc.)
    template <typename KeyValueContainer>
    std::enable_if_t<is_key_value_container<KeyValueContainer>::value &&
                     !is_pair<KeyValueContainer>::value>
    format_named_to(std::ostream& os, std::string_view fmt, const KeyValueContainer& args) {
        format_named_impl(os, fmt, [&](std::string_view key) {
            for (const auto& [k, v] : args) {
                // Convert key to string for comparison
                std::string key_str = to_string_helper(k);
                if (key_str == key) {
                    write_arg(os, v);
                    return true;
                }
            }
            return false;
            });
    }

} // namespace mino::core::string::detail
