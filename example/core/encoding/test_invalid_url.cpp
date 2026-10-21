#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_invalid_url() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 10. URL 디코딩 비정상 입력 에러 처리 검증

    println("[Test 10] Invalid URL Decode Error Handling");

    std::vector<std::string> invalid_urls = {
        "abc%",        // 끝자리에 '%'만 단독으로 옴
        "abc%2",       // '%' 뒤에 1자리만 옴
        "abc%2G",      // 유효하지 않은 16진수 문자 ('G')
        "abc%ZZ"       // 유효하지 않은 16진수 시퀀스
    };

    for (const auto& invalid_url : invalid_urls) {
        std::string out;
        bool success = mce::url_decode(invalid_url, out);

        assert(!success);
        assert(out.empty());

        std::string out_str = mce::url_decode(invalid_url);
        assert(out_str.empty());
    }
    println("  Result  : ALL PASSED\n");

}
