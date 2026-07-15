#pragma once

#include <charconv>   
#include <cmath>      
#include <cstdint>    
#include <iomanip>    
#include <iostream>
#include <limits>     
#include <optional>   
#include <sstream>    
#include <stdexcept>  
#include <string>
#include <type_traits>

// ==========================================
// 1. String -> Number (double/float/integral) 변환 함수
// ==========================================
// integral 타입은 std::from_chars로 파싱 (예외 없이 실패를 검사).
// floating 타입은 기존 방식(stof/stod)을 사용하여 예외를 처리.
namespace mino::core::convert {

    template <typename T>
    std::enable_if_t<std::is_integral_v<T>, std::optional<T>>
        safe_string_to_num(const std::string& str) {
        if (str.empty())
            return std::nullopt;

        T value{};
        const char* first = str.data();
        const char* last = str.data() + str.size();

        auto res = std::from_chars(first, last, value);
        if (res.ec == std::errc() && res.ptr == last) {
            return value;
        }
        return std::nullopt;
    }

    template <typename T>
    std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>>
        safe_string_to_num(const std::string& str) {
        try {
            if constexpr (std::is_same_v<T, float>) {
                return std::stof(str);
            }
            else {
                return std::stod(str);
            }
        }
        catch (const std::invalid_argument&) {
            return std::nullopt; // 숫자가 아닌 잘못된 문자열일 때
        }
        catch (const std::out_of_range&) {
            return std::nullopt; // 데이터 타입을 초과하는 범위일 때
        }
    }

    // ==========================================
    // 2. Number (int/double/float) -> String 변환 함수
    // ==========================================
    // floating 타입 전용으로 변경: integral은 std::to_string 사용 권장.
    // precision < 0 : use default std::to_string formatting
    // precision >= 0 : format using std::setprecision(precision)
    // use_fixed controls whether std::fixed is applied (default true)
    template <typename T>
    std::enable_if_t<std::is_floating_point_v<T>, std::optional<std::string>>
        safe_num_to_string(T value, int precision = -1, bool use_fixed = true) {
        if (std::isnan(value)) {
            return std::nullopt; // NaN인 경우 유효한 문자열이 없음을 반환
        }

        if (precision < 0) { // 정수 계열 
            return std::to_string(value);
        }

        std::ostringstream oss;
        if (use_fixed) {
            oss << std::fixed;
        }
        oss << std::setprecision(precision) << value;
        return oss.str();
    }

} // namespace mino::core::convert
