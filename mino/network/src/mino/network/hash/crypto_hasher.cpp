#include <iomanip>
#include <sstream>
#include <algorithm>

// OpenSSL 헤더 (cpp 내부에서만 처리)
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "mino/network/hash/crypto_hasher.hpp"

namespace mino::network::hash {

    std::string crypto_hasher::to_hex_string(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        for (uint8_t byte : bytes) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

    std::string crypto_hasher::generate_md5(std::string_view message) {
        std::vector<uint8_t> hash_buffer(EVP_MAX_MD_SIZE);
        unsigned int hash_len = 0;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (ctx) {
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
            EVP_DigestUpdate(ctx, message.data(), message.size()); // EVP_DigestUpdate는 size_t를 허용합니다
            EVP_DigestFinal_ex(ctx, hash_buffer.data(), &hash_len);
            EVP_MD_CTX_free(ctx);
        }

        hash_buffer.resize(hash_len);
        return to_hex_string(hash_buffer);
    }

    std::string crypto_hasher::generate_sha256(std::string_view message, size_t output_size) {
        std::vector<uint8_t> hash_buffer(EVP_MAX_MD_SIZE);
        unsigned int hash_len = 0;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (ctx) {
            EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
            EVP_DigestUpdate(ctx, message.data(), message.size());
            EVP_DigestFinal_ex(ctx, hash_buffer.data(), &hash_len);
            EVP_MD_CTX_free(ctx);
        }

        size_t final_size = std::min(static_cast<size_t>(hash_len), output_size);
        hash_buffer.resize(final_size);
        return to_hex_string(hash_buffer);
    }

    std::string crypto_hasher::generate_hmac_sha256(std::string_view key, std::string_view message) {
        std::vector<uint8_t> hash_buffer(EVP_MAX_MD_SIZE);
        unsigned int hash_len = 0;

        HMAC(EVP_sha256(),
            key.data(), static_cast<int>(key.size()), // C4267 해결: size_t -> int 명시적 변환
            reinterpret_cast<const unsigned char*>(message.data()), message.size(),
            hash_buffer.data(), &hash_len);

        hash_buffer.resize(hash_len);
        return to_hex_string(hash_buffer);
    }

    std::string crypto_hasher::derive_key_pbkdf2(
        std::string_view password,
        std::string_view salt,
        int iterations,
        size_t key_length
    ) {
        std::vector<uint8_t> derived_key(key_length);

        int result = PKCS5_PBKDF2_HMAC(
            password.data(), static_cast<int>(password.size()), // C4267 해결: password.size() 형변환
            reinterpret_cast<const unsigned char*>(salt.data()), static_cast<int>(salt.size()), // C4267 해결: salt.size() 형변환
            iterations,
            EVP_sha256(),
            static_cast<int>(key_length), // C4267 해결: key_length 형변환
            derived_key.data()
        );

        if (result != 1) {
            return "";
        }

        return to_hex_string(derived_key);
    }

}
