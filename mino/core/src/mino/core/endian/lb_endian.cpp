#include "mino/core/endian/lb_endian.hpp"

namespace mino::core::endian {

    namespace {

        constexpr type detect_native() noexcept {
#if defined(_WIN32)
            return type::little;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
            if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
                return type::little;
            }
            else if constexpr (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
                return type::big;
            }
            else {
                static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__,
                    "지원하지 않는 혼합 엔디언입니다.");
            }
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || \
      defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || \
      defined(__i386__) || defined(__x86_64__)
            return type::little;
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
            return type::big;
#else
#error "지원하지 않거나 엔디언을 식별할 수 없는 플랫폼입니다."
#endif
        }

    } // namespace

    type get_native() noexcept {
        return detect_native();
    }

    bool is_little() noexcept {
        return detect_native() == type::little;
    }

    bool is_big() noexcept {
        return detect_native() == type::big;
    }

} // namespace mino::core::endian
