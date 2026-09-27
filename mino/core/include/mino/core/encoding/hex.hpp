#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mino::core::encoding
{
    // 바이트 배열 -> 16진수 문자열 변환 (기본: 소문자)
    std::string to_hex(const std::vector<uint8_t>& data, bool uppercase = false);

    // 16진수 문자열 -> 바이트 배열 변환 (실패 시 빈 벡터 반환)
    std::vector<uint8_t> from_hex(std::string_view hex_str);

    // 안전한 디코드 (성공 시 true 및 out에 저장, 실패 시 false)
    bool from_hex(std::string_view hex_str, std::vector<uint8_t>& out);

} // namespace mino::core::encoding
