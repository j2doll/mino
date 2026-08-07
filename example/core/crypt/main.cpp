#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include "mino/core/crypt/cipher.hpp"
#include "mino/core/crypt/cipher_utils.hpp"
#include "mino/core/crypt/keyless_cipher.hpp"

// 콘솔 테스트용 로그 헬퍼
void print_test_header(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << " Test: " << title << "\n";
    std::cout << "========================================\n";
}

// 1. cipher 클래스 퍼블릭 멤버 테스트
void test_cipher() {
    print_test_header("cipher Class Test");

    mino::core::crypt::cipher c;
    std::string key = "MySecretKey12345"; // 16바이트 이상의 키 (AES-128 기준)
    std::string plain_text = "Hello!"; // 평문

    // (1) set_key() 테스트
    c.set_key(key);
    std::cout << "[+] Key set: " << key << std::endl;

    // (2) encrypt() 테스트
    std::string encrypted_hex = c.encrypt(plain_text); // 암호화 후 16진수 문자열 반환
    std::cout << "[+] Original PlainText  : " << plain_text << std::endl;
    std::cout << "[+] Encrypted Hex String: " << encrypted_hex << std::endl;
    assert(!encrypted_hex.empty());

    // (3) decrypt() 테스트
    std::string decrypted_text = c.decrypt(encrypted_hex);
    std::cout << "[+] Decrypted PlainText : " << decrypted_text << std::endl;

    // 검증: 원래 평문과 복호화 결과가 일치해야 함
    assert(plain_text == decrypted_text);
    std::cout << "[=>] PASS: cipher Encrypt & Decrypt Success!" << std::endl;

    // Edge Case: 키 설정 전 동작 검증 (키를 재설정 안한 새 객체)
    mino::core::crypt::cipher unset_c;
    assert(unset_c.encrypt("test").empty()); 
        assert(unset_c.decrypt("test").empty());
        std::cout << "[=>] PASS: Unset key protection handled correctly!" << std::endl;
}

// 2. cipher_utils 클래스 퍼블릭 멤버 테스트
void test_cipher_utils() {
    print_test_header("cipher_utils Class Test");

    using mino::core::crypt::cipher_utils;

    // (1) to_hex() & from_hex() 테스트
    std::vector<uint8_t> raw_bytes = { 0x01, 0x0F, 0xA5, 0xFF, 0x00, 0x42 };
    std::string hex_str = cipher_utils::to_hex(raw_bytes);
    std::cout << "[+] Original Bytes Hex: " << hex_str << std::endl;
    assert(hex_str == "010fa5ff0042");

    std::vector<uint8_t> restored_bytes = cipher_utils::from_hex(hex_str);
    assert(raw_bytes == restored_bytes);
    std::cout << "[=>] PASS: to_hex & from_hex conversion!" << std::endl;

    // (2) add_padding() & remove_padding() 테스트 (PKCS#7)
    std::string input_text = "TestPadding";
    size_t block_size = 16;

    std::vector<uint8_t> padded = cipher_utils::add_padding(input_text, block_size);
    std::cout << "[+] Padded size (multiple of 16): " << padded.size() << std::endl;
    assert(padded.size() % block_size == 0);

    std::string unpadded = cipher_utils::remove_padding(padded);
    std::cout << "[+] Unpadded Text: " << unpadded << std::endl;
    assert(input_text == unpadded);
    std::cout << "[=>] PASS: add_padding & remove_padding!" << std::endl;
}

// 3. keyless_cipher 클래스 퍼블릭 멤버 테스트
void test_keyless_cipher() {
    print_test_header("keyless_cipher Class Test");

    using mino::core::crypt::keyless_cipher;

    std::string original_text = "Hello123!@#";

    // (1) encrypt() 테스트
    std::string encrypted = keyless_cipher::encrypt(original_text); // 키 없이 암호화
    std::cout << "[+] Original Text : " << original_text << std::endl;
    std::cout << "[+] Encrypted Text: " << encrypted << std::endl;
    assert(!encrypted.empty());
    assert(original_text != encrypted);

    // (2) decrypt() 테스트
    std::string decrypted = keyless_cipher::decrypt(encrypted);
    std::cout << "[+] Decrypted Text: " << decrypted << std::endl;
    assert(original_text == decrypted);

    std::cout << "[=>] PASS: keyless_cipher Encrypt & Decrypt Success!" << std::endl;
}

int main() {
    try {
        std::cout << "Starting C++ Crypt Library Tests...\n";

        test_cipher();
        test_cipher_utils();
        test_keyless_cipher();

        std::cout << "\n========================================\n";
        std::cout << " ALL TESTS PASSED SUCCESSFULLY! \n";
        std::cout << "========================================\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
