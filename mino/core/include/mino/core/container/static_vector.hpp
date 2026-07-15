#pragma once

#include <cstddef>
#include <stdexcept>
#include <new>
#include <utility>
#include <iterator>
#include <type_traits>

namespace mino::core::container {

    template <typename T, std::size_t Capacity>
    class  static_vector {
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

        // 생성자 및 소멸자
        static_vector() noexcept : size_(0) {}

        static_vector(const static_vector& other) {
            assign_impl(other);
        }

        static_vector(static_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
            move_impl(std::move(other));
        }

        ~static_vector() {
            clear();
        }

        // 대입 연산자
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

        // 엘리먼트 접근 (Element access)
        reference at(size_type pos) {
            if (pos >= size_) {
                throw std::out_of_range("static_vector::at - index out of range");
            }
            return data_ptr()[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size_) {
                throw std::out_of_range("static_vector::at - index out of range");
            }
            return data_ptr()[pos];
        }

        reference operator[](size_type pos) noexcept { return data_ptr()[pos]; }
        const_reference operator[](size_type pos) const noexcept { return data_ptr()[pos]; }

        reference front() noexcept { return data_ptr()[0]; }
        const_reference front() const noexcept { return data_ptr()[0]; }

        reference back() noexcept { return data_ptr()[size_ - 1]; }
        const_reference back() const noexcept { return data_ptr()[size_ - 1]; }

        pointer data() noexcept { return data_ptr(); }
        const_pointer data() const noexcept { return data_ptr(); }

        // 반복자 (Iterators)
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

        // 용량 (Capacity)
        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }
        size_type max_size() const noexcept { return Capacity; }
        size_type capacity() const noexcept { return Capacity; }

        // 수정자 (Modifiers)
        void clear() noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_type i = 0; i < size_; ++i) {
                    data_ptr()[i].~T();
                }
            }
            size_ = 0;
        }

        void push_back(const T& value) {
            if (size_ >= Capacity) throw std::bad_alloc();
            new (static_cast<void*>(data_ptr() + size_)) T(value);
            ++size_;
        }

        void push_back(T&& value) {
            if (size_ >= Capacity) throw std::bad_alloc();
            new (static_cast<void*>(data_ptr() + size_)) T(std::move(value));
            ++size_;
        }

        template <typename... Args>
        reference emplace_back(Args&&... args) {
            if (size_ >= Capacity) throw std::bad_alloc();
            pointer p = new (static_cast<void*>(data_ptr() + size_)) T(std::forward<Args>(args)...);
            ++size_;
            return *p;
        }

        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    data_ptr()[size_].~T();
                }
            }
        }

        void swap(static_vector& other) noexcept(std::is_nothrow_swappable_v<T>) {
            if (this == &other) return;

            static_vector* min_size_vec = (this->size_ < other.size_) ? this : &other;
            static_vector* max_size_vec = (this->size_ < other.size_) ? &other : this;

            size_type min_sz = min_size_vec->size_;
            size_type max_sz = max_size_vec->size_;

            // 공통 원소들 swap
            for (size_type i = 0; i < min_sz; ++i) {
                using std::swap;
                swap(this->data_ptr()[i], other.data_ptr()[i]);
            }

            // 남은 원소들 이동 분해
            for (size_type i = min_sz; i < max_sz; ++i) {
                new (static_cast<void*>(min_size_vec->data_ptr() + i)) T(std::move(max_size_vec->data_ptr()[i]));
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    max_size_vec->data_ptr()[i].~T();
                }
            }

            std::swap(this->size_, other.size_);
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

        // 정렬(Alignment)을 고려한 무형성 스토리지 버퍼
        alignas(T) std::byte storage_[sizeof(T) * Capacity];
        size_type size_ = 0;
    };

    // 비멤버 함수 swap
    template <typename T, std::size_t Capacity>
    void swap(static_vector<T, Capacity>& lhs, static_vector<T, Capacity>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

} // namespace mino::core::container
