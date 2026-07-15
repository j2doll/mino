#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mino::core::crypt {

    class  cipher {
    public:
        cipher() = default;

        // AES-128 키 설정 (16바이트)
        void set_key(const std::string& key);

        // AES-128 ECB 모드로 문자열 암호화 
        std::string encrypt(const std::string& plainText) const;

        // AES-128 ECB 모드로 16진수 문자열 복호화
        std::string decrypt(const std::string& cipherTextHex) const;

    private:
        std::vector<uint8_t> m_round_keys;
        bool m_is_key_set = false;

        void key_expansion(const uint8_t* key);
        void encrypt_block(const uint8_t* in, uint8_t* out) const;
        void decrypt_block(const uint8_t* in, uint8_t* out) const;

        // AES 내부 연산 헬퍼
        static uint8_t gmix(uint8_t a, uint8_t b);
        static void sub_bytes(uint8_t* state);
        static void inv_sub_bytes(uint8_t* state);
        static void shift_rows(uint8_t* state);
        static void inv_shift_rows(uint8_t* state);
        static void mix_columns(uint8_t* state);
        static void inv_mix_columns(uint8_t* state);
        static void add_round_key(uint8_t* state, const uint8_t* roundKey);
    };

} // namespace mino::core::crypt
