#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

#include "mino/core/bit/bit.hpp"
#include "mino/core/string/to_console_encoding.hpp"

using bit_array = mino::core::bit::bit_array;

//------------------------------------------------------------------------------
// 1. 생성자, clear, size, data, from 검증
//------------------------------------------------------------------------------
void test_constructors_and_basic_methods() {
    std::cout << "[RUN] test_constructors_and_basic_methods..." << std::endl;

    // 기본 생성자
    bit_array a;
    assert(a.size() == 0);
    assert(a.data().empty());

    // explicit 생성자 & from()
    std::vector<uint8_t> bytes = { 0xAA, 0x55 }; // 1010 1010   0101 0101
    bit_array b(bytes, 16);
    assert(b.size() == 16);
    assert(b.data() == bytes);

    // bits 매개변수가 0일 때 (자동으로 byte 크기 * 8 비트로 계산)
    bit_array c;
    c.from(bytes, 0);
    assert(c.size() == 16);
    assert(c.data() == bytes);

    // 명시적으로 비트 수를 다르게 지정 (12비트만 사용)
    bit_array d(bytes, 12);
    assert(d.size() == 12);
    assert(d.data() == bytes);

    // clear() 검증
    b.clear();
    assert(b.size() == 0);
    assert(b.data().empty());

    std::cout << "  [PASS] Constructors and basic methods tested successfully.\n";
}

//------------------------------------------------------------------------------
// 2. set_bytes, set_bits 검증
//------------------------------------------------------------------------------
void test_set_bytes_and_bits() {
    std::cout << "[RUN] test_set_bytes_and_bits..." << std::endl;

    bit_array arr;

    // set_bytes
    arr.set_bytes(3);
    assert(arr.size() == 24);
    assert(arr.data().size() == 3);
    for (uint8_t byte : arr.data()) {
        assert(byte == 0x00);
    }

    // set_bits (경계값: 올림 처리 검증)
    arr.set_bits(10); // 10비트 -> 2바이트 필요
    assert(arr.size() == 10);
    assert(arr.data().size() == 2);
    for (uint8_t byte : arr.data()) {
        assert(byte == 0x00);
    }

    arr.set_bits(0);
    assert(arr.size() == 0);
    assert(arr.data().empty());

    std::cout << "  [PASS] set_bytes and set_bits tested successfully.\n";
}

//------------------------------------------------------------------------------
// 3. to_array() 검증
//------------------------------------------------------------------------------
void test_to_array() {
    std::cout << "[RUN] test_to_array..." << std::endl;

    // 0xA5 = 10100101 (8비트)
    std::vector<uint8_t> bytes = { 0xA5 };
    bit_array arr(bytes, 8);

    std::vector<bool> expected = { true, false, true, false, false, true, false, true };
    std::vector<bool> result = arr.to_array();

    assert(result.size() == 8);
    assert(result == expected);

    // 비트 크기가 바이트 경계에 딱 떨어지지 않는 경우 (6비트만)
    bit_array arr6(bytes, 6);
    std::vector<bool> expected6 = { true, false, true, false, false, true };
    assert(arr6.to_array() == expected6);

    std::cout << "  [PASS] to_array tested successfully.\n";
}

//------------------------------------------------------------------------------
// 4. get(bitOffset, bitLength) 검증
//------------------------------------------------------------------------------
void test_get() {
    std::cout << "[RUN] test_get..." << std::endl;

    // 0xAA (10101010), 0x55 (01010101) -> 연속 16비트: 1010 1010 0101 0101
    std::vector<uint8_t> bytes = { 0xAA, 0x55 };
    bit_array arr(bytes, 16);

    // 4번째 비트 오프셋부터 8비트 추출
    // 오프셋 4~11: 1010 (AA 하위4비트) + 0101 (55 상위4비트) => 10100101 (0xA5)
    bit_array sub = arr.get(4, 8);
    assert(sub.size() == 8);
    assert(sub.data().size() == 1);
    assert(sub.data()[0] == 0xA5);

    // 범위 초과(Out of bounds) 처리 검증
    bit_array sub_overflow = arr.get(12, 10); // 12~15비트만 존재하므로 4비트만 추출되어야 함
    assert(sub_overflow.size() == 4);

    bit_array sub_out = arr.get(20, 5); // 오프셋이 전체 크기 이상
    assert(sub_out.size() == 0);

    std::cout << "  [PASS] get tested successfully.\n";
}

//------------------------------------------------------------------------------
// 5. merge(input, bitOffset) 검증
//------------------------------------------------------------------------------
void test_merge() {
    std::cout << "[RUN] test_merge..." << std::endl;

    // 타겟: 0x00 0x00 (16개의 0 비트)
    std::vector<uint8_t> target_bytes = { 0x00, 0x00 };
    bit_array target(target_bytes, 16);

    // 소스: 0xFF (11111111 - 8비트)
    std::vector<uint8_t> src_bytes = { 0xFF };
    bit_array src(src_bytes, 8);

    // 오프셋 4에 병합 -> 0000 1111 1111 0000 (0x0F, 0xF0)
    bool ok = target.merge(src, 4);
    assert(ok == true);
    assert(target.data()[0] == 0x0F);
    assert(target.data()[1] == 0xF0);

    // 범위 벗어난 오프셋 병합 실패 검증
    bool fail = target.merge(src, 20);
    assert(fail == false);

    std::cout << "  [PASS] merge tested successfully.\n";
}

//------------------------------------------------------------------------------
// 6. operator+ (결합 연산자) 검증
//------------------------------------------------------------------------------
void test_operator_plus() {
    std::cout << "[RUN] test_operator_plus..." << std::endl;

    // a = 1010 (4비트, 0xA0)
    bit_array a(std::vector<uint8_t>{ 0xA0 }, 4);
    // b = 0101 (4비트, 0x50)
    bit_array b(std::vector<uint8_t>{ 0x50 }, 4);

    // joined = 10100101 (8비트, 0xA5)
    bit_array joined = a + b;
    assert(joined.size() == 8);
    assert(joined.data().size() == 1);
    assert(joined.data()[0] == 0xA5);

    std::cout << "  [PASS] operator+ tested successfully.\n";
}

//------------------------------------------------------------------------------
// 7. operator<<, operator>> (시프트 연산자) 검증
//------------------------------------------------------------------------------
void test_shift_operators() {
    std::cout << "[RUN] test_shift_operators..." << std::endl;

    // 0xA5 = 10100101 (8비트)
    bit_array arr(std::vector<uint8_t>{ 0xA5 }, 8);

    // Left shift 2비트 -> 10010100 (0x94)
    bit_array shl = arr << 2;
    assert(shl.size() == 8);
    assert(shl.data()[0] == 0x94);

    // Right shift 2비트 -> 00101001 (0x29)
    bit_array shr = arr >> 2;
    assert(shr.size() == 8);
    assert(shr.data()[0] == 0x29);

    // 전체 크기 이상의 시프트 (모두 0이 되어야 함)
    bit_array shl_over = arr << 10;
    assert(shl_over.size() == 8);
    assert(shl_over.data()[0] == 0x00);

    bit_array shr_over = arr >> 10;
    assert(shr_over.size() == 8);
    assert(shr_over.data()[0] == 0x00);

    std::cout << "  [PASS] Shift operators tested successfully.\n";
}

//------------------------------------------------------------------------------
// 8. reverser() 비트 반전 검증
//------------------------------------------------------------------------------
void test_reverser() {
    std::cout << "[RUN] test_reverser..." << std::endl;

    // 0xB0 = 10110000 (4비트만 사용: 1011)
    bit_array arr(std::vector<uint8_t>{ 0xB0 }, 4);

    // 반전 후: 1101 0000 -> 0xD0
    arr.reverser();
    assert(arr.size() == 4);
    assert(arr.data()[0] == 0xD0);

    // 8비트 풀 반전 테스트: 10100101 (0xA5) -> 반전 후 10200101 (0xA5: 대칭)
    bit_array arr2(std::vector<uint8_t>{ 0xA5 }, 8);
    arr2.reverser();
    assert(arr2.data()[0] == 0xA5);

    // 비대칭 8비트: 11000000 (0xC0) -> 반전 후 00000011 (0x03)
    bit_array arr3(std::vector<uint8_t>{ 0xC0 }, 8);
    arr3.reverser();
    assert(arr3.data()[0] == 0x03);

    std::cout << "  [PASS] reverser tested successfully.\n";
}

//------------------------------------------------------------------------------
// 9. print(), dump() 출력 함수 동작 확인
//------------------------------------------------------------------------------
void test_print_and_dump() {
    std::cout << "[RUN] test_print_and_dump (Visual Check)..." << std::endl;

    bit_array arr(std::vector<uint8_t>{ 0xDE, 0xAD, 0xBE, 0xEF }, 32);

    std::cout << "--- print(true) ---" << std::endl;
    arr.print(true);

    std::cout << "--- print(false) ---" << std::endl;
    arr.print(false);

    std::cout << "--- dump() ---" << std::endl;
    arr.dump();

    std::cout << "  [PASS] Visual output tested successfully.\n";
}

//------------------------------------------------------------------------------
// 메인 함수
//------------------------------------------------------------------------------
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Starting bit_array Unit Tests          " << std::endl;
    std::cout << "========================================" << std::endl;

    test_constructors_and_basic_methods();
    test_set_bytes_and_bits();
    test_to_array();
    test_get();
    test_merge();
    test_operator_plus();
    test_shift_operators();
    test_reverser();
    test_print_and_dump();

    std::cout << "========================================" << std::endl;
    std::cout << " All Unit Tests Passed Successfully!    " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
