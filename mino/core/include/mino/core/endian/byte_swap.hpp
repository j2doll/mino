#pragma once

#include <cstdint>
#include <type_traits>

namespace mino::core::endian {

    class byte_swap final {
    public:
        byte_swap() = delete;
        ~byte_swap() = delete;
        byte_swap(const byte_swap&) = delete;
        byte_swap& operator=(const byte_swap&) = delete;

        // 16 / 32 / 64비트 기본 바이트 스왑
        static std::uint16_t swap_16(std::uint16_t value) noexcept;
        static std::uint32_t swap_32(std::uint32_t value) noexcept;
        static std::uint64_t swap_64(std::uint64_t value) noexcept;

        // 16 / 32 / 64비트 엔디언 변환 (구현부는 .cpp에 은닉)
        static std::uint16_t to_little_16(std::uint16_t value) noexcept;
        static std::uint32_t to_little_32(std::uint32_t value) noexcept;
        static std::uint64_t to_little_64(std::uint64_t value) noexcept;

        static std::uint16_t to_big_16(std::uint16_t value) noexcept;
        static std::uint32_t to_big_32(std::uint32_t value) noexcept;
        static std::uint64_t to_big_64(std::uint64_t value) noexcept;

        // 제네릭 정수형 바이트 스왑
        template <typename T>
        static std::enable_if_t<std::is_integral_v<T>, T> swap(T value) noexcept {
            using unsigned_t = std::make_unsigned_t<T>;
            const auto u_val = static_cast<unsigned_t>(value);

            if constexpr (sizeof(T) == 1) {
                return value;
            }
            else if constexpr (sizeof(T) == 2) {
                return static_cast<T>(swap_16(u_val));
            }
            else if constexpr (sizeof(T) == 4) {
                return static_cast<T>(swap_32(u_val));
            }
            else if constexpr (sizeof(T) == 8) {
                return static_cast<T>(swap_64(u_val));
            }
            else {
                static_assert(sizeof(T) <= 8, "1, 2, 4, 8바이트 정수 타입만 지원합니다.");
            }
        }

        // 제네릭 리틀 엔디언 변환
        template <typename T>
        static std::enable_if_t<std::is_integral_v<T>, T> to_little(T value) noexcept {
            using unsigned_t = std::make_unsigned_t<T>;
            const auto u_val = static_cast<unsigned_t>(value);

            if constexpr (sizeof(T) == 1) {
                return value;
            }
            else if constexpr (sizeof(T) == 2) {
                return static_cast<T>(to_little_16(u_val));
            }
            else if constexpr (sizeof(T) == 4) {
                return static_cast<T>(to_little_32(u_val));
            }
            else if constexpr (sizeof(T) == 8) {
                return static_cast<T>(to_little_64(u_val));
            }
            else {
                static_assert(sizeof(T) <= 8, "1, 2, 4, 8바이트 정수 타입만 지원합니다.");
            }
        }

        template <typename T>
        static std::enable_if_t<std::is_integral_v<T>, T> from_little(T value) noexcept {
            return to_little(value);
        }

        // 제네릭 빅 엔디언 변환
        template <typename T>
        static std::enable_if_t<std::is_integral_v<T>, T> to_big(T value) noexcept {
            using unsigned_t = std::make_unsigned_t<T>;
            const auto u_val = static_cast<unsigned_t>(value);

            if constexpr (sizeof(T) == 1) {
                return value;
            }
            else if constexpr (sizeof(T) == 2) {
                return static_cast<T>(to_big_16(u_val));
            }
            else if constexpr (sizeof(T) == 4) {
                return static_cast<T>(to_big_32(u_val));
            }
            else if constexpr (sizeof(T) == 8) {
                return static_cast<T>(to_big_64(u_val));
            }
            else {
                static_assert(sizeof(T) <= 8, "1, 2, 4, 8바이트 정수 타입만 지원합니다.");
            }
        }

        template <typename T>
        static std::enable_if_t<std::is_integral_v<T>, T> from_big(T value) noexcept {
            return to_big(value);
        }
    };

} // namespace mino::core::endian
