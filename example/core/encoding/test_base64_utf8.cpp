#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_base64_utf8() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 5. UTF-8 (u8str) 및 콘솔 출력 통합 테스트
 
    println("[Test 5] UTF-8 (u8str) Base64 Test");

    mcsu8::u8str original_u8("안녕하세요, Base64 테스트입니다!"); // mino UTF-8 문자열 생성

    const mcsu8::u8string& val = original_u8.value();
    // std::u8string은 C++20 이상에서만 지원되므로, mino::core::string::u8::u8string을 사용 

    //------------------------------------------------
    // Base64 인코딩 

    std::string encoded = mce::base64_encode({ val.begin(), val.end() });
    // std::u8string 문자열 => Base64 인코딩 => std::string

    //------------------------------------------------
    // Base64 디코딩

    std::vector<uint8_t> decoded_bytes = mce::base64_decode(encoded);
    // std::string => Base64 디코딩 => std::vector<uint8_t>

    std::string decoded_std_string = mcsu8::to_std_string({ decoded_bytes.begin(), decoded_bytes.end() });
    // std::vector<uint8_t> => std::string 변환

    mcsu8::u8str decoded_u8(decoded_std_string);
    // std::string => mino::core::string::u8::u8str 변환  

    println("  Original: {}", original_u8.to_std_string()); // u8string => std::string 변환 후 출력
    println("  Encoded : {}", encoded); // Base64 인코딩 결과 출력
    println("  Decoded : {}", decoded_u8.to_std_string()); // 디코딩 결과 출력

    assert(original_u8 == decoded_u8);
    println("  Result  : PASSED\n");
  
}
