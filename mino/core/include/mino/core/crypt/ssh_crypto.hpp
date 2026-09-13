#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mino::core::crypt {

    // SHA-256 단방향 해시 (RFC 6234)
    class sha256 {
    private:
        uint32_t s[8];
        uint64_t bit_len;
        uint8_t buf[64];
        size_t buf_len;

        static const uint32_t K[64];
        static uint32_t ror(uint32_t x, uint32_t n);
        void transform(const uint8_t* block);

    public:
        sha256();
        void reset();
        void update(const uint8_t* data, size_t len);
        std::array<uint8_t, 32> finalize();

        static std::array<uint8_t, 32> hash(const uint8_t* data, size_t len);
    };

    // HMAC-SHA256 (RFC 2104)
    std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t klen, const uint8_t* data, size_t dlen);

    // X25519 (Curve25519, RFC 7748)
    namespace x25519 {
        void curve25519(uint8_t* mypublic, const uint8_t* secret, const uint8_t* basepoint);
    }

    // 스트림 오프셋을 보존하는 표준 AES-128-CTR 엔진 (RFC 4344)
    class aes128_ctr {
    private:
        uint8_t round_keys_[176]{};
        std::array<uint8_t, 16> counter_{};
        std::array<uint8_t, 16> keystream_{};
        size_t keystream_pos_{ 16 };

        void encrypt_block(const uint8_t in[16], uint8_t out[16]) const;
        void increment_counter();

    public:
        aes128_ctr();
        void init(const uint8_t* key, const uint8_t* iv);
        void process(const uint8_t* in, uint8_t* out, size_t length);
    };

} // namespace mino::core::crypt
