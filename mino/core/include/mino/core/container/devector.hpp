#pragma once

#include <iostream>
#include <memory>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <iterator>

namespace mino::core::container { 
       
    template <typename T, typename Allocator = std::allocator<T>>
    class  devector {
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using iterator = pointer;
        using const_iterator = const_pointer;

    private:
        allocator_type alloc_;
        pointer buffer_start_ = nullptr; // 할당된 전체 버퍼의 시작
        pointer buffer_end_ = nullptr;   // 할당된 전체 버퍼의 끝
        pointer front_ptr_ = nullptr;    // 실제 데이터의 시작 (첫 번째 원소)
        pointer back_ptr_ = nullptr;     // 실제 데이터의 끝 (마지막 원소 다음)

        void clear_and_deallocate() {
            if (buffer_start_) {
                clear();
                std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_start_, buffer_end_ - buffer_start_);
                buffer_start_ = buffer_end_ = front_ptr_ = back_ptr_ = nullptr;
            }
        }

        void reallocate_and_align(size_type new_capacity, size_type front_free_space) {
            // size_type current_size = size();
            pointer new_buffer = std::allocator_traits<allocator_type>::allocate(alloc_, new_capacity);
            pointer new_front = new_buffer + front_free_space;
            pointer new_back = new_front;

            try {
                for (auto it = front_ptr_; it != back_ptr_; ++it) {
                    std::allocator_traits<allocator_type>::construct(alloc_, new_back, std::move(*it));
                    new_back++;
                }
            }
            catch (...) {
                // 예외 발생 시 원복 및 해제
                for (pointer p = new_front; p != new_back; ++p) {
                    std::allocator_traits<allocator_type>::destroy(alloc_, p);
                }
                std::allocator_traits<allocator_type>::deallocate(alloc_, new_buffer, new_capacity);
                throw;
            }

            // 기존 데이터 파괴
            for (pointer p = front_ptr_; p != back_ptr_; ++p) {
                std::allocator_traits<allocator_type>::destroy(alloc_, p);
            }
            if (buffer_start_) {
                std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_start_, buffer_end_ - buffer_start_);
            }

            buffer_start_ = new_buffer;
            buffer_end_ = new_buffer + new_capacity;
            front_ptr_ = new_front;
            back_ptr_ = new_back;
        }



        void grow_if_needed_front() {
            if (front_ptr_ == buffer_start_) {
                size_type current_cap = capacity();
                size_type new_cap = current_cap == 0 ? 4 : current_cap * 2;
                // 앞쪽에 공간을 확보하기 위해 새 버퍼의 중간 이후 지점에 정렬
                size_type front_free = new_cap - size() - (new_cap / 4);
                reallocate_and_align(new_cap, front_free);
            }
        }

        void grow_if_needed_back() {
            if (back_ptr_ == buffer_end_) {
                size_type current_cap = capacity();
                size_type new_cap = current_cap == 0 ? 4 : current_cap * 2;
                // 뒤쪽에 공간을 확보하기 위해 새 버퍼의 앞쪽에 가깝게 정렬
                size_type front_free = new_cap / 4;
                reallocate_and_align(new_cap, front_free);
            }
        }

    public:
        // --- 생성자 & 소멸자 ---
        devector() = default;

        explicit devector(size_type count, const allocator_type& alloc = allocator_type())
            : alloc_(alloc) {
            if (count > 0) {
                reallocate_and_align(count * 2, count / 2);
                for (size_type i = 0; i < count; ++i) {
                    std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, T());
                    back_ptr_++;
                }
            }
        }

        devector(std::initializer_list<T> init, const allocator_type& alloc = allocator_type())
            : alloc_(alloc) {
            if (init.size() > 0) {
                reallocate_and_align(init.size() * 2, init.size() / 2);
                for (const auto& item : init) {
                    std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, item);
                    back_ptr_++;
                }
            }
        }

        ~devector() {
            clear_and_deallocate();
        }

        // 복사 / 이동 생성자 및 대입 연산자
        devector(const devector& other) : alloc_(other.alloc_) {
            if (!other.empty()) {
                reallocate_and_align(other.capacity(), other.front_ptr_ - other.buffer_start_);
                for (auto it = other.front_ptr_; it != other.back_ptr_; ++it) {
                    std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, *it);
                    back_ptr_++;
                }
            }
        }

        devector(devector&& other) noexcept
            : alloc_(std::move(other.alloc_)), buffer_start_(other.buffer_start_),
            buffer_end_(other.buffer_end_), front_ptr_(other.front_ptr_), back_ptr_(other.back_ptr_) {
            other.buffer_start_ = other.buffer_end_ = other.front_ptr_ = other.back_ptr_ = nullptr;
        }

        devector& operator=(const devector& other) {
            if (this != &other) {
                clear_and_deallocate();
                alloc_ = other.alloc_;
                if (!other.empty()) {
                    reallocate_and_align(other.capacity(), other.front_ptr_ - other.buffer_start_);
                    for (auto it = other.front_ptr_; it != other.back_ptr_; ++it) {
                        std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, *it);
                        back_ptr_++;
                    }
                }
            }
            return *this;
        }

        devector& operator=(devector&& other) noexcept {
            if (this != &other) {
                clear_and_deallocate();
                alloc_ = std::move(other.alloc_);
                buffer_start_ = other.buffer_start_;
                buffer_end_ = other.buffer_end_;
                front_ptr_ = other.front_ptr_;
                back_ptr_ = other.back_ptr_;
                other.buffer_start_ = other.buffer_end_ = other.front_ptr_ = other.back_ptr_ = nullptr;
            }
            return *this;
        }

        // --- 요소 접근 (Element Access) ---
        reference operator[](size_type pos) { return front_ptr_[pos]; }
        const_reference operator[](size_type pos) const { return front_ptr_[pos]; }

        reference at(size_type pos) {
            if (pos >= size()) throw std::out_of_range("devector::at out of range");
            return front_ptr_[pos];
        }
        const_reference at(size_type pos) const {
            if (pos >= size()) throw std::out_of_range("devector::at out of range");
            return front_ptr_[pos];
        }

        reference front() { return *front_ptr_; }
        const_reference front() const { return *front_ptr_; }

        reference back() { return *(back_ptr_ - 1); }
        const_reference back() const { return *(back_ptr_ - 1); }

        T* data() noexcept { return front_ptr_; }
        const T* data() const noexcept { return front_ptr_; }

        // --- 반복자 (Iterators) ---
        iterator begin() noexcept { return front_ptr_; }
        const_iterator begin() const noexcept { return front_ptr_; }
        const_iterator cbegin() const noexcept { return front_ptr_; }

        iterator end() noexcept { return back_ptr_; }
        const_iterator end() const noexcept { return back_ptr_; }
        const_iterator cend() const noexcept { return back_ptr_; }

        // --- 용량 (Capacity) ---
        bool empty() const noexcept { return front_ptr_ == back_ptr_; }
        size_type size() const noexcept { return back_ptr_ - front_ptr_; }
        size_type capacity() const noexcept { return buffer_end_ - buffer_start_; }
        size_type free_front() const noexcept { return front_ptr_ - buffer_start_; }
        size_type free_back() const noexcept { return buffer_end_ - back_ptr_; }

        // --- 수정자 (Modifiers) ---
        void clear() noexcept {
            for (pointer p = front_ptr_; p != back_ptr_; ++p) {
                std::allocator_traits<allocator_type>::destroy(alloc_, p);
            }
            front_ptr_ = back_ptr_ = buffer_start_ + (capacity() / 2); // 중앙 정렬 복구
        }

        void push_back(const T& value) {
            grow_if_needed_back();
            std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, value);
            back_ptr_++;
        }

        void push_back(T&& value) {
            grow_if_needed_back();
            std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, std::move(value));
            back_ptr_++;
        }

        void push_front(const T& value) {
            grow_if_needed_front();
            front_ptr_--;
            std::allocator_traits<allocator_type>::construct(alloc_, front_ptr_, value);
        }

        void push_front(T&& value) {
            grow_if_needed_front();
            front_ptr_--;
            std::allocator_traits<allocator_type>::construct(alloc_, front_ptr_, std::move(value));
        }

        void pop_back() {
            if (!empty()) {
                back_ptr_--;
                std::allocator_traits<allocator_type>::destroy(alloc_, back_ptr_);
            }
        }

        void pop_front() {
            if (!empty()) {
                std::allocator_traits<allocator_type>::destroy(alloc_, front_ptr_);
                front_ptr_++;
            }
        }

        template <typename... Args>
        reference emplace_back(Args&&... args) {
            grow_if_needed_back();
            std::allocator_traits<allocator_type>::construct(alloc_, back_ptr_, std::forward<Args>(args)...);
            reference ref = *back_ptr_;
            back_ptr_++;
            return ref;
        }

        template <typename... Args>
        reference emplace_front(Args&&... args) {
            grow_if_needed_front();
            front_ptr_--;
            std::allocator_traits<allocator_type>::construct(alloc_, front_ptr_, std::forward<Args>(args)...);
            return *front_ptr_;
        }
    };

} // namespace mino::core::container 
