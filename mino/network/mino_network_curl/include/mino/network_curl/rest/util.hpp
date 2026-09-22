#include <algorithm>
#include <cctype>
#include <string_view>

namespace mino::network_curl::rest {

    // RFC 3986 기준 URL Path에 허용되는 문자인지 검사하는 헬퍼 함수
    bool is_valid_url_path(std::string_view path);
 
} 
