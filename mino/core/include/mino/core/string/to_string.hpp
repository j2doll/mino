#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <typeinfo>
#include <type_traits>
#include <limits>

#include "mino/core/string/u8.hpp"

namespace mino::core::string {

    // 부동소수점 타입(float, double) 전용 to_string (정밀도 미지정)  
    template <typename T>
    std::string to_string(typename std::enable_if< std::is_floating_point<T>::value, T >::type value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
        std::string ret = oss.str();
        return ret;
    }

    // 부동소수점 타입(float, double) 전용 to_string (정밀도 지정)
    template <typename T>
    std::string to_string(typename std::enable_if< std::is_floating_point<T>::value, T >::type value, long long precision)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << value;
        std::string ret = oss.str();
        return ret;
    }

    // 부동소수점 타입(float, double)이 동일한 값인지 확인 (정밀도 지정)
    template <typename T>
    bool is_equal(T value1, T value2, long long precision)
    {
        std::string str1 = to_string<T>(value1, precision);
        std::string str2 = to_string<T>(value2, precision);

        if (str1 == str2) {
            return true;
        }

        return false;
    }

    // 부동소수점 타입(float, double)이 동일한 값인지 확인 (정밀도 미지정)
    template <typename T>
    bool is_equal(T value1, T value2)
    {
        std::string str1 = to_string<T>(value1);
        std::string str2 = to_string<T>(value2);

        if (str1 == str2) {
            return true;
        }

        return false;
    }

} 