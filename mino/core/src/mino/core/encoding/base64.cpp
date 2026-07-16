#include <array>
#include <cctype>

#include "mino/core/encoding/base64.hpp"

namespace mino::core::encoding
{
    namespace {
        constexpr char s_b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr std::size_t s_b64_table_len = 64;
    }

    std::string base64_encode(const std::vector<uint8_t>& data)
    {
        std::string out;
        if (data.empty()) return out;

        out.reserve(((data.size() + 2) / 3) * 4);

        std::size_t i = 0;
        const std::size_t n = data.size();
        while (i + 2 < n) {
            uint32_t triple = (static_cast<uint32_t>(data[i]) << 16) |
                              (static_cast<uint32_t>(data[i + 1]) << 8) |
                              (static_cast<uint32_t>(data[i + 2]));
            out.push_back(s_b64_table[(triple >> 18) & 0x3F]);
            out.push_back(s_b64_table[(triple >> 12) & 0x3F]);
            out.push_back(s_b64_table[(triple >> 6) & 0x3F]);
            out.push_back(s_b64_table[triple & 0x3F]);
            i += 3;
        }

        std::size_t rem = n - i;
        if (rem == 1) {
            uint32_t triple = (static_cast<uint32_t>(data[i]) << 16);
            out.push_back(s_b64_table[(triple >> 18) & 0x3F]);
            out.push_back(s_b64_table[(triple >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
        else if (rem == 2) {
            uint32_t triple = (static_cast<uint32_t>(data[i]) << 16) |
                              (static_cast<uint32_t>(data[i + 1]) << 8);
            out.push_back(s_b64_table[(triple >> 18) & 0x3F]);
            out.push_back(s_b64_table[(triple >> 12) & 0x3F]);
            out.push_back(s_b64_table[(triple >> 6) & 0x3F]);
            out.push_back('=');
        }

        return out;
    }

    std::vector<uint8_t> base64_decode(const std::string& b64)
    {
        std::vector<uint8_t> out;
        if (!base64_decode(b64, out)) return {};
        return out;
    }

    bool base64_decode(const std::string& b64, std::vector<uint8_t>& out)
    {
        out.clear();
        const std::size_t len = b64.size();
        if (len == 0) return true;
        if (len % 4 != 0) return false;

        std::array<int8_t, 256> rev{};
        rev.fill(static_cast<int8_t>(-1));
        for (int i = 0; i < static_cast<int>(s_b64_table_len); ++i) {
            rev[static_cast<unsigned char>(s_b64_table[i])] = static_cast<int8_t>(i);
        }
        rev[static_cast<unsigned char>('=')] = 0;

        out.reserve(((len / 4) * 3));

        for (std::size_t i = 0; i < len; i += 4) {
            // 패딩 위치 검사: '=' 문자는 쿼르텟의 마지막 쿼르텟에서만 허용되며
            // 위치 0 또는 1에 '='가 있으면 무효.
            if (b64[i] == '=' || b64[i + 1] == '=') return false;

            bool pad2 = (b64[i + 2] == '=');
            bool pad1 = (b64[i + 3] == '=');

            // pad2(즉 position 2)가 '='이면 position3도 '='이어야 함
            if (pad2 && !pad1) return false;

            // 패딩이 있으면 반드시 마지막 쿼르텟이어야 함
            if ((pad1 || pad2) && (i + 4 != len)) return false;

            int8_t v[4];
            for (int j = 0; j < 4; ++j) {
                unsigned char c = static_cast<unsigned char>(b64[i + j]);
                int8_t rv = rev[c];
                if (rv == -1) return false; // 유효하지 않은 문자
                v[j] = rv;
            }

            uint32_t triple = (static_cast<uint32_t>(v[0]) << 18) |
                              (static_cast<uint32_t>(v[1]) << 12) |
                              (static_cast<uint32_t>(v[2]) << 6) |
                              static_cast<uint32_t>(v[3]);

            out.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));
            if (!pad2) {
                out.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
            }
            if (!pad1) {
                out.push_back(static_cast<uint8_t>(triple & 0xFF));
            }
        }

        return true;
    }
    
}  