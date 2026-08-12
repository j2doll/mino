#include <iomanip>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdint>

#include "mino/core/hash/crypto_hasher.hpp"

namespace {
    // -------------------------------------------------------------------------
    // 1. 순수 C++ MD5 구현체
    // -------------------------------------------------------------------------
    struct PureMD5 {
        uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
        uint32_t count[2] = { 0, 0 };
        uint8_t buffer[64] = { 0 };

        static void transform(uint32_t state[4], const uint8_t block[64]) {
            auto rotate_left = [](uint32_t x, int n) { return (x << n) | (x >> (32 - n)); };
            uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
            for (int i = 0, j = 0; i < 16; ++i, j += 4)
                x[i] = (uint32_t)block[j] | ((uint32_t)block[j + 1] << 8) | ((uint32_t)block[j + 2] << 16) | ((uint32_t)block[j + 3] << 24);

#define FF(a, b, c, d, x, s, ac) { a += ((b & c) | (~b & d)) + x + (uint32_t)(ac); a = rotate_left(a, s) + b; }
#define GG(a, b, c, d, x, s, ac) { a += ((b & d) | (c & ~d)) + x + (uint32_t)(ac); a = rotate_left(a, s) + b; }
#define HH(a, b, c, d, x, s, ac) { a += (b ^ c ^ d) + x + (uint32_t)(ac); a = rotate_left(a, s) + b; }
#define II(a, b, c, d, x, s, ac) { a += (c ^ (b | ~d)) + x + (uint32_t)(ac); a = rotate_left(a, s) + b; }

            FF(a, b, c, d, x[0], 7, 0xd76aa478);   FF(d, a, b, c, x[1], 12, 0xe8c7b756);  FF(c, d, a, b, x[2], 17, 0x242070db);  FF(b, c, d, a, x[3], 22, 0xc1bdceee);
            FF(a, b, c, d, x[4], 7, 0xf57c0faf);   FF(d, a, b, c, x[5], 12, 0x4787c62a);  FF(c, d, a, b, x[6], 17, 0xa8304613);  FF(b, c, d, a, x[7], 22, 0xfd469501);
            FF(a, b, c, d, x[8], 7, 0x698098d8);   FF(d, a, b, c, x[9], 12, 0x8b44f7af);  FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
            FF(a, b, c, d, x[12], 7, 0x6b901122);  FF(d, a, b, c, x[13], 12, 0xfd987193); FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);

            GG(a, b, c, d, x[1], 5, 0xf61e2562);   GG(d, a, b, c, x[6], 9, 0xc040b340);   GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
            GG(a, b, c, d, x[5], 5, 0xd62f105d);   GG(d, a, b, c, x[10], 9, 0x02441453);  GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
            GG(a, b, c, d, x[9], 5, 0x21e1cde6);   GG(d, a, b, c, x[14], 9, 0xc33707d6);  GG(c, d, a, b, x[3], 14, 0xf4d50d87);  GG(b, c, d, a, x[8], 20, 0x455a14ed);
            GG(a, b, c, d, x[13], 5, 0xa9e3e905);  GG(d, a, b, c, x[2], 9, 0xfcefa3f8);   GG(c, d, a, b, x[7], 14, 0x676f02d9);  GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

            HH(a, b, c, d, x[5], 4, 0xfffa3942);   HH(d, a, b, c, x[8], 11, 0x8771f681);  HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
            HH(a, b, c, d, x[1], 4, 0xa4beea44);   HH(d, a, b, c, x[4], 11, 0x4bdecfa9);  HH(c, d, a, b, x[7], 16, 0xf6bb4b60);  HH(b, c, d, a, x[10], 23, 0xbebfbc70);
            HH(a, b, c, d, x[13], 4, 0x289b7ec6);  HH(d, a, b, c, x[16 % 16], 11, 0xeaa127fa); HH(c, d, a, b, x[3], 16, 0xd4ef3085); HH(b, c, d, a, x[6], 23, 0x04881d05);
            HH(a, b, c, d, x[9], 4, 0xd9d4d039);   HH(d, a, b, c, x[12], 11, 0xe6db99e5); HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[2], 23, 0xc4ac5665);

            II(a, b, c, d, x[0], 6, 0xf4292244);   II(d, a, b, c, x[7], 10, 0x432aff97);  II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[5], 21, 0xfc93a039);
            II(a, b, c, d, x[12], 6, 0x655b59c3);  II(d, a, b, c, x[3], 10, 0x8f0ccc92);  II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[1], 21, 0x85845dd1);
            II(a, b, c, d, x[8], 6, 0x6fa87e4f);   II(d, a, b, c, x[15], 10, 0xfe2ce6e0); II(c, d, a, b, x[6], 15, 0xa3014314);  II(b, c, d, a, x[13], 21, 0x4e0811a1);
            II(a, b, c, d, x[4], 6, 0xf7537e82);   II(d, a, b, c, x[11], 10, 0xbd3af235); II(c, d, a, b, x[2], 15, 0x2ad7d2bb);  II(b, c, d, a, x[9], 21, 0xeb86d391);

#undef FF
#undef GG
#undef HH
#undef II

            state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        }

        void update(const uint8_t* input, size_t length) {
            uint32_t index = (count[0] >> 3) & 0x3F;
            if ((count[0] += ((uint32_t)length << 3)) < ((uint32_t)length << 3)) count[1]++;
            count[1] += ((uint32_t)length >> 29);
            size_t partLen = 64 - index, i = 0;
            if (length >= partLen) {
                std::copy(input, input + partLen, &buffer[index]);
                transform(state, buffer);
                for (i = partLen; i + 63 < length; i += 64) transform(state, &input[i]);
                index = 0;
            }
            std::copy(input + i, input + length, &buffer[index]);
        }

        std::vector<uint8_t> final() {
            uint8_t bits[8];
            for (int i = 0; i < 4; ++i) {
                bits[i] = (uint8_t)((count[0] >> (i * 8)) & 0xFF);
                bits[i + 4] = (uint8_t)((count[1] >> (i * 8)) & 0xFF);
            }
            uint32_t index = (count[0] >> 3) & 0x3F;
            uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
            static const uint8_t PADDING[64] = { 0x80 };
            update(PADDING, padLen);
            update(bits, 8);
            std::vector<uint8_t> digest(16);
            for (int i = 0; i < 4; ++i) {
                digest[i * 4] = (uint8_t)(state[i] & 0xFF);
                digest[i * 4 + 1] = (uint8_t)((state[i] >> 8) & 0xFF);
                digest[i * 4 + 2] = (uint8_t)((state[i] >> 16) & 0xFF);
                digest[i * 4 + 3] = (uint8_t)((state[i] >> 24) & 0xFF);
            }
            return digest;
        }
    };

    // -------------------------------------------------------------------------
    // 2. 순수 C++ SHA256 구현체
    // -------------------------------------------------------------------------
    struct PureSHA256 {
        uint32_t state[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
        uint64_t bitlen = 0;
        uint8_t buffer[64] = { 0 };
        size_t datalen = 0;

        static const uint32_t k[64];

        static uint32_t rotr(uint32_t x, uint32_t n) {
            return (x >> n) | (x << ((32u - n) & 31u));
        }

        void transform() {
            uint32_t w[64], a, b, c, d, e, f, g, h;

            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(buffer[i * 4]) << 24) |
                    (static_cast<uint32_t>(buffer[i * 4 + 1]) << 16) |
                    (static_cast<uint32_t>(buffer[i * 4 + 2]) << 8) |
                    (static_cast<uint32_t>(buffer[i * 4 + 3]));
            }

            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            a = state[0]; b = state[1]; c = state[2]; d = state[3];
            e = state[4]; f = state[5]; g = state[6]; h = state[7];

            for (int i = 0; i < 64; ++i) {
                uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + k[i] + w[i];
                uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                h = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            state[0] += a; state[1] += b; state[2] += c; state[3] += d;
            state[4] += e; state[5] += f; state[6] += g; state[7] += h;
        }

        void update(const uint8_t* data, size_t length) {
            for (size_t i = 0; i < length; ++i) {
                buffer[datalen] = data[i];
                datalen++;
                if (datalen == 64) {
                    transform();
                    bitlen += 512;
                    datalen = 0;
                }
            }
        }

        std::vector<uint8_t> final() {
            size_t orig_datalen = datalen;
            uint64_t total_bits = bitlen + (static_cast<uint64_t>(orig_datalen) * 8ull);

            buffer[datalen++] = 0x80;

            if (datalen > 56) {
                while (datalen < 64) buffer[datalen++] = 0x00;
                transform();
                datalen = 0;
            }

            while (datalen < 56) buffer[datalen++] = 0x00;

            for (int i = 7; i >= 0; --i) {
                buffer[56 + (7 - i)] = static_cast<uint8_t>((total_bits >> (i * 8)) & 0xFF);
            }

            transform();

            std::vector<uint8_t> hash(32);
            for (size_t i = 0; i < 8; ++i) {
                hash[i * 4 + 0] = static_cast<uint8_t>((state[i] >> 24) & 0xFF);
                hash[i * 4 + 1] = static_cast<uint8_t>((state[i] >> 16) & 0xFF);
                hash[i * 4 + 2] = static_cast<uint8_t>((state[i] >> 8) & 0xFF);
                hash[i * 4 + 3] = static_cast<uint8_t>(state[i] & 0xFF);
            }

            return hash;
        }
    };

    // SHA-256 라운드 상수 (K[62] = 0xbef9a3f7 적용)
    const uint32_t PureSHA256::k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    // -------------------------------------------------------------------------
    // 3. 순수 C++ HMAC-SHA256 Helper
    // -------------------------------------------------------------------------
    std::vector<uint8_t> pure_hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len) {
        std::vector<uint8_t> k(64, 0);
        if (key_len > 64) {
            PureSHA256 hasher;
            hasher.update(key, key_len);
            auto hashed_key = hasher.final();
            std::copy(hashed_key.begin(), hashed_key.end(), k.begin());
        }
        else {
            std::copy(key, key + key_len, k.begin());
        }

        std::vector<uint8_t> o_key_pad(64), i_key_pad(64);
        for (size_t i = 0; i < 64; ++i) {
            o_key_pad[i] = k[i] ^ 0x5c;
            i_key_pad[i] = k[i] ^ 0x36;
        }

        PureSHA256 inner_hasher;
        inner_hasher.update(i_key_pad.data(), 64);
        inner_hasher.update(data, data_len);
        auto inner_hash = inner_hasher.final();

        PureSHA256 outer_hasher;
        outer_hasher.update(o_key_pad.data(), 64);
        outer_hasher.update(inner_hash.data(), inner_hash.size());
        return outer_hasher.final();
    }

    // -------------------------------------------------------------------------
    // 4. 순수 C++ PBKDF2-HMAC-SHA256 Helper
    // -------------------------------------------------------------------------
    std::vector<uint8_t> pure_pbkdf2_sha256(const uint8_t* password, size_t pass_len, const uint8_t* salt, size_t salt_len, int iterations, size_t key_length) {
        std::vector<uint8_t> derived_key;
        derived_key.reserve(key_length + 32);
        uint32_t block_index = 1;

        while (derived_key.size() < key_length) {
            std::vector<uint8_t> salt_with_index(salt_len + 4);
            std::copy(salt, salt + salt_len, salt_with_index.begin());
            salt_with_index[salt_len] = static_cast<uint8_t>((block_index >> 24) & 0xFF);
            salt_with_index[salt_len + 1] = static_cast<uint8_t>((block_index >> 16) & 0xFF);
            salt_with_index[salt_len + 2] = static_cast<uint8_t>((block_index >> 8) & 0xFF);
            salt_with_index[salt_len + 3] = static_cast<uint8_t>(block_index & 0xFF);

            auto u = pure_hmac_sha256(password, pass_len, salt_with_index.data(), salt_with_index.size());
            auto u_block = u;

            for (int i = 1; i < iterations; ++i) {
                u = pure_hmac_sha256(password, pass_len, u.data(), u.size());
                for (size_t j = 0; j < u_block.size(); ++j) {
                    u_block[j] ^= u[j];
                }
            }

            for (uint8_t b : u_block) {
                derived_key.push_back(b);
            }
            block_index++;
        }

        derived_key.resize(key_length);
        return derived_key;
    }
}

namespace mino::core::hash {

    std::string crypto_hasher::to_hex_string(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        for (uint8_t byte : bytes) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

    std::string crypto_hasher::generate_md5(std::string_view message) {
        PureMD5 hasher;
        hasher.update(reinterpret_cast<const uint8_t*>(message.data()), message.size());
        return to_hex_string(hasher.final());
    }

    std::string crypto_hasher::generate_sha256(std::string_view message, size_t output_size) {
        PureSHA256 hasher;
        hasher.update(reinterpret_cast<const uint8_t*>(message.data()), message.size());
        auto hash_buffer = hasher.final();

        size_t final_size = std::min(hash_buffer.size(), output_size);
        hash_buffer.resize(final_size);
        return to_hex_string(hash_buffer);
    }

    std::string crypto_hasher::generate_hmac_sha256(std::string_view key, std::string_view message) {
        auto hash_buffer = pure_hmac_sha256(
            reinterpret_cast<const uint8_t*>(key.data()), key.size(),
            reinterpret_cast<const uint8_t*>(message.data()), message.size()
        );
        return to_hex_string(hash_buffer);
    }

    std::string crypto_hasher::derive_key_pbkdf2(
        std::string_view password,
        std::string_view salt,
        int iterations,
        size_t key_length
    ) {
        auto derived_key = pure_pbkdf2_sha256(
            reinterpret_cast<const uint8_t*>(password.data()), password.size(),
            reinterpret_cast<const uint8_t*>(salt.data()), salt.size(),
            iterations,
            key_length
        );
        return to_hex_string(derived_key);
    }

} // namespace mino::core::hash
