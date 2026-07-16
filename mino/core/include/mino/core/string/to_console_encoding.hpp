#pragma once

#include <string>

#include "mino/core/string/u8.hpp"

namespace mino::core::string {

    /// UTF-8 문자열을 콘솔에 적합한 인코딩으로 변환하여 반환합니다.
    /// - Windows: EUC-KR(51949)로 변환 → 실패 시 CP949(949)로 변환 → 실패 시 UTF-8 반환
    /// - Linux/macOS: 입력 그대로(UTF-8) 반환
    /// 주의: 반환값은 변환 결과 바이트열이므로, std::cout 등에 그대로 출력하십시오.
     std::string to_console_encoding(const std::string& utf8_string);

    /// 콘솔용 바이트열을 UTF-8로 변환하여 반환합니다.
    /// - Windows: EUC-KR(51949)로 해석 시도 → 실패 시 CP949(949)로 해석 → 실패 시 입력 그대로 반환
    /// - Linux/macOS: 입력 그대로(UTF-8) 반환
    /// 주의: 입력은 콘솔에서 읽어온 바이트열이어야 합니다.
     std::string from_console_encoding(const std::string& console_bytes);


} 
