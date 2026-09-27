#include "mino/core/encoding/url.hpp"

#include <cstdint>

namespace mino::core::encoding
{
    namespace
    {
        // RFC 3986 Unreserved Characters: A-Z, a-z, 0-9, '-', '_', '.', '~'
        constexpr bool is_unreserved(char c)
        {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~';
        }

        constexpr int8_t hex_char_to_nibble(char c)
        {
            if (c >= '0' && c <= '9') return static_cast<int8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<int8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<int8_t>(c - 'A' + 10);
            return -1;
        }

        constexpr char s_hex_digits[] = "0123456789ABCDEF";
    } // namespace

    std::string url_encode(std::string_view input, bool encode_spaces_as_plus)
    {
        std::string out;
        out.reserve(input.size());

        for (char c : input)
        {
            if (is_unreserved(c))
            {
                out.push_back(c);
            }
            else if (c == ' ' && encode_spaces_as_plus)
            {
                out.push_back('+');
            }
            else
            {
                auto byte = static_cast<uint8_t>(c);
                out.push_back('%');
                out.push_back(s_hex_digits[(byte >> 4) & 0x0F]);
                out.push_back(s_hex_digits[byte & 0x0F]);
            }
        }

        return out;
    }

    std::string url_decode(std::string_view input, bool decode_plus_as_space)
    {
        std::string out;
        if (!url_decode(input, out, decode_plus_as_space))
        {
            return {};
        }
        return out;
    }

    bool url_decode(std::string_view input, std::string& out, bool decode_plus_as_space)
    {
        out.clear();
        out.reserve(input.size());

        const std::size_t len = input.size();
        for (std::size_t i = 0; i < len; ++i)
        {
            char c = input[i];

            if (c == '%')
            {
                // % 뒤에 최소 2자리의 16진수 문자가 있어야 함
                if (i + 2 >= len)
                {
                    out.clear();
                    return false;
                }

                int8_t high = hex_char_to_nibble(input[i + 1]);
                int8_t low = hex_char_to_nibble(input[i + 2]);

                if (high == -1 || low == -1)
                {
                    out.clear();
                    return false;
                }

                out.push_back(static_cast<char>((high << 4) | low));
                i += 2;
            }
            else if (c == '+' && decode_plus_as_space)
            {
                out.push_back(' ');
            }
            else
            {
                out.push_back(c);
            }
        }

        return true;
    }

} // namespace mino::core::encoding
