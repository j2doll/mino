#pragma once

#include <type_traits>
#include <utility>
#include <cstddef>

namespace mino::core::traits {

    // --- 1. Detection Idiom (C++ Library Fundamentals TS 표준 패턴) ---
    namespace detail {
        template <typename Default, typename AlwaysVoid, template <typename...> class Op, typename... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };

        template <typename Default, template <typename...> class Op, typename... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };

        struct nonesuch {
            nonesuch() = delete;
            ~nonesuch() = delete;
            nonesuch(const nonesuch&) = delete;
            void operator=(const nonesuch&) = delete;
        };
    } // namespace detail

    template <template <typename...> class Op, typename... Args>
    using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

    template <template <typename...> class Op, typename... Args>
    inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

    // --- 2. C++17 가독성 향상 매크로 (C++20 requires 모사) ---
    // 템플릿 매개변수 목록 맨 뒤에 제약 조건을 거는 매크로
#define MINO_REQUIRES(...) , std::enable_if_t<(__VA_ARGS__), int> = 0

// --- 3. 기본 사전 정의 컨셉 (Type Concepts) ---

// 정수형 타입 제약
    template <typename T>
    inline constexpr bool Integral = std::is_integral_v<std::decay_t<T>>;

    // 부동소수점 타입 제약
    template <typename T>
    inline constexpr bool FloatingPoint = std::is_floating_point_v<std::decay_t<T>>;

    // Trivially Copyable (직렬화 가능한 POD 성격의 타입)
    template <typename T>
    inline constexpr bool TriviallyCopyable = std::is_trivially_copyable_v<std::decay_t<T>>;

    // --- 4. 표현식 검사 컨셉 (Expression Concepts via void_t) ---

    // .size() 멤버 함수가 존재하는가?
    template <typename T>
    using has_size_expr = decltype(std::declval<const T&>().size());

    template <typename T>
    inline constexpr bool HasSize = is_detected_v<has_size_expr, T>;

    // .data() 멤버 함수가 존재하는가?
    template <typename T>
    using has_data_expr = decltype(std::declval<T&>().data());

    template <typename T>
    inline constexpr bool HasData = is_detected_v<has_data_expr, T>;

    // 연속 메모리 버퍼(Contiguous Buffer) 컨셉 (size와 data를 모두 가짐)
    template <typename T>
    inline constexpr bool ContiguousContainer = HasSize<T> && HasData<T>;

} // namespace mino::core::traits
