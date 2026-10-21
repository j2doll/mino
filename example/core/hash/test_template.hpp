#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

namespace {
    // 테스트 결과를 출력하고 통과 여부를 검사하는 간단한 헬퍼 함수
    template <typename T, typename U>
    bool expect_eq(const std::string& test_name, const T& actual, const U& expected) {
        if (actual == expected) {
            std::cout << "  [PASS] " << test_name << "\n";
            return true;
        }
        else {
            std::cout << "  [FAIL] " << test_name << "\n";
            std::cout << "    - Actual:   " << actual << "\n";
            std::cout << "    - Expected: " << expected << "\n";
            return false;
        }
    }

    template <typename T, typename U>
    bool expect_ne(const std::string& test_name, const T& actual, const U& expected) {
        if (actual != expected) {
            std::cout << "  [PASS] " << test_name << "\n";
            return true;
        }
        else {
            std::cout << "  [FAIL] " << test_name << "\n";
            std::cout << "    - Actual & Expected values are identical: " << actual << "\n";
            return false;
        }
    }
}

