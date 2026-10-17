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

namespace mino::core::string::format {

    namespace detail {

        // 1. 반복 가능한 컨테이너 판별 (std::begin, std::end 존재 여부)
        template <typename T, typename = void>
        struct is_iterable : std::false_type {};

        template <typename T>
        struct is_iterable<T, std::void_t<
            decltype(std::begin(std::declval<const T&>())),
            decltype(std::end(std::declval<const T&>()))
            >> : std::true_type {};

        // 2. std::pair 판별 트레이트 (안전한 타입 체크)
        template <typename T>
        struct is_pair : std::false_type {};

        template <typename T1, typename T2>
        struct is_pair<std::pair<T1, T2>> : std::true_type {};

        // 3. std::tuple 판별 트레이트
        template <typename T>
        struct is_tuple : std::false_type {};

        template <typename... Ts>
        struct is_tuple<std::tuple<Ts...>> : std::true_type {};

        // 전방 선언
        template <typename T>
        std::string to_string_helper(const T& value) noexcept;

        // std::pair 지원 함수 (map 요소 및 pair 포맷팅)
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
                // 문자열 및 string_view 변환 가능 타입
                if constexpr (std::is_convertible_v<T, std::string_view>) {
                    return std::string(std::string_view(value));
                }
                // std::pair
                else if constexpr (is_pair<T>::value) {
                    return format_pair(value);
                }
                // std::tuple
                else if constexpr (is_tuple<T>::value) {
                    return format_tuple(value);
                }
                // 반복 가능한 STL 컨테이너 (vector, list, deque, array, set, map 등)
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
                // 일반 기본 타입 및 operator<< 오버로딩 객체
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
        inline void collect_format_args(std::vector<std::string>&) noexcept {}

        template <typename First, typename... Rest>
        void collect_format_args(std::vector<std::string>& args_out, const First& first, const Rest&... rest) noexcept {
            try {
                args_out.push_back(to_string_helper(first));
                collect_format_args(args_out, rest...);
            }
            catch (...) {
                // 메모리 할당 실패 시 중단
            }
        }

        // 포맷 문자열 파싱 엔진 (예외를 던지지 않고 에러 발생 시 fallback 마커 삽입)
        inline std::string format_impl(std::string_view fmt_str, const std::vector<std::string>& args) noexcept {
            try {
                std::string result;
                result.reserve(fmt_str.size() + args.size() * 16);

                std::size_t arg_index = 0;
                const std::size_t n = fmt_str.size();

                for (std::size_t i = 0; i < n; ++i) {
                    char ch = fmt_str[i];

                    if (ch == '{') {
                        // 이스케이프 처리: "{{" -> "{"
                        if (i + 1 < n && fmt_str[i + 1] == '{') {
                            result.push_back('{');
                            ++i;
                            continue;
                        }

                        // 플레이스홀더: "{}"
                        if (i + 1 < n && fmt_str[i + 1] == '}') {
                            if (arg_index >= args.size()) {
                                result.append("{missing_arg}");
                            }
                            else {
                                result.append(args[arg_index++]);
                            }
                            ++i;
                            continue;
                        }

                        result.append("{invalid_syntax}");
                    }
                    else if (ch == '}') {
                        // 이스케이프 처리: "}}" -> "}"
                        if (i + 1 < n && fmt_str[i + 1] == '}') {
                            result.push_back('}');
                            ++i;
                            continue;
                        }
                        result.append("{unmatched_bracket}");
                    }
                    else {
                        result.push_back(ch);
                    }
                }

                // 인자가 남았을 경우 접미사 표시
                if (arg_index < args.size()) {
                    result.append(" {extra_args_omitted}");
                }

                return result;
            }
            catch (...) {
                // std::string 할당 실패 등 긴급 상황
                return std::string(fmt_str);
            }
        }

    } // namespace detail

    /**
     * @brief 문자열 포맷팅 함수 (noexcept)
     * @param fmt_str 포맷 문자열 ("Value: {}")
     * @param args 치환할 가변 인자들
     * @return std::string 포맷팅 완료된 문자열 (오류 발생 시 에러 태그 포함 또는 원본 반환)
     */
    template <typename... Args>
    inline std::string format(std::string_view fmt_str, const Args&... args) noexcept {
        if constexpr (sizeof...(args) == 0) {
            return detail::format_impl(fmt_str, {});
        }
        else {
            std::vector<std::string> formatted_args;
            try {
                formatted_args.reserve(sizeof...(args));
                detail::collect_format_args(formatted_args, args...);
            }
            catch (...) {
                // 할당 실패 시 비어있는 상태로 진행
            }
            return detail::format_impl(fmt_str, formatted_args);
        }
    }

} // namespace mino::core::string::format
