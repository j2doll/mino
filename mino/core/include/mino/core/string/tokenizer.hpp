#pragma once

#include <string>
#include <vector>

namespace mino::core::string {

    /**
     * @brief STL만을 사용하여 입력 문자열을 지정된 구분자 기준으로 분리합니다.
     * @param input_target 분리할 원본 문자열
     * @param dropped_delims 분리 기준이 되는 구분자 문자열 (예: ",\n\r")
     * @return 분리된 토큰들의 벡터 (연속된 구분자 사이의 빈 토큰 포함)
     */
     std::vector<std::string> tokenize_string(const std::string& input_target, const std::string& dropped_delims);

}  
