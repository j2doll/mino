#include <iostream>
#include <string>
#include <string_view>

#include "mino/core/hash/crypto_hasher.hpp"

extern void test_generate_md5(); 
extern void test_generate_sha256();
extern void test_generate_hmac_sha256();
extern void test_derive_key_pbkdf2();

int main(int argc, char* argv[]) {
    std::cout << "=========================================" << std::endl;
    std::cout << "       CryptoHasher Test Runner          " << std::endl;
    std::cout << "=========================================\n" << std::endl;

    test_generate_md5(); // 1. MD5 테스트
    test_generate_sha256(); // 2. SHA-256 테스트
    test_generate_hmac_sha256(); // 3. HMAC-SHA256 테스트
    test_derive_key_pbkdf2(); // 4. PBKDF2 테스트

    return 0;
}
