#include "mino/core/encoding/hex.hpp"

#include <array>

namespace mino::core::encoding
{
    namespace
    {
        constexpr char s_hex_lower_digits[] = "0123456789abcdef";
        constexpr char s_hex_upper_digits[] = "0123456789ABCDEF";

        constexpr int8_t hex_char_to_nibble(char c)
        {
            if (c >= '0' && c <= '9') return static_cast<int8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<int8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<int8_t>(c - 'A' + 10);
            return -1;
        }
    } // namespace

    std::string to_hex(const std::vector<uint8_t>& data, bool uppercase)
    {
        std::string out;
        out.reserve(data.size() * 2);

        const char* digits = uppercase ? s_hex_upper_digits : s_hex_lower_digits;
        for (uint8_t byte : data)
        {
            out.push_back(digits[(byte >> 4) & 0x0F]);
            out.push_back(digits[byte & 0x0F]);
        }

        return out;
    }

    std::vector<uint8_t> from_hex(std::string_view hex_str)
    {
        std::vector<uint8_t> out;
        if (!from_hex(hex_str, out))
        {
            return {};
        }
        return out;
    }

    bool from_hex(std::string_view hex_str, std::vector<uint8_t>& out)
    {
        out.clear();

        // 16진수 문자열 길이는 짝수여야 합니다.
        if (hex_str.size() % 2 != 0)
        {
            return false;
        }

        out.reserve(hex_str.size() / 2);

        for (std::size_t i = 0; i < hex_str.size(); i += 2)
        {
            int8_t high = hex_char_to_nibble(hex_str[i]);
            int8_t low = hex_char_to_nibble(hex_str[i + 1]);

            if (high == -1 || low == -1)
            {
                out.clear();
                return false;
            }

            out.push_back(static_cast<uint8_t>((high << 4) | low));
        }

        return true;
    }

} // namespace mino::core::encoding
