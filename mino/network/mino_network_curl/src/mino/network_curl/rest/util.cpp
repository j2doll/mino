#include "mino/network_curl/rest/util.hpp"

namespace mino::network_curl::rest {

    // RFC 3986 기준 URL Path에 허용되는 문자인지 검사하는 헬퍼 함수
    bool is_valid_url_path(std::string_view path)
    {
        for (std::size_t i = 0; i < path.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(path[i]);

            // 1. 비 ASCII 문자(한글 등 UTF-8 멀티바이트: 128 이상), 공백, 제어 문자 차단
            if (c >= 128 || std::iscntrl(c) || std::isspace(c)) {
                return false;
            }

            // 2. 영문 대소문자 및 숫자 허용
            if (std::isalnum(c)) {
                continue;
            }

            // 3. RFC 3986 Path 허용 특수문자:
            //    - unreserved: '-' | '.' | '_' | '~'
            //    - sub-delims: '!' | '$' | '&' | '\'' | '(' | ')' | '*' | '+' | ',' | ';' | '='
            //    - path delims: '/' | ':' | '@'
            //    - percent-encoding: '%' (단, 뒤에 2자리 16진수가 와야 함)
            switch (c) {
            case '-': case '.': case '_': case '~':
            case '!': case '$': case '&': case '\'': case '(': case ')':
            case '*': case '+': case ',': case ';': case '=':
            case '/': case ':': case '@':
                continue;

            case '%':
                // % 뒤에 2자리의 16진수 문자가 오는지 확인 (Percent-Encoding 검증)
                if (i + 2 < path.size() &&
                    std::isxdigit(static_cast<unsigned char>(path[i + 1])) &&
                    std::isxdigit(static_cast<unsigned char>(path[i + 2]))) {
                    i += 2;
                    continue;
                }
                return false;

            default:
                // '<', '>', '"', '#', '{', '}', '|', '\\', '^', '`', 공백 등 차단
                return false;
            }
        }
        return true;
    }

}

