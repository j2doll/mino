#pragma once

#include <cstdint>

namespace mino::core::endian {

    enum class type : std::uint32_t {
        little = 0x00000001, // 1
        big = 0x01000000 // 16777216
    };

    inline constexpr type little = type::little;
    inline constexpr type big = type::big;

    // 런타임 엔디언 조회 함수 선언
    type get_native() noexcept;
    bool is_little() noexcept;
    bool is_big() noexcept;

} // namespace mino::core::endian
