#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace mino::core::hash {

    class  crypto_hasher {
    private:
        static std::string to_hex_string(const std::vector<uint8_t>& bytes);

    public:
        // 1. 일반 해시 (MD5)
        static std::string generate_md5(std::string_view message);

        // 2. SHA-256 해시 (출력 바이트 크기 설정 지원, 기본값 32)
        static std::string generate_sha256(std::string_view message, size_t output_size = 32);

        // 3. 키 기반 해시 (HMAC-SHA256)
        static std::string generate_hmac_sha256(std::string_view key, std::string_view message);

        // 4. KDF (PBKDF2) (출력 키 크기 설정 지원, 기본값 32)
        static std::string derive_key_pbkdf2(
            std::string_view password,
            std::string_view salt,
            int iterations = 10000,
            size_t key_length = 32
        );
    };

}  

