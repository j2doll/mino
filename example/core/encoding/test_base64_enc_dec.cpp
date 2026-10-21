#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_base64_encoding_decoding() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 1. 기본 문자열 인코딩 / 디코딩 테스트

    auto to_u8_string
        = [](auto&& str) { return mcsu8::to_u8_string(std::forward<decltype(str)>(str)); };

    std::string original = "Hello, World!";

    auto u8_str = to_u8_string(original); // utf8 문자열로 변환
    std::vector<uint8_t> data(u8_str.begin(), u8_str.end()); // std::vector<uint8_t>로 변환

    std::string encoded = mce::base64_encode(data); // Base64 인코딩
    println("[Test 1] Base64 Basic Text");
    println("  Original: {}", original);
    println("  Encoded : {}", encoded);

    std::vector<uint8_t> decoded_vec = mce::base64_decode(encoded); // Base64 디코딩

    // to_string() 대체: mcsu8::to_std_string 이용
    std::string decoded_str = mcsu8::to_std_string({ decoded_vec.begin(), decoded_vec.end() });
    println("  Decoded : {}", decoded_str);

    assert(decoded_str == original); // 디코딩 결과가 원본과 동일해야 함
    println("  Result  : PASSED\n");
}


