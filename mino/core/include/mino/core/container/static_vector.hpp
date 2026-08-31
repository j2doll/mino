#pragma once

#include <cstddef>
#include <stdexcept>
#include <new>
#include <utility>
#include <iterator>
#include <type_traits>
#include <initializer_list>
#include <algorithm>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::array        |  static_vector
// ------------+--------------------+--------------------+------------------
// 메모리 할당 | 무조건 힙 동적할당 | 스택 고정 할당     | 100% 스택 고정 (동적 할당 0%)
// 크기(Size)  | 가변 크기          | 고정 크기 (N)      | 가변 크기 (최대 Capacity)
// 힙 오버헤드 | 항상 발생          | 전혀 없음          | 전혀 없음 (Zero Allocation)
// ------------+--------------------+--------------------+------------------
// 캐시 효율성 | 보통 (포인터 역참조)| 최상 (스택 연속)   | 최상 (스택 연속)
// 예외 안전성 | 메모리 부족 예외   | 크기 초과 없음     | 가득 참 검사 (Non-throwing)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 범용 동적 컨테이너 | 정적 상수 배열     | 임베디드, 실시간 게임 루프,
//             |                    |                    | 무할당(Zero-Alloc) 고성능 큐
// ============+====================+====================+==================
//
// static_vector는 힙(Heap) 동적 할당을 전혀 사용하지 않고, 컴파일 타임에 지정된
// 최대 용량(Capacity) 내에서 동적 가변 크기(Dynamic Size)를 지원하는 고성능 컨테이너입니다.
//
// static_vector<int, 5> vec; // 최대 용량 5인 스택 벡터 생성
// 
// // [1] 데이터 삽입 (Non-throwing: 용량 초과 시 false 반환)
// vec.push_back(10);
// vec.push_back(20);
// vec.push_back(30);
// 
// // [2] 상태 확인
// std::cout << "Size: " << vec.size() << "/" << vec.capacity() << std::endl; // 3/5
// std::cout << "Front: " << vec.front() << ", Back: " << vec.back() << std::endl; // 10, 30
// 
// // [3] 인덱스 및 안전 포인터 접근 (at)
// std::cout << "vec[1]: " << vec[1] << std::endl; // 20
// if (int* p = vec.at(2)) {
//     std::cout << "at(2): " << *p << std::endl;  // 30
// }
// 
// // [4] 용량 초과 삽입 방어
// vec.push_back(40);
// vec.push_back(50);
// bool ok = vec.push_back(60); // 5개 꽉 참 -> false 반환
// std::cout << "Push 60 Success: " << std::boolalpha << ok << std::endl; // false
// 
// // [5] 범위 기반 for문 순회
// for (const auto& val : vec) {
//     std::cout << val << " "; // 10 20 30 40 50
// }
// std::cout << std::endl;
// 
// // [6] 초기화
// vec.clear();
// std::cout << "Is empty: " << vec.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, std::size_t Capacity>
    class static_vector {
    public:
        // 타입 정의 (Type definitions)
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // --------------------------------------------------------------------
        // 생성자 및 소멸자
        // --------------------------------------------------------------------
        static_vector() noexcept : size_(0) {}

        explicit static_vector(size_type count) {
            size_type n = std::min(count, Capacity);
            for (size_type i = 0; i < n; ++i) {
                new (static_cast<void*>(data_ptr() + i)) T();
            }
            size_ = n;
        }

        static_vector(size_type count, const T& value) {
            size_type n = std::min(count, Capacity);
            for (size_type i = 0; i < n; ++i) {
                new (static_cast<void*>(data_ptr() + i)) T(value);
            }
            size_ = n;
        }

        static_vector(std::initializer_list<T> init) {
            size_type n = std::min(init.size(), Capacity);
            auto it = init.begin();
            for (size_type i = 0; i < n; ++i, ++it) {
                new (static_cast<void*>(data_ptr() + i)) T(*it);
            }
            size_ = n;
        }

        template <typename InputIt>
        static_vector(InputIt first, InputIt last) {
            while (first != last && size_ < Capacity) {
                new (static_cast<void*>(data_ptr() + size_)) T(*first);
                ++size_;
                ++first;
            }
        }

        static_vector(const static_vector& other) {
            assign_impl(other);
        }

        static_vector(static_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
            move_impl(std::move(other));
        }

        ~static_vector() {
            clear();
        }

        static_vector& operator=(const static_vector& other) {
            if (this != &other) {
                clear();
                assign_impl(other);
            }
            return *this;
        }

        static_vector& operator=(static_vector&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if (this != &other) {
                clear();
                move_impl(std::move(other));
            }
            return *this;
        }

        // --------------------------------------------------------------------
        // 원소 접근 (Element access)
        // --------------------------------------------------------------------
        pointer at(size_type pos) noexcept {
            if (pos >= size_) return nullptr;
            return data_ptr() + pos;
        }

        const_pointer at(size_type pos) const noexcept {
            if (pos >= size_) return nullptr;
            return data_ptr() + pos;
        }

        reference operator[](size_type pos) noexcept { return data_ptr()[pos]; }
        const_reference operator[](size_type pos) const noexcept { return data_ptr()[pos]; }

        reference front() noexcept { return data_ptr()[0]; }
        const_reference front() const noexcept { return data_ptr()[0]; }

        reference back() noexcept { return data_ptr()[size_ - 1]; }
        const_reference back() const noexcept { return data_ptr()[size_ - 1]; }

        pointer data() noexcept { return data_ptr(); }
        const_pointer data() const noexcept { return data_ptr(); }

        // --------------------------------------------------------------------
        // 반복자 (Iterators)
        // --------------------------------------------------------------------
        iterator begin() noexcept { return data_ptr(); }
        const_iterator begin() const noexcept { return data_ptr(); }
        const_iterator cbegin() const noexcept { return data_ptr(); }

        iterator end() noexcept { return data_ptr() + size_; }
        const_iterator end() const noexcept { return data_ptr() + size_; }
        const_iterator cend() const noexcept { return data_ptr() + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        // --------------------------------------------------------------------
        // 용량 (Capacity)
        // --------------------------------------------------------------------
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type max_size() const noexcept { return Capacity; }
        [[nodiscard]] size_type capacity() const noexcept { return Capacity; }

        // --------------------------------------------------------------------
        // 수정자 (Modifiers)
        // --------------------------------------------------------------------
        void clear() noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_type i = 0; i < size_; ++i) {
                    data_ptr()[i].~T();
                }
            }
            size_ = 0;
        }

        [[nodiscard]] bool push_back(const T& value) noexcept {
            if (size_ >= Capacity) return false;
            new (static_cast<void*>(data_ptr() + size_)) T(value);
            ++size_;
            return true;
        }

        [[nodiscard]] bool push_back(T&& value) noexcept {
            if (size_ >= Capacity) return false;
            new (static_cast<void*>(data_ptr() + size_)) T(std::move(value));
            ++size_;
            return true;
        }

        pointer emplace_back() noexcept {
            if (size_ >= Capacity) return nullptr;
            pointer p = new (static_cast<void*>(data_ptr() + size_)) T();
            ++size_;
            return p;
        }

        template <typename... Args>
        pointer emplace_back(Args&&... args) noexcept {
            if (size_ >= Capacity) return nullptr;
            pointer p = new (static_cast<void*>(data_ptr() + size_)) T(std::forward<Args>(args)...);
            ++size_;
            return p;
        }

        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    data_ptr()[size_].~T();
                }
            }
        }

        void resize(size_type count) {
            size_type target = std::min(count, Capacity);
            if (target < size_) {
                while (size_ > target) pop_back();
            }
            else {
                while (size_ < target) emplace_back();
            }
        }

        void resize(size_type count, const T& value) {
            size_type target = std::min(count, Capacity);
            if (target < size_) {
                while (size_ > target) pop_back();
            }
            else {
                while (size_ < target) push_back(value);
            }
        }

        void swap(static_vector& other) noexcept(std::is_nothrow_swappable_v<T>) {
            if (this == &other) return;

            static_vector* min_vec = (this->size_ < other.size_) ? this : &other;
            static_vector* max_vec = (this->size_ < other.size_) ? &other : this;

            size_type min_sz = min_vec->size_;
            size_type max_sz = max_vec->size_;

            for (size_type i = 0; i < min_sz; ++i) {
                using std::swap;
                swap(this->data_ptr()[i], other.data_ptr()[i]);
            }

            for (size_type i = min_sz; i < max_sz; ++i) {
                new (static_cast<void*>(min_vec->data_ptr() + i)) T(std::move(max_vec->data_ptr()[i]));
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    max_vec->data_ptr()[i].~T();
                }
            }

            std::swap(this->size_, other.size_);
        }

        // ==========================================
        // 순수 ASCII 기반 Static Vector Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << "/" << Capacity << ") ===\n";
            }
            else {
                std::cout << "=== Static Vector Dump (Size: " << size_ << "/" << Capacity << ") ===\n";
            }

            if (Capacity == 0) {
                std::cout << "  \\-- <Zero Capacity Vector>\n\n";
                return;
            }

            for (size_type i = 0; i < Capacity; ++i) {
                bool is_last = (i == Capacity - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << i << "] : ";

                if (i < size_) {
                    std::cout << data_ptr()[i];
                }
                else {
                    std::cout << "<Uninitialized>";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }

    private:
        pointer data_ptr() noexcept {
            return reinterpret_cast<pointer>(&storage_);
        }

        const_pointer data_ptr() const noexcept {
            return reinterpret_cast<const_pointer>(&storage_);
        }

        void assign_impl(const static_vector& other) {
            for (size_type i = 0; i < other.size_; ++i) {
                new (static_cast<void*>(data_ptr() + i)) T(other.data_ptr()[i]);
            }
            size_ = other.size_;
        }

        void move_impl(static_vector&& other) {
            for (size_type i = 0; i < other.size_; ++i) {
                new (static_cast<void*>(data_ptr() + i)) T(std::move(other.data_ptr()[i]));
            }
            size_ = other.size_;
            other.clear();
        }

        static constexpr size_type buffer_bytes_ = (Capacity > 0 ? sizeof(T) * Capacity : sizeof(void*));
        alignas(alignof(T)) std::byte storage_[buffer_bytes_];
        size_type size_ = 0;
    };

    template <typename T, std::size_t Capacity>
    void swap(static_vector<T, Capacity>& lhs, static_vector<T, Capacity>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

} // namespace mino::core::container
