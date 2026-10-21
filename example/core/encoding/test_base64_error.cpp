#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_base64_error() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 3. 잘못된 형식의 Base64 입력 시 예외/에러 처리 검증

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
        assert(!success); // 실패함
        assert(out.empty()); // 출력 벡터가 비워져야 함

        // [2] 디코딩 시 std::vector 반환 오버로드

        std::vector<uint8_t> out_vec = mce::base64_decode(invalid_str);
        assert(out_vec.empty()); // 출력 벡터가 비워져야 함
    }
    println("  Result  : ALL PASSED\n");
}
