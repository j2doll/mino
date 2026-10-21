#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_rfc_4648_encoding_decoding() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 2. RFC 4648 표준 테스트 벡터 및 패딩 경우의 수 검증

    auto to_u8_string
        = [](auto&& str) { return mcsu8::to_u8_string(std::forward<decltype(str)>(str)); };

    struct TestCase {
        std::string input;        // 인코딩할 원본 문자열
        std::string expected_b64; // 기대되는 Base64 인코딩 결과
    };

    std::vector<TestCase> test_cases = {
     // {"input",  "expected_b64"   }, // {인코딩할 원본 문자열, 기대되는 Base64 인코딩 결과}
        {"",       ""               }, // 빈 데이터 (패딩 없음)
        {"f",      "Zg=="           }, // 패딩 2개 (==)
        {"fo",     "Zm8="           }, // 패딩 1개 (=)
        {"foo",    "Zm9v"           }, // 패딩 0개
        {"foob",   "Zm9vYg=="       }, // 패딩 2개 (==)
        {"fooba",  "Zm9vYmE="       }, // 패딩 1개 (=)
        {"foobar", "Zm9vYmFy"       }  // 패딩 0개
    };

    println("[Test 2] Base64 RFC 4648 Test Vectors & Padding");
    for (const auto& tc : test_cases) {

        // TestCase의 input값을 std::vector<uint8_t>로 변환
        auto u8_in = to_u8_string(tc.input);
        std::vector<uint8_t> input_bytes(u8_in.begin(), u8_in.end()); 

        // 인코딩 테스트
        std::string encoded = mce::base64_encode(input_bytes); // Base64 인코딩
        assert(encoded == tc.expected_b64); // 인코딩 결과가 기대값과 동일한지 검사

        // 디코딩 테스트 (std::vector 반환 오버로드)
        std::vector<uint8_t> decoded_vec = mce::base64_decode(tc.expected_b64); // Base64 디코딩
        std::string decoded_str = mcsu8::to_std_string({ decoded_vec.begin(), decoded_vec.end() }); // std::vector<uint8_t> => std::string
        assert(decoded_str == tc.input); // 디코딩 결과가 원본과 동일한지 검사

        // 디코딩 테스트 (bool 참조 반환 오버로드)
        std::vector<uint8_t> decoded_out;
        bool success = mce::base64_decode(tc.expected_b64, decoded_out); // TestCase의 expected_b64를 Base64 디코딩
        assert(success); // 디코딩 성공 여부 확인
        assert(mcsu8::to_std_string({ decoded_out.begin(), decoded_out.end() }) == tc.input); // 디코딩 결과가 원본과 동일한지 검사
    }

    println("  Result  : ALL PASSED\n");

}
