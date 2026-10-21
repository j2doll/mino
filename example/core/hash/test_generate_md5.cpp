#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

#include "test_template.hpp"

// 1. MD5 테스트
void test_generate_md5() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[1] MD5 Test" << std::endl;

    std::string_view input_text = "hello";
    std::string actual_hash = crypto_hasher::generate_md5(input_text); // MD5 해시 생성

    std::string expected_hash = "5d41402abc4b2a76b9719d911017c592"; // "hello"의 널리 알려진 실제 MD5 해시값
    expect_eq("MD5 Value Match", actual_hash, expected_hash); // 실제 값과 비교

    expect_eq("MD5 Hex String Length (32)", actual_hash.length(), 32); // 16 바이트 -> 32자리 16진수
    // NOTE: MD5는 항상 16바이트(32자리 16진수) 길이를 가져야 함

    std::cout << std::endl;
}
