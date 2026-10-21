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

#include "mino/core/string/detail/detail.hpp"

namespace mino::core::string::format {

    /**
     * @brief 문자열 포맷팅 함수 (순서 기반 플레이스홀더: {})
     * @param fmt_str 포맷 문자열 ("Value: {}")
     * @param args 치환할 가변 인자들 (기본형, 컨테이너, std::map 등 모두 지원)
     * @return std::string 포맷팅 완료된 문자열
     */
    template <typename... Args>
    inline std::string format(std::string_view fmt_str, const Args&... args) noexcept {
        namespace detail = mino::core::string::detail;
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

    /**
     * @brief 이름 기반 플레이스홀더({key}) 포맷팅 함수 (initializer_list 지원)
     * @param fmt_str 포맷 문자열 ("Hello {name}, {world}")
     * @param args 치환할 키-값 목록 ({ {"name", "John"}, {"world", str1} })
     * @return std::string 포맷팅 완료된 문자열
     */
    inline std::string formatss(std::string_view fmt_str, mino::core::string::detail::named_args args) {
        namespace detail = mino::core::string::detail;
        std::ostringstream oss;
        detail::format_named_to(oss, fmt_str, args);
        return oss.str();
    }

    /**
     * @brief 이름 기반 플레이스홀더({key}) 포맷팅 함수 (std::map, std::unordered_map 등 키-값 컨테이너 지원)
     * @param fmt_str 포맷 문자열 ("Server: {HOST}:{PORT}")
     * @param args 키-값 쌍을 가진 컨테이너
     * @return std::string 포맷팅 완료된 문자열
     */
    template <typename KeyValueContainer>
    inline std::enable_if_t<mino::core::string::detail::is_key_value_container<KeyValueContainer>::value, std::string>
        formatss(std::string_view fmt_str, const KeyValueContainer& args) {
        namespace detail = mino::core::string::detail;
        std::ostringstream oss;
        detail::format_named_to(oss, fmt_str, args);
        return oss.str();
    }

} // namespace mino::core::string::format
