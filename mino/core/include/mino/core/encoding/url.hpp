#pragma once

#include <string>
#include <string_view>

namespace mino::core::encoding
{
    // URL 인코딩 (encode_spaces_as_plus가 true이면 공백을 '+', false이면 '%20'으로 변환)
    std::string url_encode(std::string_view input, bool encode_spaces_as_plus = false);

    // URL 디코딩 (실패 시 빈 문자열 반환)
    std::string url_decode(std::string_view input, bool decode_plus_as_space = true);

    // 안전한 디코드 (유효하지 않은 % 시퀀스 등 오류 시 false 반환)
    bool url_decode(std::string_view input, std::string& out, bool decode_plus_as_space = true);

} // namespace mino::core::encoding
