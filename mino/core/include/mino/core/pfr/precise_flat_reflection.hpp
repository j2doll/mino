#pragma once

#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <string>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <string_view>
#include <optional>
#include <variant>
#include <any>

#include <cstddef> 

namespace mino::core::pfr {

    // ============================================================================
    // [1] Boost.Pfr 코어 기능 구현 (멤버 개수 추출 및 튜플 변환)
    // ============================================================================

    struct universal_type {
        template <typename T>
        constexpr operator T() const noexcept;
    };

    template <typename T, typename... Args>
    constexpr auto detect_fields_count(long) {
        return sizeof...(Args) - 1;
    }

    template <typename T, typename... Args>
    constexpr auto detect_fields_count(int)
        -> decltype(T{ std::declval<Args>()... }, size_t{}) {
        return detect_fields_count<T, Args..., universal_type>(0);
    }

    template <typename T>
    constexpr size_t fields_count() {
        return detect_fields_count<std::decay_t<T>, universal_type>(0);
    }

    template <typename T>
    constexpr auto structure_to_tuple(T&& val) {
        constexpr size_t count = fields_count<std::decay_t<T>>();

        if constexpr (count == 0) {
            return std::make_tuple();
        }
        else if constexpr (count == 1) {
            auto&& [v1] = std::forward<T>(val);
            return std::forward_as_tuple(v1);
        }
        else if constexpr (count == 2) {
            auto&& [v1, v2] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2);
        }
        else if constexpr (count == 3) {
            auto&& [v1, v2, v3] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3);
        }
        else if constexpr (count == 4) {
            auto&& [v1, v2, v3, v4] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4);
        }
        else if constexpr (count == 5) { 
            auto&& [v1, v2, v3, v4, v5] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4, v5);
        }
        else if constexpr (count == 6) { 
            auto&& [v1, v2, v3, v4, v5, v6] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4, v5, v6);
        }
        else if constexpr (count == 7) { 
            auto&& [v1, v2, v3, v4, v5, v6, v7] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7);
        }
        else if constexpr (count == 8) { 
            auto&& [v1, v2, v3, v4, v5, v6, v7, v8] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8);
        }
        else if constexpr (count == 9) {
            auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9] = std::forward<T>(val);
            return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9);
        }
        else {
            static_assert(count <= 9, "The number of fields exceeded 9. Add a structure extension code if necessary.");
            return std::make_tuple();
        }
    }

    template <size_t Index, typename T>
    constexpr decltype(auto) get(T&& val) {
        return std::get<Index>(structure_to_tuple(std::forward<T>(val)));
    }

    template <typename T, typename Func>
    constexpr void structure_for_each(T&& val, Func&& func) {
        std::apply([&func](auto&&... args) {
            (func(std::forward<decltype(args)>(args)), ...);
            }, structure_to_tuple(std::forward<T>(val)));
    }

    // ============================================================================
    // [2] 컴파일 타임 타입 이름 자동 추출기 및 컨테이너 전체 자동 등록
    // ============================================================================

    template <typename T, typename = void>
    struct type_namer {
        static std::string name() { return "unknown_type"; }
    };

    // 기본 타입 등록용 매크로
#define REGISTER_BASIC_TYPE(Type, Name) \
template <typename T> \
struct type_namer<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Type>>> { \
    static std::string name() { return Name; } \
};

        REGISTER_BASIC_TYPE(int, "int")
        REGISTER_BASIC_TYPE(float, "float")
        REGISTER_BASIC_TYPE(double, "double")
        REGISTER_BASIC_TYPE(bool, "bool")
        REGISTER_BASIC_TYPE(char, "char")
        REGISTER_BASIC_TYPE(std::string, "string")
        REGISTER_BASIC_TYPE(std::wstring, "wstring")
        REGISTER_BASIC_TYPE(std::string_view, "string_view")
        REGISTER_BASIC_TYPE(std::any, "any")
        REGISTER_BASIC_TYPE(std::byte, "byte")

#undef REGISTER_BASIC_TYPE

        // 1. std::array 특수화 (크기 N 포함)
        template <typename T, size_t N>
    struct type_namer<std::array<T, N>> {
        static std::string name() {
            return "array<" + type_namer<T>::name() + ", " + std::to_string(N) + ">";
        }
    };

    // 단일 인자 템플릿 컨테이너 등록용 매크로 (optional 포함)
#define REGISTER_SINGLE_ARG_CONTAINER(ContainerClass, StringName) \
template <typename T, typename... Args> \
struct type_namer<ContainerClass<T, Args...>> { \
    static std::string name() { \
        return std::string(StringName) + "<" + type_namer<T>::name() + ">"; \
    } \
};

    REGISTER_SINGLE_ARG_CONTAINER(std::vector, "vector")
    REGISTER_SINGLE_ARG_CONTAINER(std::deque, "deque")
    REGISTER_SINGLE_ARG_CONTAINER(std::list, "list")
    REGISTER_SINGLE_ARG_CONTAINER(std::forward_list, "forward_list")
    REGISTER_SINGLE_ARG_CONTAINER(std::set, "set")
    REGISTER_SINGLE_ARG_CONTAINER(std::multiset, "multiset")
    REGISTER_SINGLE_ARG_CONTAINER(std::unordered_set, "unordered_set")
    REGISTER_SINGLE_ARG_CONTAINER(std::unordered_multiset, "unordered_multiset")
    REGISTER_SINGLE_ARG_CONTAINER(std::stack, "stack")
    REGISTER_SINGLE_ARG_CONTAINER(std::queue, "queue")
    REGISTER_SINGLE_ARG_CONTAINER(std::priority_queue, "priority_queue")

#undef REGISTER_SINGLE_ARG_CONTAINER // 매크로 해제

        // --- std::optional 전용 독립 특수화 코드로 분리 ---
        template <typename T>
        struct type_namer<std::optional<T>> {
            static std::string name() {
                return "optional<" + type_namer<T>::name() + ">";
            }
        };

        // Key-Value 기반 맵 계열 등록용 매크로
#define REGISTER_MAP_CONTAINER(MapClass, StringName) \
template <typename Key, typename Value, typename... Args> \
struct type_namer<MapClass<Key, Value, Args...>> { \
    static std::string name() { \
        return std::string(StringName) + "<" + type_namer<Key>::name() + ", " + type_namer<Value>::name() + ">"; \
    } \
};

        REGISTER_MAP_CONTAINER(std::map, "map")
        REGISTER_MAP_CONTAINER(std::multimap, "multimap")
        REGISTER_MAP_CONTAINER(std::unordered_map, "unordered_map")
        REGISTER_MAP_CONTAINER(std::unordered_multimap, "unordered_multimap")

#undef REGISTER_MAP_CONTAINER

        // 가변 인자 템플릿 std::variant 전용 특수화 처리
        template <typename... Types>
    struct type_namer<std::variant<Types...>> {
    private:
        static std::string join_types() {
            std::string result = "";
            size_t count = 0;
            ((result += (count++ > 0 ? ", " : "") + type_namer<Types>::name()), ...);
            return result;
        }
    public:
        static std::string name() {
            return "variant<" + join_types() + ">";
        }
    };

    // 외부 노출형 인터페이스 함수
    template <typename T>
    std::string get_type_name() {
        return type_namer<std::decay_t<T>>::name();
    }

    // --- 이름 기반 순회를 위한 보조 유틸리티 추가 -----------------------------

    // T::mino_field_names() 가 존재하는지 검사
    template <typename T, typename = void>
    struct has_mino_field_names : std::false_type {};

    template <typename T>
    struct has_mino_field_names<T, std::void_t<decltype(T::mino_field_names())>> : std::true_type {};

    // 원시 이름 배열을 std::array<std::string, N>으로 변환
    template <typename Raw, size_t... I>
    static auto make_names_from_raw_impl(const Raw &raw, std::index_sequence<I...>) -> std::array<std::string, sizeof...(I)> {
        return { std::string(raw[I])... };
    }

    template <typename Raw, size_t N>
    static auto make_names_from_raw(const Raw &raw) -> std::array<std::string, N> {
        return make_names_from_raw_impl(raw, std::make_index_sequence<N>{});
    }

    // 인덱스 기반의 기본 이름 생성: "field0", "field1", ...
    template <size_t... I>
    static auto make_indexed_names_impl(std::index_sequence<I...>) -> std::array<std::string, sizeof...(I)> {
        return { (std::string("field") + std::to_string(I))... };
    }

    template <size_t N>
    static auto make_indexed_names() -> std::array<std::string, N> {
        return make_indexed_names_impl(std::make_index_sequence<N>{});
    }

    // 내부 구현: 이름 배열과 인덱스 시퀀스로 각 멤버에 접근하여 콜백 호출
    template <size_t... I, typename T, typename Func>
    static void structure_for_each_with_name_impl(T&& val, Func&& func, const std::array<std::string, sizeof...(I)> &names, std::index_sequence<I...>) {
        (func(names[I].c_str(), get<I>(val)), ...);
    }

    // 공개 인터페이스: mino_field_names가 있으면 그 이름을 사용하고, 없으면 "fieldN" 형태의 이름을 생성하여 사용
    template <typename T, typename Func>
    void structure_for_each_with_name(T&& val, Func&& func) {
        constexpr size_t count = fields_count<std::decay_t<T>>();
        if constexpr (count == 0) {
            return;
        }

        if constexpr (has_mino_field_names<std::decay_t<T>>::value) {
            constexpr auto raw = std::decay_t<T>::mino_field_names();
            auto names = make_names_from_raw<decltype(raw), count>(raw);
            structure_for_each_with_name_impl(std::forward<T>(val), std::forward<Func>(func), names, std::make_index_sequence<count>{});
        } else {
            auto names = make_indexed_names<count>();
            structure_for_each_with_name_impl(std::forward<T>(val), std::forward<Func>(func), names, std::make_index_sequence<count>{});
        }
    }

} 