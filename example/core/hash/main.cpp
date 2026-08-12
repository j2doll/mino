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

// 1. MD5 테스트
void test_generate_md5() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[1] MD5 Test" << std::endl;

    std::string_view input_text = "hello";

    // "hello"의 널리 알려진 실제 MD5 해시값
    std::string expected_hash = "5d41402abc4b2a76b9719d911017c592";

    std::string actual_hash = crypto_hasher::generate_md5(input_text);

    expect_eq("MD5 Value Match", actual_hash, expected_hash);
    expect_eq("MD5 Hex String Length (32)", actual_hash.length(), 32); // 16 바이트 -> 32자리 16진수
    std::cout << std::endl;
}

// 2. SHA-256 테스트 및 크기 조절 테스트
void test_generate_sha256() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[2] SHA-256 Test" << std::endl;

    std::string_view input_text = "hello";

    // "hello"의 널리 알려진 실제 SHA-256 해시값
    std::string expected_full_hash = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

    // 기본 크기 (32 바이트) 테스트
    std::string full_hash = crypto_hasher::generate_sha256(input_text);
    expect_eq("SHA-256 Full Value Match", full_hash, expected_full_hash);
    expect_eq("SHA-256 Full Length (64)", full_hash.length(), 64); // 32 바이트 -> 64자리 16진수

    // 크기 자르기 (16 바이트) 테스트
    std::string truncated_hash = crypto_hasher::generate_sha256(input_text, 16);
    expect_eq("SHA-256 Truncated Length (32)", truncated_hash.length(), 32); // 16 바이트 -> 32자리 16진수

    auto var_sha256_2 = expected_full_hash.substr(0, 32);
    expect_eq("SHA-256 Truncated Prefix Match", truncated_hash, var_sha256_2); // 앞부분이 동일해야 함

    std::cout << std::endl;
}

// 3. HMAC-SHA256 테스트
void test_generate_hmac_sha256() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[3] HMAC-SHA256 Test" << std::endl;

    std::string_view secret_key = "my_secret_key";
    std::string_view input_text = "hello_world";

    std::string_view hash_value = "b7dc82d139f377aaf175b746e2d79e3047f162813aad4de7c8de01959c4aec9f";

    std::string hash1 = crypto_hasher::generate_hmac_sha256(secret_key, input_text);
    std::string hash2 = crypto_hasher::generate_hmac_sha256(secret_key, input_text);

    // 같은 키와 데이터면 결과가 항상 같아야 함
    expect_eq("HMAC Consistency Test", hash1, hash2);
    expect_eq("HMAC Expected Value Match", hash1, hash_value);
    expect_eq("HMAC Length Check (64)", hash1.length(), 64);

    // 다른 키를 사용하면 결과가 달라야 함
    std::string hash_wrong_key = crypto_hasher::generate_hmac_sha256("wrong_key", input_text);
    expect_ne("HMAC Key Variation Test", hash1, hash_wrong_key);
    std::cout << std::endl;
}

// 4. KDF (PBKDF2) 테스트
void test_derive_key_pbkdf2() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[4] PBKDF2 Test" << std::endl;

    std::string_view password = "strong_password";
    std::string_view salt = "random_salt";
    int iterations = 1000;

    // 32 바이트 키 요청
    std::string key_32 = crypto_hasher::derive_key_pbkdf2(password, salt, iterations, 32);
    expect_eq("PBKDF2 32B Key Length (64)", key_32.length(), 64); // 32 바이트 -> 64자리 16진수

    // 16 바이트 키 요청
    std::string key_16 = crypto_hasher::derive_key_pbkdf2(password, salt, iterations, 16);
    expect_eq("PBKDF2 16B Key Length (32)", key_16.length(), 32); // 16 바이트 -> 32자리 16진수

    // PBKDF2 특성상 같은 비밀번호/솔트/반복횟수면 16바이트 결과는 32바이트 결과의 앞부분과 동일해야 함
    expect_eq("PBKDF2 Prefix Match", key_16, key_32.substr(0, 32));

    // 반복 횟수(iterations)가 다르면 결과가 완전히 달라야 함
    std::string key_diff_iter = crypto_hasher::derive_key_pbkdf2(password, salt, 2000, 32);
    expect_ne("PBKDF2 Iteration Variation Test", key_32, key_diff_iter);
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================" << std::endl;
    std::cout << "       CryptoHasher Test Runner          " << std::endl;
    std::cout << "=========================================\n" << std::endl;

    test_generate_md5();
    test_generate_sha256();
    test_generate_hmac_sha256();
    test_derive_key_pbkdf2();

    return 0;
}
