
#include "mino/core/string/tokenizer.hpp"

namespace mino::core::string {

    std::vector<std::string> tokenize_string(const std::string& input_target, const std::string& dropped_delims) {
        std::vector<std::string> result_tokens;

        // 빈 문자열인 경우 빈 토큰 하나를 반환하거나 바로 종료할 수 있습니다.
        if (input_target.empty()) {
            result_tokens.emplace_back("");
            return result_tokens;
        }

        std::size_t start_pos = 0;

        while (start_pos <= input_target.size()) {
            // 현재 위치(start_pos)부터 구분자 중 하나가 처음으로 나타나는 위치를 찾습니다.
            std::size_t found_pos = input_target.find_first_of(dropped_delims, start_pos);

            // 구분자를 찾지 못한 경우 (문자열의 마지막 토큰 구간)
            if (found_pos == std::string::npos) {
                result_tokens.push_back(input_target.substr(start_pos));
                break;
            }

            // 구분자를 찾은 경우 해당 구간의 문자열을 잘라내어 저장 (빈 문자열도 그대로 포함됨)
            result_tokens.push_back(input_target.substr(start_pos, found_pos - start_pos));

            // 다음 탐색 위치를 구분자 바로 다음 인덱스로 갱신
            start_pos = found_pos + 1;
        }

        return result_tokens;
    }

} // namespace mino::core::string
