#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_hex_error() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 7. Hex 비정상 입력 에러 처리 검증
 
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
