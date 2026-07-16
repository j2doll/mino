#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mino::core::encoding
{
    // 바이트 배열 -> Base64 문자열
     std::string base64_encode(const std::vector<uint8_t>& data);

    // Base64 문자열 -> 바이트 배열
    // 실패 시 빈 벡터 반환
     std::vector<uint8_t> base64_decode(const std::string& b64);

    // 안전한 디코드(결과를 out에 저장, 실패 시 false)
     bool base64_decode(const std::string& b64, std::vector<uint8_t>& out);

} // namespace mino::core::encoding
