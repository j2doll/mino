#include "catch_amalgamated.hpp"
#include "mino/core/bit/bit.hpp"

#include <vector>
#include <cstdint>

using mino::core::bit::bit_array;

TEST_CASE("bit_array - Constructors and Basic Properties", "[mino][bit_array]") {
    // 기본 생성자 검증
    SECTION("Default constructor") {
        bit_array ba;
        REQUIRE(ba.size() == 0);
        REQUIRE(ba.data().empty());
        REQUIRE(ba.to_array().empty());
    }

    // 바이트 벡터 및 비트 크기 기반 생성자 검증
    SECTION("Constructor with bytes and bit size") {
        std::vector<uint8_t> bytes = { 0b10101010, 0b11001100 };
        bit_array ba(bytes, 16);

        REQUIRE(ba.size() == 16);
        REQUIRE(ba.data().size() == 2);
        REQUIRE(ba.data()[0] == 0b10101010);
        REQUIRE(ba.data()[1] == 0b11001100);
    }

    // bits 인자가 0일 때 바이트 크기 * 8로 기본 설정되는지 확인
    SECTION("Constructor with bit size 0 uses byte size * 8") {
        std::vector<uint8_t> bytes = { 0xFF, 0x00 };
        bit_array ba(bytes, 0);

        REQUIRE(ba.size() == 16);
    }

    // clear() 호출 후 상태 초기화 검증
    SECTION("clear method resets state") {
        std::vector<uint8_t> bytes = { 0xAB };
        bit_array ba(bytes, 8);
        REQUIRE(ba.size() == 8);

        ba.clear();
        REQUIRE(ba.size() == 0);
        REQUIRE(ba.data().empty());
    }

    // set_bytes() 및 set_bits() 초기화 검증
    SECTION("set_bytes and set_bits zero-initialization") {
        bit_array ba;

        ba.set_bytes(3);
        REQUIRE(ba.size() == 24);
        REQUIRE(ba.data().size() == 3);
        for (uint8_t b : ba.data()) {
            REQUIRE(b == 0);
        }

        ba.set_bits(11);
        REQUIRE(ba.size() == 11);
        REQUIRE(ba.data().size() == 2); // 11비트는 2바이트 필요
        for (uint8_t b : ba.data()) {
            REQUIRE(b == 0);
        }
    }
}

TEST_CASE("bit_array - to_array and Bit Ordering", "[mino][bit_array]") {
    // MSB 우선(Big-endian bit) 변환 확인
    SECTION("MSB bit ordering conversion") {
        // 0b10000001 = 첫 비트 1, 마지막 비트 1
        std::vector<uint8_t> bytes = { 0b10000001 };
        bit_array ba(bytes, 8);

        std::vector<bool> expected = { true, false, false, false, false, false, false, true };
        REQUIRE(ba.to_array() == expected);
    }

    // 부분 바이트 비트 크기 변환 확인
    SECTION("Sub-byte bit size conversion") {
        // 5비트만 사용: 10101...
        std::vector<uint8_t> bytes = { 0b10101111 };
        bit_array ba(bytes, 5);

        std::vector<bool> expected = { true, false, true, false, true };
        REQUIRE(ba.to_array() == expected);
    }
}

TEST_CASE("bit_array - Sub-array Extraction (get)", "[mino][bit_array]") {
    // 0b11010010 = D2
    std::vector<uint8_t> bytes = { 0b11010010 };
    bit_array ba(bytes, 8);

    // 중간 비트 슬라이스 추출 검증
    SECTION("Extract slice within single byte") {
        // offset 2부터 4비트: [0, 1, 0, 0] -> MSB 정렬 시 0b01000000
        bit_array sub = ba.get(2, 4);

        REQUIRE(sub.size() == 4);
        std::vector<bool> expected = { false, true, false, false };
        REQUIRE(sub.to_array() == expected);
    }

    // 바이트 경계를 넘어가는 비트 추출 검증
    SECTION("Extract slice across byte boundaries") {
        std::vector<uint8_t> bytes2 = { 0xF0, 0x0F };
        bit_array ba2(bytes2, 16);

        // offset 6부터 4비트 추출: 6, 7번(0, 0)과 8, 9번(0, 0)
        bit_array sub = ba2.get(6, 4);
        REQUIRE(sub.size() == 4);
        REQUIRE(sub.to_array() == std::vector<bool>{ false, false, false, false });

        // offset 2부터 8비트 추출: 2~7번(110000), 8~9번(00) -> 11000000
        bit_array sub2 = ba2.get(2, 8);
        REQUIRE(sub2.size() == 8);
        REQUIRE(sub2.data()[0] == 0b11000000);
    }

    // 범위 초과 시 자동 클램핑 처리 검증
    SECTION("Automatic clamping on out of bound offsets") {
        // 8비트 중 offset 6부터 요청 시 남은 2비트만 반환
        bit_array sub = ba.get(6, 10);
        REQUIRE(sub.size() == 2);
        std::vector<bool> expected = { true, false };
        REQUIRE(sub.to_array() == expected);

        // 완전 범위 초과 시 빈 객체 반환
        bit_array emptySub = ba.get(10, 5);
        REQUIRE(emptySub.size() == 0);
    }
}

TEST_CASE("bit_array - Merge Operations", "[mino][bit_array]") {
    // merge 정상 동작 검증
    SECTION("Successful bit merge") {
        bit_array ba({ 0x00 }, 8);
        bit_array input({ 0b11100000 }, 3);

        bool ok = ba.merge(input, 2);
        REQUIRE(ok == true);
        REQUIRE(ba.size() == 8);

        // offset 2부터 3개 비트 세팅 -> 0b00111000
        REQUIRE(ba.data()[0] == 0b00111000);
    }

    // merge 범위 초과 시 실패 및 경계 클램핑 검증
    SECTION("Out of bounds merge and boundary handling") {
        bit_array ba({ 0x00 }, 8);
        bit_array input({ 0xFF }, 4);

        // offset이 전체 크기 이상일 경우 실패
        REQUIRE_FALSE(ba.merge(input, 8));

        // 일부만 걸치는 경우 (offset 6에 4비트 머지 -> 2비트만 적용)
        bool ok = ba.merge(input, 6);
        REQUIRE(ok == true);
        REQUIRE(ba.data()[0] == 0b00000011);
    }
}

TEST_CASE("bit_array - Concatenation (operator+)", "[mino][bit_array]") {
    // 두 bit_array 연결 검증
    SECTION("Concatenate two bit_arrays") {
        // ba1: 4비트 0b1011....
        bit_array ba1({ 0b10110000 }, 4);
        // ba2: 5비트 0b11001...
        bit_array ba2({ 0b11001000 }, 5);

        bit_array combined = ba1 + ba2;

        REQUIRE(combined.size() == 9);
        std::vector<bool> expected = {
            true, false, true, true,       // ba1 (4 bits)
            true, true, false, false, true // ba2 (5 bits)
        };
        REQUIRE(combined.to_array() == expected);
    }
}

TEST_CASE("bit_array - Shift Operators", "[mino][bit_array]") {
    // 왼쪽 시프트 (operator<<) 검증
    SECTION("Left shift operator") {
        // [1, 0, 1, 1, 0, 0, 1, 0]
        bit_array ba({ 0b10110010 }, 8);

        bit_array shifted = ba << 2;
        REQUIRE(shifted.size() == 8);
        // 2비트 왼쪽 이동: [1, 1, 0, 0, 1, 0, 0, 0] -> 0b11001000
        REQUIRE(shifted.data()[0] == 0b11001000);

        // 크기 이상의 시프트 시 전체 0으로 설정
        bit_array cleared = ba << 8;
        REQUIRE(cleared.data()[0] == 0x00);
    }

    // 오른쪽 시프트 (operator>>) 검증
    SECTION("Right shift operator") {
        // [1, 0, 1, 1, 0, 0, 1, 0]
        bit_array ba({ 0b10110010 }, 8);

        bit_array shifted = ba >> 2;
        REQUIRE(shifted.size() == 8);
        // 2비트 오른쪽 이동: [0, 0, 1, 0, 1, 1, 0, 0] -> 0b00101100
        REQUIRE(shifted.data()[0] == 0b00101100);

        // 크기 이상의 시프트 시 전체 0으로 설정
        bit_array cleared = ba >> 10;
        REQUIRE(cleared.data()[0] == 0x00);
    }
}

TEST_CASE("bit_array - Inversion and Reversal", "[mino][bit_array]") {
    // bitwise NOT (operator~) 검증
    SECTION("Bitwise NOT operator") {
        // 6비트: 0b101100..
        bit_array ba({ 0b10110000 }, 6);
        bit_array inverted = ~ba;

        REQUIRE(inverted.size() == 6);
        std::vector<bool> expected = { false, true, false, false, true, true };
        REQUIRE(inverted.to_array() == expected);
    }

    // reverser() 비트 순서 반전 검증
    SECTION("Reverser method") {
        // 5비트: [1, 1, 0, 0, 1]
        bit_array ba({ 0b11001000 }, 5);
        ba.reverser();

        REQUIRE(ba.size() == 5);
        std::vector<bool> expected = { true, false, false, true, true };
        REQUIRE(ba.to_array() == expected);
    }

    // 빈 bit_array의 reverser() 호출 안정성 검증
    SECTION("Reverser on empty bit_array") {
        bit_array emptyBa;
        emptyBa.reverser();
        REQUIRE(emptyBa.size() == 0);
    }
}

TEST_CASE("bit_array - Print and Dump Smoke Test", "[mino][bit_array]") {
    // print() 및 dump() 호출 시 예외 없이 실행되는지 검증
    bit_array ba({ 0x12, 0x34, 0xAB, 0xCD }, 32);

    REQUIRE_NOTHROW(ba.print(true));
    REQUIRE_NOTHROW(ba.print(false));
    REQUIRE_NOTHROW(ba.dump());
}
