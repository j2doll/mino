#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <utility>

#define MINO_REFLECT(...) \
    auto reflect() { return std::tie(__VA_ARGS__); } \
    auto reflect() const { return std::tie(__VA_ARGS__); }

// MINO_REFLECT 적용 대상인 구조체의 멤버 타입
// - `int`, `uint8_t`, `uint32_t`, `uint64_t`, `float`, `double`, `bool`, `char` 등의 기본 타입(POD)
// - `enum` 및 `enum class`
// - `MINO_REFLECT` 매크로가 정의된 구조체 및 클래스
// - `std::string`
// - `std::vector<T>`
// - `std::deque<T>`
// - `std::list<T>`
// - Trivially Copyable한 C 스타일 구조체 및 기본형 고정 크기 배열

namespace mino::core::reflect {

    namespace detail {

        // C++17 std::void_t 기반 타입 특성(Type Traits) 검출기
        template <typename, typename = void>
        struct has_reflect : std::false_type {};

        template <typename T>
        struct has_reflect<T, std::void_t<decltype(std::declval<T>().reflect())>> : std::true_type {};

        template <typename T>
        inline constexpr bool has_reflect_v = has_reflect<T>::value;

        template <typename, typename = void>
        struct is_container : std::false_type {};

        template <typename T>
        struct is_container<T, std::void_t<
            decltype(std::declval<T>().size()),
            decltype(std::declval<T>().begin()),
            decltype(std::declval<T>().end())
            >> : std::true_type {};

        template <typename T>
        inline constexpr bool is_container_v = is_container<T>::value;

        template <typename, typename = void>
        struct is_resizable : std::false_type {};

        template <typename T>
        struct is_resizable < T, std::void_t<decltype(std::declval<T>().resize(size_t{})) >> : std::true_type {};

        template <typename T>
        inline constexpr bool is_resizable_v = is_resizable<T>::value;

    } // namespace detail

    class binary_writer {
    public:
        binary_writer();
        ~binary_writer();

        void write_bytes(const void* src, size_t size);
        void write_string(std::string_view str);

        [[nodiscard]] const std::vector<uint8_t>& get_buffer() const noexcept;
        [[nodiscard]] std::vector<uint8_t>& get_buffer() noexcept;
        [[nodiscard]] size_t size() const noexcept;
        void clear() noexcept;

        template <typename T>
        void operator()(const T& value);

    private:
        std::vector<uint8_t> buffer_;
    };

    class binary_reader {
    public:
        binary_reader(const uint8_t* data, size_t size) noexcept;
        ~binary_reader();

        bool read_bytes(void* dest, size_t size);
        bool read_string(std::string& str);

        [[nodiscard]] bool has_error() const noexcept;
        [[nodiscard]] size_t bytes_read() const noexcept;

        template <typename T>
        void operator()(T& value);

    private:
        const uint8_t* data_;
        size_t size_;
        size_t offset_;
        bool error_;
    };

    // --- C++17 템플릿 메서드 구현부 ---

    template <typename T>
    void binary_writer::operator()(const T& value) {
        if constexpr (detail::has_reflect_v<T>) {
            std::apply([this](const auto&... args) {
                ((*this)(args), ...); // Fold Expression
                }, value.reflect());
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            write_string(value);
        }
        else if constexpr (detail::is_container_v<T> && !std::is_trivially_copyable_v<T>) {
            const auto count = static_cast<uint32_t>(value.size());
            (*this)(count);
            for (const auto& elem : value) {
                (*this)(elem);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            write_bytes(&value, sizeof(T));
        }
    }

    template <typename T>
    void binary_reader::operator()(T& value) {
        if (error_) return;

        if constexpr (detail::has_reflect_v<T>) {
            std::apply([this](auto&... args) {
                ((*this)(args), ...);
                }, value.reflect());
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            read_string(value);
        }
        else if constexpr (detail::is_resizable_v<T> && detail::is_container_v<T>) {
            uint32_t count = 0;
            (*this)(count);
            if (error_) return;
            value.resize(count);
            for (auto& elem : value) {
                (*this)(elem);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            read_bytes(&value, sizeof(T));
        }
    }

} // namespace mino::core::reflect
