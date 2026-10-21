#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

#include "test_template.hpp"

// 2. SHA-256 테스트 및 크기 조절 테스트
void test_generate_sha256() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[2] SHA-256 Test" << std::endl;

    std::string_view input_text = "hello";
    std::string full_hash = crypto_hasher::generate_sha256(input_text);

    std::string expected_full_hash = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"; // "hello"의 널리 알려진 실제 SHA-256 해시값
    expect_eq("SHA-256 Full Value Match", full_hash, expected_full_hash); // 실제 값과 비교

    expect_eq("SHA-256 Full Length (64)", full_hash.length(), 64); // 32 바이트 -> 64자리 16진수
    // NOTE: SHA-256은 항상 32바이트(64자리 16진수) 길이를 가져야 함

    // 크기 자르기 (16 바이트) 테스트
    std::string truncated_hash = crypto_hasher::generate_sha256(input_text, 16); // 16 바이트 길이로 자른 SHA-256 해시 생성
    expect_eq("SHA-256 Truncated Length (32)", truncated_hash.length(), 32); // 16 바이트 -> 32자리 16진수
    // NOTE: SHA-256을 16바이트로 자르면 32자리 16진수 길이를 가져야 함

    auto var_sha256_2 = expected_full_hash.substr(0, 32); // 32자리(16바이트)로 자른 원래 SHA-256 해시값
    expect_eq("SHA-256 Truncated Prefix Match", truncated_hash, var_sha256_2); // 앞부분이 동일해야 함
    // NOTE: SHA-256을 16바이트로 자르면 원래 해시값의 앞부분과 동일해야 함

    std::cout << std::endl;
}
