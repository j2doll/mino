#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

#include "test_template.hpp"


// 3. HMAC-SHA256 테스트
void test_generate_hmac_sha256() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[3] HMAC-SHA256 Test" << std::endl;

    std::string_view secret_key = "my_secret_key"; // 해시 키
    std::string_view input_text = "hello_world"; // 해시할 데이터

    std::string hash1 = crypto_hasher::generate_hmac_sha256(secret_key, input_text); // HMAC-SHA256 해시 생성
    std::string hash2 = crypto_hasher::generate_hmac_sha256(secret_key, input_text); // 동일한 키와 데이터로 다시 해시 생성

    // 같은 키와 데이터면 결과가 항상 같아야 함
    expect_eq("HMAC Consistency Test", hash1, hash2);

    std::string_view hash_value = "b7dc82d139f377aaf175b746e2d79e3047f162813aad4de7c8de01959c4aec9f";
    expect_eq("HMAC Expected Value Match", hash1, hash_value); // "my_secret_key"와 "hello_world"의 HMAC-SHA256 해시값

    expect_eq("HMAC Length Check (64)", hash1.length(), 64); // 32 바이트 -> 64자리 16진수
    // NOTE: HMAC-SHA256은 항상 32바이트(64자리 16진수) 길이를 가져야 함

    // 다른 키를 사용하면 결과가 달라야 함
    std::string hash_wrong_key = crypto_hasher::generate_hmac_sha256("wrong_key", input_text);
    expect_ne("HMAC Key Variation Test", hash1, hash_wrong_key); // 다른 키를 사용하면 결과가 달라야 함


    std::cout << std::endl;
}

