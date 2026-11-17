#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mino::core::encoding
{
    // 바이트 배열 -> 16진수 문자열 변환 (기본: 소문자)
    std::string to_hex(const std::vector<uint8_t>& data, bool uppercase = false);

    template <typename T>
    std::string to_hex_string(T value, bool uppercase = true) {
        static_assert(std::is_integral_v<T>, "Only integral types supported");
        std::vector<uint8_t> bytes;
        for (int i = sizeof(T) - 1; i >= 0; --i) {
            bytes.push_back((value >> (i * 8)) & 0xFF);
        }
        return mino::core::encoding::to_hex(bytes, uppercase);
    }

    // 16진수 문자열 -> 바이트 배열 변환 (실패 시 빈 벡터 반환)
    std::vector<uint8_t> from_hex(std::string_view hex_str);

    // 안전한 디코드 (성공 시 true 및 out에 저장, 실패 시 false)
    bool from_hex(std::string_view hex_str, std::vector<uint8_t>& out);

    template <typename T>
    bool from_hex_string(std::string_view hex_str, T& out) {
        static_assert(std::is_integral_v<T>, "Only integral types supported");
        std::vector<uint8_t> bytes = mino::core::encoding::from_hex(hex_str);
        if (bytes.size() != sizeof(T)) {
            return false; // 크기가 맞지 않으면 실패
        }
        out = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            out = (out << 8) | bytes[i];
        }
        return true;
    }

} // namespace mino::core::encoding
