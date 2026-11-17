#include "mino/core/endian/lb_endian.hpp"
#include "mino/core/endian/byte_swap.hpp"

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace mino::core::endian {

    std::uint16_t byte_swap::swap_16(std::uint16_t value) noexcept {
#if defined(_MSC_VER)
        return _byteswap_ushort(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(value);
#else
        return static_cast<std::uint16_t>((value << 8) | (value >> 8));
#endif
    }

    std::uint32_t byte_swap::swap_32(std::uint32_t value) noexcept {
#if defined(_MSC_VER)
        return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(value);
#else
        return ((value & 0x000000FFu) << 24) |
            ((value & 0x0000FF00u) << 8) |
            ((value & 0x00FF0000u) >> 8) |
            ((value & 0xFF000000u) >> 24);
#endif
    }

    std::uint64_t byte_swap::swap_64(std::uint64_t value) noexcept {
#if defined(_MSC_VER)
        return _byteswap_uint64(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(value);
#else
        return ((value & 0x00000000000000FFull) << 56) |
            ((value & 0x000000000000FF00ull) << 40) |
            ((value & 0x0000000000FF0000ull) << 24) |
            ((value & 0x00000000FF000000ull) << 8) |
            ((value & 0x000000FF00000000ull) >> 8) |
            ((value & 0x0000FF0000000000ull) >> 24) |
            ((value & 0x00FF000000000000ull) >> 40) |
            ((value & 0xFF00000000000000ull) >> 56);
#endif
    }

    std::uint16_t byte_swap::to_little_16(std::uint16_t value) noexcept {
        return is_little() ? value : swap_16(value);
    }

    std::uint32_t byte_swap::to_little_32(std::uint32_t value) noexcept {
        return is_little() ? value : swap_32(value);
    }

    std::uint64_t byte_swap::to_little_64(std::uint64_t value) noexcept {
        return is_little() ? value : swap_64(value);
    }

    std::uint16_t byte_swap::to_big_16(std::uint16_t value) noexcept {
        return is_big() ? value : swap_16(value);
    }

    std::uint32_t byte_swap::to_big_32(std::uint32_t value) noexcept {
        return is_big() ? value : swap_32(value);
    }

    std::uint64_t byte_swap::to_big_64(std::uint64_t value) noexcept {
        return is_big() ? value : swap_64(value);
    }

} // namespace mino::core::endian
