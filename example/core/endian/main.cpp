#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "mino/core/string/string.hpp"
#include "mino/core/encoding/encoding.hpp"
#include "mino/core/endian/endian.hpp"

int main() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;
    namespace mcenc = mino::core::encoding;
    namespace mce = mino::core::endian;

    {
        // -------------------------------------------------------------------------
        // 1. endian.hpp 모든 public 멤버 테스트
        // -------------------------------------------------------------------------
        mcsp::println("=== 1. endian 식별 테스트 ===");

        using type = mce::type;

        // type 열거형 및 little / big 상수
        constexpr type little_val = type::little;
        static_assert(little_val == mce::little);

        constexpr type big_val = type::big;
        static_assert(big_val == mce::big);

        static_assert(mce::little != mce::big);

        // get_native(), is_little(), is_big()
        const type native = mce::get_native(); // x64 CPU의 Windows/Linux 환경에서는 little 엔디언이 반환됨
        const bool little_flag = mce::is_little();
        const bool big_flag = mce::is_big();

        assert(little_flag != big_flag);
        assert((native == mce::little) == little_flag); // native가 little이면 is_little()이 true, is_big()이 false
        assert((native == mce::big) == big_flag); // native가 big이면 is_big()이 true, is_little()이 false

        mcsp::println("native    : {}", (native == mce::little ? "little" : "big"));
        mcsp::println("is_little : {}", little_flag);
        mcsp::println("is_big    : {}", big_flag);
    }

    constexpr std::uint16_t u16_orig = 0x1234u; // u: unsigned
    constexpr std::uint32_t u32_orig = 0x12345678u; 
    constexpr std::uint64_t u64_orig = 0x0123456789ABCDEFull; // ull: unsigned long long

    using byte_swap = mce::byte_swap;

    const std::uint16_t u16_swapped = byte_swap::swap_16(u16_orig); // 0x 34 12
    const std::uint32_t u32_swapped = byte_swap::swap_32(u32_orig); // 0x 7856 3412
    const std::uint64_t u64_swapped = byte_swap::swap_64(u64_orig); // 0x EFCDAB89 67452301

    {
        // -------------------------------------------------------------------------
        // 2. byte_swap 명시적 비트 크기 메서드 테스트 (swap_16 / 32 / 64)
        // -------------------------------------------------------------------------
        mcsp::println("=== 2. byte_swap 명시적 스왑 테스트 ===");

        assert(u16_swapped == 0x3412u);
        assert(u32_swapped == 0x78563412u);
        assert(u64_swapped == 0xEFCDAB8967452301ull);

        // 이중 스왑 시 원본 복원 검증
        assert(byte_swap::swap_16(u16_swapped) == u16_orig);
        assert(byte_swap::swap_32(u32_swapped) == u32_orig);
        assert(byte_swap::swap_64(u64_swapped) == u64_orig);

        mcsp::println("swap_16 : {} -> {}", mcenc::to_hex_string(u16_orig), mcenc::to_hex_string(u16_swapped));
        mcsp::println("swap_32 : {} -> {}", mcenc::to_hex_string(u32_orig), mcenc::to_hex_string(u32_swapped));
        mcsp::println("swap_64 : {} -> {}", mcenc::to_hex_string(u64_orig), mcenc::to_hex_string(u64_swapped));
    }

    {
        // -------------------------------------------------------------------------
        // 3. byte_swap 명시적 엔디언 변환 테스트 (to_little_* / to_big_*)
        // -------------------------------------------------------------------------

        mcsp::println("=== 3. byte_swap 명시적 엔디언 변환 테스트 ===");

        const std::uint16_t le_16 = byte_swap::to_little_16(u16_orig);
        const std::uint32_t le_32 = byte_swap::to_little_32(u32_orig);
        const std::uint64_t le_64 = byte_swap::to_little_64(u64_orig);

        const std::uint16_t be_16 = byte_swap::to_big_16(u16_orig);
        const std::uint32_t be_32 = byte_swap::to_big_32(u32_orig);
        const std::uint64_t be_64 = byte_swap::to_big_64(u64_orig);

        if (mce::is_little()) {
            assert(le_16 == u16_orig && be_16 == u16_swapped);
            assert(le_32 == u32_orig && be_32 == u32_swapped);
            assert(le_64 == u64_orig && be_64 == u64_swapped);
        }
        else {
            assert(le_16 == u16_swapped && be_16 == u16_orig);
            assert(le_32 == u32_swapped && be_32 == u32_orig);
            assert(le_64 == u64_swapped && be_64 == u64_orig);
        }

        mcsp::println("to_little_32 : {}", mcenc::to_hex_string(le_32));
        mcsp::println("to_big_32    : {}", mcenc::to_hex_string(be_32));
    }

    {
        // -------------------------------------------------------------------------
        // 4. 제네릭 템플릿 메서드 테스트 (swap / to_little / from_little / to_big / from_big)
        // -------------------------------------------------------------------------
        mcsp::println("=== 4. 제네릭 템플릿 메서드 테스트 ===");

        // 1바이트 정수 (변화 없어야 함)
        constexpr std::uint8_t u8_orig = 0xABu;
        constexpr std::int8_t  i8_orig = -42;
        assert(byte_swap::swap(u8_orig) == u8_orig);
        assert(byte_swap::swap(i8_orig) == i8_orig);
        assert(byte_swap::to_little(u8_orig) == u8_orig);
        assert(byte_swap::from_little(u8_orig) == u8_orig);
        assert(byte_swap::to_big(i8_orig) == i8_orig);
        assert(byte_swap::from_big(i8_orig) == i8_orig);

        // 2, 4, 8바이트 부호 없는 정수
        assert(byte_swap::swap(u16_orig) == u16_swapped);
        assert(byte_swap::swap(u32_orig) == u32_swapped);
        assert(byte_swap::swap(u64_orig) == u64_swapped);

        assert(byte_swap::from_little(byte_swap::to_little(u16_orig)) == u16_orig);
        assert(byte_swap::from_little(byte_swap::to_little(u32_orig)) == u32_orig);
        assert(byte_swap::from_little(byte_swap::to_little(u64_orig)) == u64_orig);

        assert(byte_swap::from_big(byte_swap::to_big(u16_orig)) == u16_orig);
        assert(byte_swap::from_big(byte_swap::to_big(u32_orig)) == u32_orig);
        assert(byte_swap::from_big(byte_swap::to_big(u64_orig)) == u64_orig);

        // 2, 4, 8바이트 부호 있는 정수 (음수 부호 비트 보존 검증)
        constexpr std::int16_t i16_orig = -12345;
        constexpr std::int32_t i32_orig = -123456789;
        constexpr std::int64_t i64_orig = -1234567890123456789LL;

        assert(byte_swap::swap(byte_swap::swap(i16_orig)) == i16_orig);
        assert(byte_swap::swap(byte_swap::swap(i32_orig)) == i32_orig);
        assert(byte_swap::swap(byte_swap::swap(i64_orig)) == i64_orig);

        assert(byte_swap::from_little(byte_swap::to_little(i16_orig)) == i16_orig);
        assert(byte_swap::from_little(byte_swap::to_little(i32_orig)) == i32_orig);
        assert(byte_swap::from_little(byte_swap::to_little(i64_orig)) == i64_orig);

        assert(byte_swap::from_big(byte_swap::to_big(i16_orig)) == i16_orig);
        assert(byte_swap::from_big(byte_swap::to_big(i32_orig)) == i32_orig);
        assert(byte_swap::from_big(byte_swap::to_big(i64_orig)) == i64_orig);

        std::cout << std::dec;
        auto tb = byte_swap::to_big(i64_orig);
        auto fb = byte_swap::from_big(tb);
        mcsp::println("int64_t 왕복 변환 (to_big -> from_big) : {} -> {}",
            mcenc::to_hex_string(i64_orig), mcenc::to_hex_string(fb) );
    }

    mcsp::println("모든 테스트를 성공적으로 통과했습니다.");
    return 0;
}
