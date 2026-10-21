#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

void test_base64_binary() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    // 4. 바이너리 데이터 테스트

    println("[Test 4] Base64 Binary Data Test");
    std::vector<uint8_t> binary_data = { 0x00, 0xFF, 0x80, 0x1F, 0x7E }; // 예시 바이너리 데이터

    std::string encoded = mce::base64_encode(binary_data); // Binary => Base64 인코딩
    std::vector<uint8_t> decoded = mce::base64_decode(encoded); // Base64 => Binary 디코딩

    assert(decoded == binary_data); // 디코딩 결과가 원본 바이너리 데이터와 동일한지 검사
    println("  Result  : PASSED\n");
}
