#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_hex() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 6. Hex 상호 변환 (to_hex, from_hex) 기본 및 대소문자 검증
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
