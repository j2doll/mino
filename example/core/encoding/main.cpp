#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/base64.hpp"
#include "mino/core/string/u8string.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    namespace encoding = mino::core::encoding;
    namespace u8 = mino::core::string::u8;
    using mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("=== Base64 & u8str Unit Test Start ===\n\n");

    // 1. 기본 문자열 인코딩 / 디코딩 테스트
    {
        std::string original = "Hello, World!";

        auto u8_str = u8::to_u8_string(original); // utf8 문자열로 변환
        std::vector<uint8_t> data(u8_str.begin(), u8_str.end()); // std::vector<uint8_t>로 변환

        std::string encoded = encoding::base64_encode(data); // Base64 인코딩
        std::cout << to_console_encoding("[Test 1] Basic Text\n");
        std::cout << "  Original: " << to_console_encoding(original) << "\n";
        std::cout << "  Encoded : " << to_console_encoding(encoded) << "\n";

        std::vector<uint8_t> decoded_vec = encoding::base64_decode(encoded);

        // to_string() 대체: u8::to_std_string 이용
        std::string decoded_str = u8::to_std_string({ decoded_vec.begin(), decoded_vec.end() });
        std::cout << "  Decoded : " << to_console_encoding(decoded_str) << "\n";

        assert(decoded_str == original);
        std::cout << to_console_encoding("  Result  : PASSED\n\n");
    }

    // 2. RFC 4648 표준 테스트 벡터 및 패딩 경우의 수 검증
    {
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

        std::cout << to_console_encoding("[Test 2] RFC 4648 Test Vectors & Padding\n");
        for (const auto& tc : test_cases) {
            auto u8_in = u8::to_u8_string(tc.input);
            std::vector<uint8_t> input_bytes(u8_in.begin(), u8_in.end());

            // 인코딩 테스트
            std::string encoded = encoding::base64_encode(input_bytes);
            assert(encoded == tc.expected_b64);

            // 디코딩 테스트 (std::vector 반환 오버로드)
            std::vector<uint8_t> decoded_vec = encoding::base64_decode(tc.expected_b64);
            std::string decoded_str = u8::to_std_string({ decoded_vec.begin(), decoded_vec.end() });
            assert(decoded_str == tc.input);

            // 디코딩 테스트 (bool 참조 반환 오버로드)
            std::vector<uint8_t> decoded_out;
            bool success = encoding::base64_decode(tc.expected_b64, decoded_out);
            assert(success);
            assert(u8::to_std_string({ decoded_out.begin(), decoded_out.end() }) == tc.input);
        }
        std::cout << to_console_encoding("  Result  : ALL PASSED\n\n");
    }

    // 3. 잘못된 형식의 Base64 입력 시 예외/에러 처리 검증
    {
        std::cout << to_console_encoding("[Test 3] Invalid Base64 Error Handling\n");

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
            std::vector<uint8_t> out;
            bool success = encoding::base64_decode(invalid_str, out);

            // 실패 시 false를 반환하고, 출력 벡터가 비워져야 함
            assert(!success);
            assert(out.empty());

            std::vector<uint8_t> out_vec = encoding::base64_decode(invalid_str);
            assert(out_vec.empty());
        }
        std::cout << to_console_encoding("  Result  : ALL PASSED\n\n");
    }

    // 4. 바이너리 데이터 테스트
    {
        std::cout << to_console_encoding("[Test 4] Binary Data Test\n");
        std::vector<uint8_t> binary_data = { 0x00, 0xFF, 0x80, 0x1F, 0x7E };

        std::string encoded = encoding::base64_encode(binary_data);
        std::vector<uint8_t> decoded = encoding::base64_decode(encoded);

        assert(decoded == binary_data);
        std::cout << to_console_encoding("  Result  : PASSED\n\n");
    }

    // 5. UTF-8 (u8str) 및 콘솔 출력 통합 테스트
    {
        std::cout << to_console_encoding("[Test 5] UTF-8 (u8str) Base64 Test\n");

        u8::u8str original_u8("안녕하세요, Base64 테스트입니다!");
        const auto& val = original_u8.value();

        // Base64 인코딩 (u8string의 반복자를 직접 전달)
        std::string encoded = encoding::base64_encode({ val.begin(), val.end() });

        // Base64 디코딩
        std::vector<uint8_t> decoded_bytes = encoding::base64_decode(encoded);
        u8::u8str decoded_u8(u8::to_std_string({ decoded_bytes.begin(), decoded_bytes.end() }));

        std::cout << to_console_encoding("  Original: ") << to_console_encoding(original_u8.to_std_string()) << "\n";
        std::cout << "  Encoded : " << encoded << "\n";
        std::cout << to_console_encoding("  Decoded : ") << to_console_encoding(decoded_u8.to_std_string()) << "\n";

        assert(original_u8 == decoded_u8);
        std::cout << to_console_encoding("  Result  : PASSED\n\n");
    }

    std::cout << to_console_encoding("=== All tests completed successfully! ===\n");
    return 0;
}
