#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace mino::core::container {
    // span은 소유권이 없는(Non-owning) 뷰(View) 컨테이너로,
    // 연속 메모리 영역을 가리키며
    // 고정 크기(Static Extent) 또는 동적 크기(Dynamic Extent)를 지원합니다.

    inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

    namespace detail {

        // 고정 크기(Static Extent)일 때는 크기 멤버 변수를 두지 않아 메모리 오버헤드 0 달성 (EBO)
        template <std::size_t Extent>
        class span_storage {
        public:
            constexpr span_storage() noexcept = default;
            constexpr explicit span_storage(std::size_t size) noexcept {
                assert(size == Extent && "Size mismatch for fixed extent span");
            }
            static constexpr std::size_t size() noexcept { return Extent; }
        };

        // 동적 크기(Dynamic Extent)일 때만 크기 멤버 변수(size_t) 유지
        template <>
        class span_storage<dynamic_extent> {
        public:
            constexpr span_storage() noexcept : size_(0) {}
            constexpr explicit span_storage(std::size_t size) noexcept : size_(size) {}
            constexpr std::size_t size() const noexcept { return size_; }

        private:
            std::size_t size_{ 0 };
        };

    } // namespace detail

    template <typename T, std::size_t Extent = dynamic_extent>
    class span {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static constexpr std::size_t extent = Extent;

        // 1. 기본 생성자 (동적 크기이거나 고정 크기가 0일 때만 활성화)
        template <std::size_t E = Extent, typename = std::enable_if_t<E == 0 || E == dynamic_extent>>
        constexpr span() noexcept : data_(nullptr), storage_(0) {}

        // 2. 포인터 + 크기 생성자
        constexpr span(pointer ptr, size_type count) noexcept
            : data_(ptr), storage_(count) {
        }

        // 3. 포인터 범위 [first, last) 생성자
        constexpr span(pointer first, pointer last) noexcept
            : data_(first), storage_(static_cast<size_type>(last - first)) {
        }

        // 4. C 스타일 배열 생성자 (T(&arr)[N])
        template <std::size_t N, typename = std::enable_if_t<Extent == dynamic_extent || Extent == N>>
        constexpr span(element_type(&arr)[N]) noexcept
            : data_(arr), storage_(N) {
        }

        // 5. std::array 연동 생성자 (Non-const & Const)
        template <typename U, std::size_t N,
            typename = std::enable_if_t<(Extent == dynamic_extent || Extent == N) &&
            std::is_convertible_v<U(*)[], element_type(*)[]>>>
            constexpr span(std::array<U, N>& arr) noexcept
            : data_(arr.data()), storage_(N) {
        }

        template <typename U, std::size_t N,
            typename = std::enable_if_t<(Extent == dynamic_extent || Extent == N) &&
            std::is_convertible_v<const U(*)[], element_type(*)[]>>>
            constexpr span(const std::array<U, N>& arr) noexcept
            : data_(arr.data()), storage_(N) {
        }

        // 6. 연속 메모리 컨테이너 (std::vector, std::string 등) 연동 생성자 (C++17 SFINAE)
        template <typename Container,
            typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<Container>, span>&&
            std::is_convertible_v<std::remove_pointer_t<decltype(std::data(std::declval<Container&>()))>(*)[], element_type(*)[]>>>
            constexpr span(Container& cont)
            : data_(std::data(cont)), storage_(std::size(cont)) {
        }

        template <typename Container,
            typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<Container>, span>&&
            std::is_convertible_v<std::remove_pointer_t<decltype(std::data(std::declval<const Container&>()))>(*)[], element_type(*)[]>>>
            constexpr span(const Container& cont)
            : data_(std::data(cont)), storage_(std::size(cont)) {
        }

        constexpr span(const span&) noexcept = default;
        constexpr span& operator=(const span&) noexcept = default;

        // 반복자
        constexpr iterator begin() const noexcept { return data_; }
        constexpr iterator end() const noexcept { return data_ + size(); }
        constexpr const_iterator cbegin() const noexcept { return data_; }
        constexpr const_iterator cend() const noexcept { return data_ + size(); }
        constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
        constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

        // 원소 접근
        constexpr reference operator[](size_type idx) const noexcept {
            assert(idx < size() && "Index out of range");
            return data_[idx];
        }
        constexpr reference front() const noexcept {
            assert(!empty() && "front() called on empty span");
            return data_[0];
        }
        constexpr reference back() const noexcept {
            assert(!empty() && "back() called on empty span");
            return data_[size() - 1];
        }
        constexpr pointer data() const noexcept { return data_; }

        // 크기 조회
        constexpr size_type size() const noexcept { return storage_.size(); }
        constexpr size_type size_bytes() const noexcept { return size() * sizeof(element_type); }
        constexpr bool empty() const noexcept { return size() == 0; }

        // 부분 뷰 (Subviews)
        template <std::size_t Count>
        constexpr span<element_type, Count> first() const {
            assert(Count <= size() && "Count exceeds span size");
            return { data_, Count };
        }
        constexpr span<element_type, dynamic_extent> first(size_type count) const {
            assert(count <= size() && "Count exceeds span size");
            return { data_, count };
        }

        template <std::size_t Count>
        constexpr span<element_type, Count> last() const {
            assert(Count <= size() && "Count exceeds span size");
            return { data_ + (size() - Count), Count };
        }
        constexpr span<element_type, dynamic_extent> last(size_type count) const {
            assert(count <= size() && "Count exceeds span size");
            return { data_ + (size() - count), count };
        }

        template <std::size_t Offset, std::size_t Count = dynamic_extent>
        constexpr auto subspan() const {
            assert(Offset <= size() && "Offset exceeds span size");
            if constexpr (Count != dynamic_extent) {
                assert(Offset + Count <= size() && "Count exceeds subspan bounds");
                return span<element_type, Count>(data_ + Offset, Count);
            }
            else {
                return span<element_type, dynamic_extent>(data_ + Offset, size() - Offset);
            }
        }

        constexpr span<element_type, dynamic_extent> subspan(size_type offset, size_type count = dynamic_extent) const {
            assert(offset <= size() && "Offset exceeds span size");
            size_type actual_count = (count == dynamic_extent) ? (size() - offset) : count;
            assert(offset + actual_count <= size() && "Subspan bounds error");
            return { data_ + offset, actual_count };
        }

    private:
        pointer data_{ nullptr };
        detail::span_storage<Extent> storage_;
    };

    // 헬퍼 바이트 뷰 변환 함수
    template <typename T, std::size_t Extent>
    auto as_bytes(span<T, Extent> s) noexcept {
        constexpr std::size_t byte_extent = (Extent == dynamic_extent) ? dynamic_extent : Extent * sizeof(T);
        return span<const std::byte, byte_extent>(reinterpret_cast<const std::byte*>(s.data()), s.size_bytes());
    }

    template <typename T, std::size_t Extent, typename = std::enable_if_t<!std::is_const_v<T>>>
    auto as_writable_bytes(span<T, Extent> s) noexcept {
        constexpr std::size_t byte_extent = (Extent == dynamic_extent) ? dynamic_extent : Extent * sizeof(T);
        return span<std::byte, byte_extent>(reinterpret_cast<std::byte*>(s.data()), s.size_bytes());
    }

    // C++17 템플릿 인자 추론 가이드 (CTAD)
    template <typename T, std::size_t N>
    span(T(&)[N]) -> span<T, N>;

    template <typename T, std::size_t N>
    span(std::array<T, N>&) -> span<T, N>;

    template <typename T, std::size_t N>
    span(const std::array<T, N>&) -> span<const T, N>;

    template <typename Container>
    span(Container&) -> span<typename Container::value_type>;

    template <typename Container>
    span(const Container&) -> span<const typename Container::value_type>;

} // namespace mino::core::container
