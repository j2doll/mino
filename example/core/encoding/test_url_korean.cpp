#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_url_korean() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 9. UTF-8 한글 문자열 URL 인코딩 / 디코딩 통합 검증

    println("[Test 9] UTF-8 (u8str) URL Encoding Test");

    mcsu8::u8str original_u8("검색어=미노 코어 & test");
    std::string raw_str = original_u8.to_std_string();

    //-----------------------------------------------------
    // RFC 3986(URI Generic Syntax)는 공백(sapce)을 %20 로 인코딩함.
    //  : URL 경로(Path) 부분에는 +를 쓰면 안 됩니다.
    //-----------------------------------------------------
    // HTML 명세(application/x-www-form-urlencoded)는 공백(sapce)을 + 로 인코딩함.
    // 더하기 기호(+)를 전달하고 싶을 때
    //  : application/x-www-form-urlencoded 환경에서는 +가 공백으로 역인코딩(디코딩)
    //-----------------------------------------------------

    bool encode_spaces_as_plus = true;
    // true  : 공백(space)을 '+'로 인코딩할지
    // false : 공백(space)을 %20으로 인코딩할지
    std::string encoded = mce::url_encode(raw_str, encode_spaces_as_plus); // URL 인코딩

    bool decode_plus_as_space = true;
    // true  : '+'를 공백(space)으로 디코딩할지
    // false : %20을 공백(space)으로 디코딩할지
    std::string decoded_str = mce::url_decode(encoded, decode_plus_as_space); // URL 디코딩

    mcsu8::u8str decoded_u8(decoded_str); // std::string => mino::core::string::u8::u8str 변환

    println("  Original: {}", original_u8.to_std_string());
    println("  Encoded : {}", encoded);
    println("  Decoded : {}", decoded_u8.to_std_string());

    assert(original_u8 == decoded_u8); // 디코딩 결과가 원본과 동일해야 함
    println("  Result  : PASSED\n");
}
