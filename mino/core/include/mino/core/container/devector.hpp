#pragma once

#include <iostream>
#include <memory>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <utility>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::deque        |  devector
// ------------+--------------------+--------------------+------------------
// 메모리 구조 | 단일 연속 블록     | 다중 청크(Chunk)   | 단일 연속 블록
// 양방향 확장 | 뒤쪽만 O(1)        | 앞/뒤 모두 O(1)    | 앞/뒤 모두 O(1) [상환]
// 캐시 효율성 | 최상 (연속 메모리) | 보통 (간접 참조)   | 최상 (연속 메모리)
// 포인터 연산 | 단순 포인터 연산   | 복잡한 2중 포인터  | 단순 포인터 연산
// ------------+--------------------+--------------------+------------------
// push_back   | O(1) [상환]        | O(1)               | O(1) [상환]
// push_front  | O(N) (전체 이동)   | O(1)               | O(1) [상환]
// pop_front   | O(N) (전체 이동)   | O(1)               | O(1)
// pop_back    | O(1)               | O(1)               | O(1)
// 임의 접근   | O(1)               | O(1)               | O(1)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 뒤쪽 삽입 위주 작업| 크기가 매우 큰 큐  | 양방향 큐 + 벡터 성능
//             |                    |                    | (캐시 친화적 고성능)
// ============+====================+====================+==================
//
// devector(Double-Ended Vector)는 단일 연속 메모리 블록 내에서
// 앞(Front)과 뒤(Back) 양방향 모두 O(1) 상환 삽입/삭제를 지원하는 컨테이너입니다.
//
// devector<int> dv;
// 
// // [1] 뒤쪽에 추가
// dv.push_back(10); // [10]
// dv.push_back(20); // [10, 20]
// dv.push_back(30); // [10, 20, 30]
// 
// // [2] 앞쪽에 추가
// dv.push_front(5); // [5, 10, 20, 30]
// dv.push_front(1); // [1, 5, 10, 20, 30]
// 
// // [3] 상태 확인
// std::cout << "Size: " << dv.size() << std::endl;         // 5
// std::cout << "Front: " << dv.front() << std::endl;       // 1
// std::cout << "Back: " << dv.back() << std::endl;         // 30
// std::cout << "Capacity: " << dv.capacity() << std::endl; // 8 (기하급수적 확장)
// 
// // [4] 인덱스 접근 (O(1))
// std::cout << "dv[0]: " << dv[0] << std::endl; // 1
// std::cout << "dv[2]: " << dv[2] << std::endl; // 10
// 
// // [5] 양 끝 데이터 제거
// dv.pop_back();  // 30 제거 -> [1, 5, 10, 20]
// dv.pop_front(); // 1 제거  -> [5, 10, 20]
// 
// // [6] 빈 공간 확인
// std::cout << "Free front: " << dv.free_front() << std::endl; // 2 (앞쪽 여유 슬롯)
// std::cout << "Free back: " << dv.free_back() << std::endl;   // 3 (뒤쪽 여유 슬롯)
// 
// // [7] 초기화
// dv.clear();
// std::cout << "Is empty: " << dv.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Allocator = std::allocator<T>>
    class devector {
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
        pointer buffer_start_ = nullptr;
        pointer buffer_end_ = nullptr;
        pointer front_ptr_ = nullptr;
        pointer back_ptr_ = nullptr;

        void clear_and_deallocate() {
            if (buffer_start_) {
                for (pointer p = front_ptr_; p != back_ptr_; ++p) {
                    std::allocator_traits<allocator_type>::destroy(alloc_, p);
                }
                std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_start_, buffer_end_ - buffer_start_);
                buffer_start_ = buffer_end_ = front_ptr_ = back_ptr_ = nullptr;
            }
        }

        void reallocate_and_align(size_type new_capacity, size_type front_free_space) {
            pointer new_buffer = std::allocator_traits<allocator_type>::allocate(alloc_, new_capacity);
            pointer new_front = new_buffer + front_free_space;
            pointer new_back = new_front;

            try {
                for (auto it = front_ptr_; it != back_ptr_; ++it) {
                    std::allocator_traits<allocator_type>::construct(alloc_, new_back, std::move_if_noexcept(*it));
                    new_back++;
                }
            }
            catch (...) {
                for (pointer p = new_front; p != new_back; ++p) {
                    std::allocator_traits<allocator_type>::destroy(alloc_, p);
                }
                std::allocator_traits<allocator_type>::deallocate(alloc_, new_buffer, new_capacity);
                throw;
            }

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
                size_type front_free = new_cap - size() - (new_cap / 4);
                reallocate_and_align(new_cap, front_free);
            }
        }

        void grow_if_needed_back() {
            if (back_ptr_ == buffer_end_) {
                size_type current_cap = capacity();
                size_type new_cap = current_cap == 0 ? 4 : current_cap * 2;
                size_type front_free = new_cap / 4;
                reallocate_and_align(new_cap, front_free);
            }
        }

    public:
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

        pointer at(size_type pos) noexcept {
            if (pos >= size()) return nullptr;
            return front_ptr_ + pos;
        }
        const_pointer at(size_type pos) const noexcept {
            if (pos >= size()) return nullptr;
            return front_ptr_ + pos;
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
        [[nodiscard]] bool empty() const noexcept { return front_ptr_ == back_ptr_; }
        [[nodiscard]] size_type size() const noexcept { return back_ptr_ - front_ptr_; }
        [[nodiscard]] size_type capacity() const noexcept { return buffer_end_ - buffer_start_; }
        [[nodiscard]] size_type free_front() const noexcept { return front_ptr_ - buffer_start_; }
        [[nodiscard]] size_type free_back() const noexcept { return buffer_end_ - back_ptr_; }

        // --- 수정자 (Modifiers) ---
        void clear() noexcept {
            for (pointer p = front_ptr_; p != back_ptr_; ++p) {
                std::allocator_traits<allocator_type>::destroy(alloc_, p);
            }
            front_ptr_ = back_ptr_ = buffer_start_ + (capacity() / 2);
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

        // --- 순수 ASCII Buffer Dump ---
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ", Cap: " << capacity() << ") ===\n";
            }
            else {
                std::cout << "=== Devector Dump (Size: " << size() << ", Cap: " << capacity() << ") ===\n";
            }

            if (capacity() == 0) {
                std::cout << "  \\-- <Unallocated Buffer>\n\n";
                return;
            }

            std::cout << "Logical Elements: ";
            if (empty()) {
                std::cout << "<Empty>\n";
            }
            else {
                for (size_type i = 0; i < size(); ++i) {
                    std::cout << "[" << (*this)[i] << "]" << (i + 1 == size() ? "" : " <-> ");
                }
                std::cout << "\n";
            }

            std::cout << "Memory Layout (Front Free: " << free_front() << ", Back Free: " << free_back() << "):\n";
            size_type cap = capacity();
            for (size_type i = 0; i < cap; ++i) {
                bool is_last = (i == cap - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                pointer curr = buffer_start_ + i;

                std::cout << connector << "[" << i << "] : ";
                if (curr >= front_ptr_ && curr < back_ptr_) {
                    std::cout << *curr;
                    if (curr == front_ptr_) std::cout << " (FRONT)";
                    if (curr == back_ptr_ - 1) std::cout << " (BACK)";
                }
                else if (curr < front_ptr_) {
                    std::cout << "<Free Front>";
                }
                else {
                    std::cout << "<Free Back>";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }
    };

} // namespace mino::core::container
