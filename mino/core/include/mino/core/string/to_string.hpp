#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <limits>

#include "mino/core/string/u8.hpp"

namespace mino::core::string {

    // Floating-point types (float, double) to_string (default precision)
    template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    std::string to_string(T value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
        return oss.str();
    }

    // Floating-point types (float, double) to_string (precision specified)
    template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    std::string to_string(T value, long long precision)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(static_cast<int>(precision)) << value;
        return oss.str();
    }

    // Integral types to_string
    template <typename T, typename std::enable_if_t<std::is_integral<T>::value, int> = 0>
    std::string to_string(T value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    // Fallback for other printable types (e.g., user types with operator<<).
    // Constrain to types that are neither integral nor floating-point to avoid ambiguity.
    template <typename T, typename std::enable_if_t<!std::is_integral<T>::value && !std::is_floating_point<T>::value, int> = 0>
    std::string to_string(const T& value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    // Floating-point equality with precision (uses string comparison of formatted values)
    template <typename T>
    bool is_equal(T value1, T value2, long long precision)
    {
        std::string str1 = to_string<T>(value1, precision);
        std::string str2 = to_string<T>(value2, precision);

        return (str1 == str2);
    }

    // Floating-point equality (default precision)
    template <typename T>
    bool is_equal(T value1, T value2)
    {
        std::string str1 = to_string<T>(value1);
        std::string str2 = to_string<T>(value2);

        return (str1 == str2);
    }

}
