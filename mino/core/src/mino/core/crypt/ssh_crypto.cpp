#include "mino/core/crypt/ssh_crypto.hpp"

#include <algorithm>
#include <cstring>

namespace mino::core::crypt {

    // --- SHA-256 ---

    const uint32_t sha256::K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t sha256::ror(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    sha256::sha256() {
        reset();
    }

    void sha256::reset() {
        s[0] = 0x6a09e667; s[1] = 0xbb67ae85; s[2] = 0x3c6ef372; s[3] = 0xa54ff53a;
        s[4] = 0x510e527f; s[5] = 0x9b05688c; s[6] = 0x1f83d9ab; s[7] = 0x5be0cd19;
        bit_len = 0;
        buf_len = 0;
    }

    void sha256::transform(const uint8_t* block) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | block[i * 4 + 3];
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t t1 = h + S1 + ch + K[i] + w[i];
            uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        s[0] += a; s[1] += b; s[2] += c; s[3] += d;
        s[4] += e; s[5] += f; s[6] += g; s[7] += h;
    }

    void sha256::update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buf[buf_len++] = data[i];
            if (buf_len == 64) {
                transform(buf);
                bit_len += 512;
                buf_len = 0;
            }
        }
    }

    std::array<uint8_t, 32> sha256::finalize() {
        bit_len += buf_len * 8;
        buf[buf_len++] = 0x80;
        if (buf_len > 56) {
            while (buf_len < 64) buf[buf_len++] = 0;
            transform(buf);
            buf_len = 0;
        }
        while (buf_len < 56) buf[buf_len++] = 0;
        for (int i = 7; i >= 0; --i) buf[56 + (7 - i)] = (bit_len >> (i * 8)) & 0xFF;
        transform(buf);

        std::array<uint8_t, 32> out{};
        for (int i = 0; i < 8; ++i) {
            out[i * 4] = (s[i] >> 24) & 0xFF;
            out[i * 4 + 1] = (s[i] >> 16) & 0xFF;
            out[i * 4 + 2] = (s[i] >> 8) & 0xFF;
            out[i * 4 + 3] = (s[i]) & 0xFF;
        }
        return out;
    }

    std::array<uint8_t, 32> sha256::hash(const uint8_t* data, size_t len) {
        sha256 ctx;
        ctx.update(data, len);
        return ctx.finalize();
    }

    // --- HMAC-SHA256 ---

    std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t klen, const uint8_t* data, size_t dlen) {
        uint8_t k[64] = { 0 };
        if (klen > 64) {
            auto h = sha256::hash(key, klen);
            std::memcpy(k, h.data(), 32);
        }
        else {
            std::memcpy(k, key, klen);
        }

        uint8_t ipad[64], opad[64];
        for (int i = 0; i < 64; ++i) {
            ipad[i] = k[i] ^ 0x36;
            opad[i] = k[i] ^ 0x5c;
        }

        sha256 in_ctx;
        in_ctx.update(ipad, 64);
        in_ctx.update(data, dlen);
        auto in_h = in_ctx.finalize();

        sha256 out_ctx;
        out_ctx.update(opad, 64);
        out_ctx.update(in_h.data(), in_h.size());
        return out_ctx.finalize();
    }

    // --- X25519 (RFC 7748 Montgomery Ladder) ---

    namespace x25519 {
        using gf = int64_t[16];

        static void car25519(gf o) {
            for (int i = 0; i < 16; ++i) {
                o[i] += (1LL << 16);
                int64_t c = o[i] >> 16;
                if (i < 15) {
                    o[i + 1] += c - 1;
                }
                else {
                    o[0] += 38 * (c - 1);
                }
                o[i] -= c << 16;
            }
        }

        static void sel25519(gf p, gf q, int b) {
            int64_t c = ~(static_cast<int64_t>(b) - 1);
            for (int i = 0; i < 16; ++i) {
                int64_t t = c & (p[i] ^ q[i]);
                p[i] ^= t;
                q[i] ^= t;
            }
        }

        static void pack25519(uint8_t* o, const gf n) {
            gf m, t;
            for (int i = 0; i < 16; ++i) t[i] = n[i];
            car25519(t);
            car25519(t);
            car25519(t);
            for (int j = 0; j < 2; ++j) {
                m[0] = t[0] - 0xffed;
                for (int i = 1; i < 15; ++i) {
                    m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
                    m[i - 1] &= 0xffff;
                }
                m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
                m[14] &= 0xffff;
                int b = static_cast<int>((m[15] >> 16) & 1);
                m[15] &= 0xffff;
                sel25519(t, m, 1 - b);
            }
            for (int i = 0; i < 16; ++i) {
                o[2 * i] = static_cast<uint8_t>(t[i] & 0xff);
                o[2 * i + 1] = static_cast<uint8_t>(t[i] >> 8);
            }
        }

        static void unpack25519(gf o, const uint8_t* n) {
            for (int i = 0; i < 16; ++i) {
                o[i] = static_cast<int64_t>(n[2 * i]) + (static_cast<int64_t>(n[2 * i + 1]) << 8);
            }
            o[15] &= 0x7fff;
        }

        static void A(gf o, const gf a, const gf b) {
            for (int i = 0; i < 16; ++i) o[i] = a[i] + b[i];
        }

        static void Z(gf o, const gf a, const gf b) {
            for (int i = 0; i < 16; ++i) o[i] = a[i] - b[i];
        }

        static void M(gf o, const gf a, const gf b) {
            int64_t t[31] = { 0 };
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 16; ++j) {
                    t[i + j] += a[i] * b[j];
                }
            }
            for (int i = 0; i < 15; ++i) {
                t[i] += 38 * t[i + 16];
            }
            for (int i = 0; i < 16; ++i) {
                o[i] = t[i];
            }
            car25519(o);
            car25519(o);
        }

        static void S(gf o, const gf a) {
            M(o, a, a);
        }

        static void inv25519(gf o, const gf i) {
            gf c;
            for (int a = 0; a < 16; ++a) c[a] = i[a];
            for (int a = 253; a >= 0; --a) {
                S(c, c);
                if (a != 2 && a != 4) M(c, c, i);
            }
            for (int a = 0; a < 16; ++a) o[a] = c[a];
        }

        void curve25519(uint8_t* mypublic, const uint8_t* secret, const uint8_t* basepoint) {
            uint8_t e[32];
            std::memcpy(e, secret, 32);
            e[0] &= 248;
            e[31] &= 127;
            e[31] |= 64;

            gf x_1, x_2, z_2, x_3, z_3;
            unpack25519(x_1, basepoint);

            for (int i = 0; i < 16; ++i) {
                x_2[i] = (i == 0) ? 1 : 0;
                z_2[i] = 0;
                x_3[i] = x_1[i];
                z_3[i] = (i == 0) ? 1 : 0;
            }

            gf a24;
            for (int i = 0; i < 16; ++i) a24[i] = 0;
            a24[0] = 56129; // 121665 = 56129 + 1 * 65536
            a24[1] = 1;

            int prev_bit = 0;
            for (int i = 254; i >= 0; --i) {
                int bit = (e[i / 8] >> (i & 7)) & 1;
                int swap = bit ^ prev_bit;
                prev_bit = bit;

                sel25519(x_2, x_3, swap);
                sel25519(z_2, z_3, swap);

                gf a, b, aa, bb, ee, c, d, da, cb, t1, t2;
                A(a, x_2, z_2);      // A = x_2 + z_2
                Z(b, x_2, z_2);      // B = x_2 - z_2
                S(aa, a);            // AA = A^2
                S(bb, b);            // BB = B^2
                Z(ee, aa, bb);       // E = AA - BB

                A(c, x_3, z_3);      // C = x_3 + z_3
                Z(d, x_3, z_3);      // D = x_3 - z_3
                M(da, d, a);         // DA = D * A
                M(cb, c, b);         // CB = C * B

                A(t1, da, cb);       // t1 = DA + CB
                S(x_3, t1);          // x_3 = (DA + CB)^2

                Z(t2, da, cb);       // t2 = DA - CB
                S(t2, t2);           // t2 = (DA - CB)^2
                M(z_3, x_1, t2);     // z_3 = x_1 * (DA - CB)^2

                M(x_2, aa, bb);      // x_2 = AA * BB

                M(t1, a24, ee);      // t1 = a24 * E
                A(t1, aa, t1);       // t1 = AA + a24 * E
                M(z_2, ee, t1);      // z_2 = E * (AA + a24 * E)
            }

            sel25519(x_2, x_3, prev_bit);
            sel25519(z_2, z_3, prev_bit);

            gf z_2_inv;
            inv25519(z_2_inv, z_2);
            M(x_2, x_2, z_2_inv);
            pack25519(mypublic, x_2);
        }
    } // namespace x25519

    // --- AES-128-CTR ---

    static const uint8_t sbox[256] = {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
    };

    static const uint8_t rcon[11] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

    static uint8_t xtime(uint8_t x) {
        return static_cast<uint8_t>((x << 1) ^ (((x >> 7) & 1) * 0x1b));
    }

    aes128_ctr::aes128_ctr() = default;

    void aes128_ctr::init(const uint8_t* key, const uint8_t* iv) {
        std::memcpy(counter_.data(), iv, 16);
        keystream_pos_ = 16;

        for (int i = 0; i < 16; ++i) {
            round_keys_[i] = key[i];
        }
        uint8_t temp[4];
        for (int i = 4; i < 44; ++i) {
            for (int j = 0; j < 4; ++j) {
                temp[j] = round_keys_[(i - 1) * 4 + j];
            }
            if (i % 4 == 0) {
                uint8_t t = temp[0];
                temp[0] = sbox[temp[1]] ^ rcon[i / 4];
                temp[1] = sbox[temp[2]];
                temp[2] = sbox[temp[3]];
                temp[3] = sbox[t];
            }
            for (int j = 0; j < 4; ++j) {
                round_keys_[i * 4 + j] = round_keys_[(i - 4) * 4 + j] ^ temp[j];
            }
        }
    }

    void aes128_ctr::encrypt_block(const uint8_t in[16], uint8_t out[16]) const {
        uint8_t state[16];
        std::memcpy(state, in, 16);

        for (int i = 0; i < 16; ++i) state[i] ^= round_keys_[i];

        for (int r = 1; r < 10; ++r) {
            for (int i = 0; i < 16; ++i) state[i] = sbox[state[i]];

            uint8_t tmp[16];
            tmp[0] = state[0];   tmp[1] = state[5];   tmp[2] = state[10];  tmp[3] = state[15];
            tmp[4] = state[4];   tmp[5] = state[9];   tmp[6] = state[14];  tmp[7] = state[3];
            tmp[8] = state[8];   tmp[9] = state[13];  tmp[10] = state[2];  tmp[11] = state[7];
            tmp[12] = state[12]; tmp[13] = state[1];  tmp[14] = state[6];  tmp[15] = state[11];

            for (int i = 0; i < 4; ++i) {
                uint8_t a0 = tmp[i * 4];
                uint8_t a1 = tmp[i * 4 + 1];
                uint8_t a2 = tmp[i * 4 + 2];
                uint8_t a3 = tmp[i * 4 + 3];
                uint8_t h = a0 ^ a1 ^ a2 ^ a3;
                state[i * 4] = a0 ^ xtime(a0 ^ a1) ^ h;
                state[i * 4 + 1] = a1 ^ xtime(a1 ^ a2) ^ h;
                state[i * 4 + 2] = a2 ^ xtime(a2 ^ a3) ^ h;
                state[i * 4 + 3] = a3 ^ xtime(a3 ^ a0) ^ h;
            }

            const uint8_t* crk = round_keys_ + r * 16;
            for (int i = 0; i < 16; ++i) state[i] ^= crk[i];
        }

        for (int i = 0; i < 16; ++i) state[i] = sbox[state[i]];

        uint8_t tmp[16];
        tmp[0] = state[0];   tmp[1] = state[5];   tmp[2] = state[10];  tmp[3] = state[15];
        tmp[4] = state[4];   tmp[5] = state[9];   tmp[6] = state[14];  tmp[7] = state[3];
        tmp[8] = state[8];   tmp[9] = state[13];  tmp[10] = state[2];  tmp[11] = state[7];
        tmp[12] = state[12]; tmp[13] = state[1];  tmp[14] = state[6];  tmp[15] = state[11];

        const uint8_t* crk = round_keys_ + 160;
        for (int i = 0; i < 16; ++i) out[i] = tmp[i] ^ crk[i];
    }

    void aes128_ctr::increment_counter() {
        for (int i = 15; i >= 0; --i) {
            if (++counter_[i] != 0) break;
        }
    }

    void aes128_ctr::process(const uint8_t* in, uint8_t* out, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            if (keystream_pos_ >= 16) {
                encrypt_block(counter_.data(), keystream_.data());
                increment_counter();
                keystream_pos_ = 0;
            }
            out[i] = in[i] ^ keystream_[keystream_pos_++];
        }
    }

} // namespace mino::core::crypt
