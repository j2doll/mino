#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8string.hpp"
#include "mino/core/string/print.hpp"

int main() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    println("=== Encoding & u8str Unit Test Start ===\n");

    // =========================================================================
    // Base64 테스트 섹션
    // =========================================================================

    // 1. 기본 문자열 인코딩 / 디코딩 테스트
    {
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

    // 2. RFC 4648 표준 테스트 벡터 및 패딩 경우의 수 검증
    {
         auto to_u8_string
            = [](auto&& str) { return mcsu8::to_u8_string(std::forward<decltype(str)>(str)); };

        struct TestCase {
            std::string input;
            std::string expected_b64;
        };

        std::vector<TestCase> test_cases = {
            {"", ""},                     // 빈 데이터 (패딩 없음)
            {"f", "Zg=="},                 // 패딩 2개
            {"fo", "Zm8="},                // 패딩 1개
            {"foo", "Zm9v"},               // 패딩 0개
            {"foob", "Zm9vYg=="},          // 패딩 2개
            {"fooba", "Zm9vYmE="},         // 패딩 1개
            {"foobar", "Zm9vYmFy"}         // 패딩 0개
        };

        println("[Test 2] Base64 RFC 4648 Test Vectors & Padding");
        for (const auto& tc : test_cases) {
            auto u8_in = to_u8_string(tc.input);
            std::vector<uint8_t> input_bytes(u8_in.begin(), u8_in.end());

            // 인코딩 테스트
            std::string encoded = mce::base64_encode(input_bytes);
            assert(encoded == tc.expected_b64);

            // 디코딩 테스트 (std::vector 반환 오버로드)
            std::vector<uint8_t> decoded_vec = mce::base64_decode(tc.expected_b64);
            std::string decoded_str = mcsu8::to_std_string({ decoded_vec.begin(), decoded_vec.end() });
            assert(decoded_str == tc.input);

            // 디코딩 테스트 (bool 참조 반환 오버로드)
            std::vector<uint8_t> decoded_out;
            bool success = mce::base64_decode(tc.expected_b64, decoded_out);
            assert(success);
            assert(mcsu8::to_std_string({ decoded_out.begin(), decoded_out.end() }) == tc.input);
        }
        println("  Result  : ALL PASSED\n");
    }

    // 3. 잘못된 형식의 Base64 입력 시 예외/에러 처리 검증
    {
        println("[Test 3] Invalid Base64 Error Handling");

        std::vector<std::string> invalid_cases = {
            "abc",             // 길이가 4의 배수가 아님 (길이 3)
            "ab===",           // 길이가 4의 배수가 아님 (길이 5)
            "=abc",            // 첫 번째 위치에 '=' 패딩 포함
            "a=bc",            // 두 번째 위치에 '=' 패딩 포함
            "ab=c",            // 세 번째 위치가 '='이나 네 번째 위치가 '='이 아님
            "Zm9vYg==extra",   // 중간/앞쪽에 패딩이 위치하고 뒤에 문자 추가
            "Zm9v!Zm8="        // 허용되지 않는 특수문자('!') 포함
        };

        for (const auto& invalid_str : invalid_cases) {
            // [1] 디코딩 시 bool 참조 반환 오버로드
            std::vector<uint8_t> out;
            bool success = mce::base64_decode(invalid_str, out);

            // 실패 시 false를 반환하고, 출력 벡터가 비워져야 함
            assert(!success);
            assert(out.empty());

            // [2] 디코딩 시 std::vector 반환 오버로드
            std::vector<uint8_t> out_vec = mce::base64_decode(invalid_str); 
            assert(out_vec.empty());
        }
        println("  Result  : ALL PASSED\n");
    }

    // 4. 바이너리 데이터 테스트
    {
        println("[Test 4] Base64 Binary Data Test");
        std::vector<uint8_t> binary_data = { 0x00, 0xFF, 0x80, 0x1F, 0x7E };

        std::string encoded = mce::base64_encode(binary_data);
        std::vector<uint8_t> decoded = mce::base64_decode(encoded);

        assert(decoded == binary_data);
        println("  Result  : PASSED\n");
    }

    // 5. UTF-8 (u8str) 및 콘솔 출력 통합 테스트
    {
        println("[Test 5] UTF-8 (u8str) Base64 Test");

        mcsu8::u8str original_u8("안녕하세요, Base64 테스트입니다!");
        const auto& val = original_u8.value();

        // Base64 인코딩 (u8string의 반복자를 직접 전달)
        std::string encoded = mce::base64_encode({ val.begin(), val.end() }); // UTF8문자열 => Base64 인코딩

        // Base64 디코딩
        std::vector<uint8_t> decoded_bytes = mce::base64_decode(encoded); // Base64 디코딩 => std::vector<uint8_t>
        mcsu8::u8str decoded_u8(mcsu8::to_std_string({ decoded_bytes.begin(), decoded_bytes.end() })); // std::vector<uint8_t> => UTF8문자열

        println("  Original: {}", original_u8.to_std_string());
        println("  Encoded : {}", encoded);
        println("  Decoded : {}", decoded_u8.to_std_string());

        assert(original_u8 == decoded_u8);
        println("  Result  : PASSED\n");
    }

    // =========================================================================
    // 16진수(Hex) 변환 테스트 섹션
    // =========================================================================

    // 6. Hex 상호 변환 (to_hex, from_hex) 기본 및 대소문자 검증
    {
        println("[Test 6] Hex Conversion & Case Handling");

        std::vector<uint8_t> origin_bytes = { 0x00, 0x0F, 0x10, 0xAB, 0xCD, 0xEF };

        std::string hex_lower = mce::to_hex(origin_bytes, /*uppercase=*/false);
        std::string hex_upper = mce::to_hex(origin_bytes, /*uppercase=*/true);

        println("  Hex (lower): {}", hex_lower);
        println("  Hex (upper): {}", hex_upper);

        assert(hex_lower == "000f10abcdef"); // 소문자 Hex
        assert(hex_upper == "000F10ABCDEF"); // 대문자 Hex 

        // 대소문자 혼합 문자열 디코딩 검증
        std::vector<uint8_t> decoded_from_lower = mce::from_hex(hex_lower);
        std::vector<uint8_t> decoded_from_upper = mce::from_hex(hex_upper);
        std::vector<uint8_t> decoded_from_mixed = mce::from_hex("000F10abCdEf");

        assert(decoded_from_lower == origin_bytes);
        assert(decoded_from_upper == origin_bytes);
        assert(decoded_from_mixed == origin_bytes);

        println("  Result     : PASSED\n");
    }

    // 7. Hex 비정상 입력 에러 처리 검증
    {
        println("[Test 7] Invalid Hex Error Handling");

        std::vector<std::string> invalid_hex_cases = {
            "a",          // 홀수 자릿수 (1자)
            "abc",        // 홀수 자릿수 (3자)
            "0g",         // 16진수가 아닌 문자 포함 ('g')
            "00 11",      // 공백 포함
            "0x12"        // 접두사 포함 시 잘못된 문자('x')로 처리
        };

        for (const auto& invalid_hex : invalid_hex_cases) {
            std::vector<uint8_t> out;
            bool success = mce::from_hex(invalid_hex, out); // 모두 실패하여야 함

            // 실패 시 false 및 빈 벡터 유지
            assert(!success);
            assert(out.empty());

            std::vector<uint8_t> out_vec = mce::from_hex(invalid_hex);
            assert(out_vec.empty());
        }
        println("  Result  : ALL PASSED\n");
    }

    // =========================================================================
    // URL (Percent) 인코딩 / 디코딩 테스트 섹션
    // =========================================================================

    // 8. URL 인코딩/디코딩 (RFC 3986 및 공백 처리 옵션) 검증
    {
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

    // 9. UTF-8 한글 문자열 URL 인코딩 / 디코딩 통합 검증
    {
        println("[Test 9] UTF-8 (u8str) URL Encoding Test");

        mcsu8::u8str original_u8("검색어=미노 코어 & test");
        std::string raw_str = original_u8.to_std_string();

        std::string encoded = mce::url_encode(raw_str, /*encode_spaces_as_plus=*/true);
        std::string decoded_str = mce::url_decode(encoded, /*decode_plus_as_space=*/true);
        mcsu8::u8str decoded_u8(decoded_str);

        println("  Original: {}", original_u8.to_std_string());
        println("  Encoded : {}", encoded);
        println("  Decoded : {}", decoded_u8.to_std_string());

        assert(original_u8 == decoded_u8);
        println("  Result  : PASSED\n");
    }

    // 10. URL 디코딩 비정상 입력 에러 처리 검증
    {
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

    println("=== All tests completed successfully! ===");
    return 0;
}
