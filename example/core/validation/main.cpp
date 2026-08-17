#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include "mino/core/validation/validation.hpp"

namespace val = mino::core::validation::fluent_validation_wrapper;

// 가독성 높은 출력을 위한 헬퍼 매크로/함수
void expect_test(const std::string& test_name, bool actual, bool expected) {
    if (actual == expected) {
        std::cout << " [PASS] " << test_name << "\n";
    }
    else {
        std::cerr << " [FAIL] " << test_name
            << " (Expected: " << (expected ? "true" : "false")
            << ", Got: " << (actual ? "true" : "false") << ")\n";
        assert(false);
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << " Fluent Validation Wrapper Test Suite\n";
    std::cout << "========================================\n\n";

    // 1. Email 검증
    std::cout << "[1] Testing is_email...\n";
    expect_test("Valid Email (Standard)", val::is_email("user@example.com"), true);
    expect_test("Valid Email (Subdomain & Symbols)", val::is_email("user.name+tag@sub.domain.co.kr"), true);
    expect_test("Invalid Email (No @)", val::is_email("userexample.com"), false);
    expect_test("Invalid Email (No Domain)", val::is_email("user@"), false);
    expect_test("Invalid Email (Empty)", val::is_email(""), false);
    std::cout << "\n";

    // 2. 한국 전화번호 검증
    std::cout << "[2] Testing is_phone_number...\n";
    expect_test("Valid Mobile (010)", val::is_phone_number("010-1234-5678"), true);
    expect_test("Valid Mobile 3자리 국번 (011)", val::is_phone_number("011-123-4567"), true);
    expect_test("Valid Seoul Landline (02)", val::is_phone_number("02-1234-5678"), true);
    expect_test("Valid Regional Landline (031)", val::is_phone_number("031-123-4567"), true);
    expect_test("Invalid Phone (No hyphen)", val::is_phone_number("01012345678"), false);
    expect_test("Invalid Phone (Invalid Prefix)", val::is_phone_number("070-1234-5678"), false);
    expect_test("Invalid Phone (Empty)", val::is_phone_number(""), false);
    std::cout << "\n";

    // 3. URL 검증
    std::cout << "[3] Testing is_url...\n";
    expect_test("Valid HTTP URL", val::is_url("http://example.com"), true);
    expect_test("Valid HTTPS URL with Path & Query", val::is_url("https://www.example.com/path?arg=val#section"), true);
    expect_test("Invalid URL (No Scheme)", val::is_url("www.example.com"), false);
    expect_test("Invalid URL (Plain text)", val::is_url("just_plain_text"), false);
    expect_test("Invalid URL (Empty)", val::is_url(""), false);
    std::cout << "\n";

    // 4. IP 주소 (IPv4) 검증
    std::cout << "[4] Testing is_ip_address...\n";
    expect_test("Valid IPv4 (Standard)", val::is_ip_address("192.168.0.1"), true);
    expect_test("Valid IPv4 (Min/Max boundary)", val::is_ip_address("0.0.0.0"), true);
    expect_test("Valid IPv4 (255 boundary)", val::is_ip_address("255.255.255.255"), true);
    expect_test("Invalid IPv4 (Out of range 256)", val::is_ip_address("256.0.0.1"), false);
    expect_test("Invalid IPv4 (Incomplete)", val::is_ip_address("192.168.1"), false);
    expect_test("Invalid IPv4 (Characters)", val::is_ip_address("192.168.0.abc"), false);
    expect_test("Invalid IPv4 (Empty)", val::is_ip_address(""), false);
    std::cout << "\n";

    // 5. 알파벳 / 영숫자 / 숫자 검증
    std::cout << "[5] Testing is_alpha / is_alphanumeric / is_numeric...\n";
    expect_test("is_alpha (Alpha only)", val::is_alpha("HelloWorld"), true);
    expect_test("is_alpha (Contains digit)", val::is_alpha("Hello123"), false);
    expect_test("is_alpha (Empty)", val::is_alpha(""), false);

    expect_test("is_alphanumeric (Alphanumeric)", val::is_alphanumeric("User1234"), true);
    expect_test("is_alphanumeric (Special char)", val::is_alphanumeric("User_1234"), false);
    expect_test("is_alphanumeric (Empty)", val::is_alphanumeric(""), false);

    expect_test("is_numeric (Digits only)", val::is_numeric("1234567890"), true);
    expect_test("is_numeric (Contains alpha)", val::is_numeric("1234a"), false);
    expect_test("is_numeric (Decimal point not allowed)", val::is_numeric("12.34"), false);
    expect_test("is_numeric (Empty)", val::is_numeric(""), false);
    std::cout << "\n";

    // 6. Base64 검증
    std::cout << "[6] Testing is_base64...\n";
    expect_test("Valid Base64 (No padding)", val::is_base64("AAAA"), true);
    expect_test("Valid Base64 (1 padding)", val::is_base64("AAA="), true);
    expect_test("Valid Base64 (2 padding)", val::is_base64("AA=="), true);
    expect_test("Invalid Base64 (Length not multiple of 4)", val::is_base64("ABC"), false);
    expect_test("Invalid Base64 (Invalid character)", val::is_base64("AA@="), false);
    expect_test("Invalid Base64 (Empty)", val::is_base64(""), false);
    std::cout << "\n";

    // 7. HEX 컬러 검증
    std::cout << "[7] Testing is_hex_color...\n";
    expect_test("Valid HEX (6-digit with #)", val::is_hex_color("#FFFFFF"), true);
    expect_test("Valid HEX (6-digit without #)", val::is_hex_color("1a2B3c"), true);
    expect_test("Valid HEX (3-digit with #)", val::is_hex_color("#abc"), true);
    expect_test("Valid HEX (3-digit without #)", val::is_hex_color("FFF"), true);
    expect_test("Invalid HEX (4-digit)", val::is_hex_color("#1234"), false);
    expect_test("Invalid HEX (Non-hex character)", val::is_hex_color("#ZZZ"), false);
    std::cout << "\n";

    // 8. JSON 구조 검증
    std::cout << "[8] Testing is_json...\n";
    expect_test("Valid JSON Object Structure", val::is_json("{\"key\": \"value\"}"), true);
    expect_test("Valid JSON Array Structure", val::is_json("[1, 2, 3]"), true);
    expect_test("Invalid JSON (Plain string)", val::is_json("Just a string"), false);
    expect_test("Invalid JSON (Mismatched brackets)", val::is_json("{\"key\": 1]"), false);
    expect_test("Invalid JSON (Empty)", val::is_json(""), false);
    std::cout << "\n";

    // 9. 주민등록번호 검증
    std::cout << "[9] Testing is_resident_registration_number...\n";
    // 900101-1234563: 체크섬 검증용 유효한 가상 번호
    // (9*2 + 0*3 + 0*4 + 1*5 + 0*6 + 1*7 + 1*8 + 2*9 + 3*2 + 4*3 + 5*4 + 6*5) = 18+0+0+5+0+7+8+18+6+12+20+30 = 124
    // 124 % 11 = 3 -> (11 - 3) % 10 = 8 -> 끝자리가 8이어야 함: 900101-1234568
    expect_test("Valid RRN (With Hyphen)", val::is_resident_registration_number("900101-1234568"), true);
    expect_test("Valid RRN (Without Hyphen)", val::is_resident_registration_number("9001011234568"), true);
    expect_test("Invalid RRN (Checksum Mismatch)", val::is_resident_registration_number("900101-1234567"), false);
    expect_test("Invalid RRN (Invalid Month)", val::is_resident_registration_number("901301-1234568"), false);
    expect_test("Invalid RRN (Invalid Day/Leap Year Check)", val::is_resident_registration_number("910229-1234568"), false);
    expect_test("Invalid RRN (Invalid Gender Digit)", val::is_resident_registration_number("900101-0234568"), false);
    expect_test("Invalid RRN (Length Short)", val::is_resident_registration_number("900101-12345"), false);
    std::cout << "\n";

    std::cout << "========================================\n";
    std::cout << " All Validation Tests Passed Successfully!\n";
    std::cout << "========================================\n";

    return 0;
}
