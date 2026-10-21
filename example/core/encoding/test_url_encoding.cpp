#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_url_encoding() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 8. URL 인코딩/디코딩 (RFC 3986 및 공백 처리 옵션) 검증
 
    println("[Test 8] URL Percent Encoding / Decoding");

    std::string raw_query = "name=Mino Kim&id=42&query=c++17 & modern"; // URL에 특수문자, 공백 포함

    // (1) RFC 3986 표준 (%20)
    std::string encoded_std = mce::url_encode(raw_query, /*encode_spaces_as_plus=*/false);
    assert(encoded_std.find("%20") != std::string::npos); // 공백
    assert(encoded_std.find('+') == std::string::npos); // '+' 없음

    std::string decoded_std = mce::url_decode(encoded_std, /*decode_plus_as_space=*/false);
    assert(decoded_std == raw_query);

    // (2) 폼 데이터 / 쿼리 스트링 형태 (+)
    std::string encoded_plus = mce::url_encode(raw_query, /*encode_spaces_as_plus=*/true);
    assert(encoded_plus.find('+') != std::string::npos); // 공백이 '+'로 인코딩됨

    std::string decoded_plus = mce::url_decode(encoded_plus, /*decode_plus_as_space=*/true);
    assert(decoded_plus == raw_query);

    println("  Raw Query    : {}", raw_query);
    println("  Encoded (std): {}", encoded_std);
    println("  Encoded (+):   {}", encoded_plus);
    println("  Result       : PASSED\n");
 

}
