#include <vector>
#include <string>
#include <cassert>

#include "mino/core/encoding/encoding.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/print/print.hpp"

extern void test_base64_encoding_decoding();
extern void test_rfc_4648_encoding_decoding();
extern void test_base64_error();
extern void test_base64_binary();
extern void test_hex_error();

extern void test_hex();

extern void test_url_encoding();
extern void test_url_korean();
extern void test_invalid_url();

int main() {
    namespace mce = mino::core::encoding;
    namespace mcsu8 = mino::core::string::u8;
    using mino::core::string::print::println;

    println("=== Encoding & u8str Unit Test Start ===\n");

    // =========================================================================
    // Base64 테스트 섹션
    // =========================================================================

    test_base64_encoding_decoding(); // 1. 기본 문자열 인코딩 / 디코딩 테스트
    test_rfc_4648_encoding_decoding(); // 2. RFC 4648 표준 테스트 벡터 및 패딩 경우의 수 검증
    test_base64_error(); // 3. 잘못된 형식의 Base64 입력 시 예외/에러 처리 검증 
    test_base64_binary(); // 4. 바이너리 데이터 테스트
    // skip 5

    // =========================================================================
    // 16진수(Hex) 변환 테스트 섹션
    // =========================================================================

    test_hex(); // 6. Hex 상호 변환 (to_hex, from_hex) 기본 및 대소문자 검증
    test_hex_error(); // 7. Hex 비정상 입력 에러 처리 검증

    // =========================================================================
    // URL (Percent) 인코딩 / 디코딩 테스트 섹션
    // =========================================================================

    test_url_encoding(); // 8. URL 인코딩/디코딩 (RFC 3986 및 공백 처리 옵션) 검증
    test_url_korean(); // 9. UTF-8 한글 문자열 URL 인코딩 / 디코딩 통합 검증
    test_invalid_url(); // 10. URL 디코딩 비정상 입력 에러 처리 검증

    println("=== All tests completed successfully! ===");
    return 0;
}
