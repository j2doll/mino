#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

#include "test_template.hpp"

// 4. KDF (PBKDF2) 테스트
void test_derive_key_pbkdf2() {
    using crypto_hasher = mino::core::hash::crypto_hasher;

    std::cout << "[4] PBKDF2 Test" << std::endl;

    std::string_view password = "strong_password"; // 비밀번호
    std::string_view salt = "random_salt"; // 솔트 값. 솔트 값은 일반적으로 무작위로 생성되어야 하며, 동일한 비밀번호라도 다른 솔트 값으로 인해 다른 결과를 생성함. 
    int iterations = 1000; // 반복 횟수. 반복 횟수는 높을수록 보안성이 증가하지만, 성능에 영향을 줄 수 있음. 일반적으로 1000 이상 권장.

    // 32 바이트 키 요청
    std::string key_32 = crypto_hasher::derive_key_pbkdf2(password, salt, iterations, 32); // 32 바이트 길이로 PBKDF2 키 생성
    expect_eq("PBKDF2 32B Key Length (64)", key_32.length(), 64); // 32 바이트 -> 64자리 16진수
    // NOTE: PBKDF2는 기본적으로 32바이트(64자리 16진수) 길이를 가져야 함

    // 16 바이트 키 요청
    std::string key_16 = crypto_hasher::derive_key_pbkdf2(password, salt, iterations, 16); // 16 바이트 길이로 PBKDF2 키 생성
    expect_eq("PBKDF2 16B Key Length (32)", key_16.length(), 32); // 16 바이트 -> 32자리 16진수
    // NOTE: PBKDF2는 16바이트(32자리 16진수) 길이를 가져야 함

    // PBKDF2 특성상 같은 비밀번호/솔트/반복횟수면 16바이트 결과는 32바이트 결과의 앞부분과 동일해야 함
    expect_eq("PBKDF2 Prefix Match", key_16, key_32.substr(0, 32));

    // 반복 횟수(iterations)가 다르면, 결과가 완전히 달라야 함
    int iterations2 = 2000; // 반복 횟수를 다르게 설정
    std::string key_diff_iter = crypto_hasher::derive_key_pbkdf2(password, salt, iterations2, 32);
    expect_ne("PBKDF2 Iteration Variation Test", key_32, key_diff_iter);

    std::cout << std::endl;
}

