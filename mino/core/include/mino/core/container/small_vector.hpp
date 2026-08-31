#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <iterator>
#include <initializer_list>
#include <algorithm>
#include <utility>
#include <optional>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::array        |  small_vector
// ------------+--------------------+--------------------+------------------
// 메모리 할당 | 무조건 힙 동적할당 | 스택 고정 할당     | N개까지 스택(SBO), 초과 시 힙
// 크기 변경   | 동적 확장 가능     | 고정 크기          | 동적 확장 가능
// 힙 오버헤드 | 항상 발생          | 없음               | N 이하 시 0 (최적 성능)
// ------------+--------------------+--------------------+------------------
// 캐시 효율성 | 보통 (포인터 역참조)| 최상 (스택 연속)   | 최상 (N 이하 시 스택 연속)
// 이동 연산   | O(1) (포인터 스왑) | O(N) (원소 복사)   | 스택: O(N), 힙: O(1)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 크기가 크거나      | 크기가 작고 고정된 | 대부분 작은 크기이지만 가끔
//             | 가변적인 컨테이너  | 경우               | 커질 수 있는 단기 임시 버퍼
// ============+====================+====================+==================
//
// small_vector는 소형 버퍼 최적화(Small Buffer Optimization)를 적용하여
// N개 이하의 원소는 힙 할당 없이 스택 내부 버퍼에 저장하고, N개를 초과할 때만
// 힙 메모리로 자동 전환되는 고성능 연속 메모리 벡터 컨테이너입니다.
//
// small_vector<int, 4> sv; // 4개까지는 스택 버퍼 사용
// 
// // [1] 데이터 삽입 (N개 이하: 스택 메모리 사용, O(1))
// sv.push_back(10);
// sv.push_back(20);
// sv.push_back(30);
// std::cout << "Is on Stack: " << sv.is_on_stack() << std::endl; // 1 (true)
// 
// // [2] 용량 초과 삽입 (4개 초과: 힙 동적 할당으로 자동 전환)
// sv.push_back(40);
// sv.push_back(50); // 5번째 요소 삽입 시 힙 메모리로 이동
// std::cout << "Is on Stack: " << sv.is_on_stack() << std::endl; // 0 (false)
// 
// // [3] 원소 접근 및 순회
// std::cout << "Front: " << sv.front() << ", Back: " << sv.back() << std::endl; // 10, 50
// for (const auto& val : sv) {
//     std::cout << val << " "; // 10 20 30 40 50
// }
// std::cout << std::endl;
// 
// // [4] shrink_to_fit (크기가 N 이하로 줄어들면 다시 스택으로 복귀)
// sv.pop_back(); // 50 제거
// sv.pop_back(); // 40 제거 (size: 3 <= N)
// sv.shrink_to_fit();
// std::cout << "Is on Stack after shrink: " << sv.is_on_stack() << std::endl; // 1 (true)
// 
// // [5] 초기화
// sv.clear();
// std::cout << "Is empty: " << sv.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, std::size_t N, typename Allocator = std::allocator<T>>
    class small_vector {
    public:
        using value_type = T;
        using allocator_type = Allocator;
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

    private:
        using alloc_traits = std::allocator_traits<Allocator>;

        static constexpr size_type stack_capacity_ = N;
        static constexpr size_type buffer_size_bytes_ = (N > 0) ? (N * sizeof(T)) : sizeof(void*);

        alignas(alignof(T)) char stack_buffer_[buffer_size_bytes_];

        pointer data_ = reinterpret_cast<pointer>(stack_buffer_);
        size_type size_ = 0;
        size_type capacity_ = N;

        Allocator allocator_;

        void destroy_elements() noexcept {
            for (size_type i = 0; i < size_; ++i) {
                alloc_traits::destroy(allocator_, data_ + i);
            }
        }

        void free_storage() noexcept {
            if (!is_on_stack() && data_) {
                alloc_traits::deallocate(allocator_, data_, capacity_);
            }
        }

    public:
        [[nodiscard]] bool is_on_stack() const noexcept {
            return data_ == reinterpret_cast<const_pointer>(stack_buffer_);
        }

        small_vector() noexcept(noexcept(Allocator())) : allocator_(Allocator()) {}

        explicit small_vector(const Allocator& alloc) noexcept : allocator_(alloc) {}

        explicit small_vector(size_type count, const Allocator& alloc = Allocator()) : allocator_(alloc) {
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                alloc_traits::construct(allocator_, data_ + i);
            }
            size_ = count;
        }

        small_vector(size_type count, const T& value, const Allocator& alloc = Allocator()) : allocator_(alloc) {
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                alloc_traits::construct(allocator_, data_ + i, value);
            }
            size_ = count;
        }

        small_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : allocator_(alloc) {
            reserve(init.size());
            for (const auto& item : init) {
                alloc_traits::construct(allocator_, data_ + size_, item);
                ++size_;
            }
        }

        ~small_vector() {
            destroy_elements();
            free_storage();
        }

        small_vector(const small_vector& other)
            : allocator_(alloc_traits::select_on_container_copy_construction(other.allocator_)) {
            reserve(other.size_);
            for (size_type i = 0; i < other.size_; ++i) {
                alloc_traits::construct(allocator_, data_ + i, other.data_[i]);
            }
            size_ = other.size_;
        }

        small_vector& operator=(const small_vector& other) {
            if (this != &other) {
                destroy_elements();
                free_storage();

                data_ = reinterpret_cast<pointer>(stack_buffer_);
                capacity_ = N;
                size_ = 0;

                if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
                    allocator_ = other.allocator_;
                }

                reserve(other.size_);
                for (size_type i = 0; i < other.size_; ++i) {
                    alloc_traits::construct(allocator_, data_ + i, other.data_[i]);
                }
                size_ = other.size_;
            }
            return *this;
        }

        small_vector(small_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            : allocator_(std::move(other.allocator_)) {
            if (other.is_on_stack()) {
                for (size_type i = 0; i < other.size_; ++i) {
                    alloc_traits::construct(allocator_, data_ + i, std::move_if_noexcept(other.data_[i]));
                }
                size_ = other.size_;
                other.destroy_elements();
                other.size_ = 0;
            }
            else {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                other.data_ = reinterpret_cast<pointer>(other.stack_buffer_);
                other.capacity_ = N;
                other.size_ = 0;
            }
        }

        small_vector& operator=(small_vector&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if (this != &other) {
                destroy_elements();
                free_storage();

                if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
                    allocator_ = std::move(other.allocator_);
                }

                if (other.is_on_stack()) {
                    data_ = reinterpret_cast<pointer>(stack_buffer_);
                    capacity_ = N;
                    for (size_type i = 0; i < other.size_; ++i) {
                        alloc_traits::construct(allocator_, data_ + i, std::move_if_noexcept(other.data_[i]));
                    }
                    size_ = other.size_;
                    other.destroy_elements();
                    other.size_ = 0;
                }
                else {
                    data_ = other.data_;
                    size_ = other.size_;
                    capacity_ = other.capacity_;

                    other.data_ = reinterpret_cast<pointer>(other.stack_buffer_);
                    other.capacity_ = N;
                    other.size_ = 0;
                }
            }
            return *this;
        }

        // 반복자 (Iterators)
        iterator begin() noexcept { return data_; }
        const_iterator begin() const noexcept { return data_; }
        const_iterator cbegin() const noexcept { return data_; }

        iterator end() noexcept { return data_ + size_; }
        const_iterator end() const noexcept { return data_ + size_; }
        const_iterator cend() const noexcept { return data_ + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type max_size() const noexcept { return alloc_traits::max_size(allocator_); }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

        void reserve(size_type new_cap) {
            if (new_cap <= capacity_) return;

            size_type allocate_cap = std::max(new_cap, capacity_ == 0 ? static_cast<size_type>(1) : capacity_ * 2);
            pointer new_data = alloc_traits::allocate(allocator_, allocate_cap);

            try {
                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::construct(allocator_, new_data + i, std::move_if_noexcept(data_[i]));
                }
            }
            catch (...) {
                alloc_traits::deallocate(allocator_, new_data, allocate_cap);
                throw;
            }

            destroy_elements();
            free_storage();

            data_ = new_data;
            capacity_ = allocate_cap;
        }

        void shrink_to_fit() {
            if (is_on_stack() || size_ == capacity_) return;

            if (size_ <= N) {
                pointer old_data = data_;
                size_type old_cap = capacity_;

                data_ = reinterpret_cast<pointer>(stack_buffer_);
                capacity_ = N;

                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::construct(allocator_, data_ + i, std::move_if_noexcept(old_data[i]));
                    alloc_traits::destroy(allocator_, old_data + i);
                }
                alloc_traits::deallocate(allocator_, old_data, old_cap);
            }
            else {
                pointer new_data = alloc_traits::allocate(allocator_, size_);
                try {
                    for (size_type i = 0; i < size_; ++i) {
                        alloc_traits::construct(allocator_, new_data + i, std::move_if_noexcept(data_[i]));
                    }
                }
                catch (...) {
                    alloc_traits::deallocate(allocator_, new_data, size_);
                    throw;
                }
                destroy_elements();
                alloc_traits::deallocate(allocator_, data_, capacity_);
                data_ = new_data;
                capacity_ = size_;
            }
        }

        reference operator[](size_type pos) noexcept { return data_[pos]; }
        const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

        pointer at(size_type pos) noexcept {
            if (pos >= size_) return nullptr;
            return data_ + pos;
        }
        const_pointer at(size_type pos) const noexcept {
            if (pos >= size_) return nullptr;
            return data_ + pos;
        }

        reference front() { return data_[0]; }
        const_reference front() const { return data_[0]; }

        reference back() { return data_[size_ - 1]; }
        const_reference back() const { return data_[size_ - 1]; }

        pointer data() noexcept { return data_; }
        const_pointer data() const noexcept { return data_; }

        void clear() noexcept {
            destroy_elements();
            size_ = 0;
        }

        void push_back(const T& value) {
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_traits::construct(allocator_, data_ + size_, value);
            ++size_;
        }

        void push_back(T&& value) {
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_traits::construct(allocator_, data_ + size_, std::move(value));
            ++size_;
        }

        template <typename... Args>
        reference emplace_back(Args&&... args) {
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_traits::construct(allocator_, data_ + size_, std::forward<Args>(args)...);
            ++size_;
            return data_[size_ - 1];
        }

        void pop_back() {
            if (size_ > 0) {
                alloc_traits::destroy(allocator_, data_ + size_ - 1);
                --size_;
            }
        }

        void resize(size_type count) {
            if (count < size_) {
                while (size_ > count) {
                    pop_back();
                }
            }
            else if (count > size_) {
                reserve(count);
                while (size_ < count) {
                    alloc_traits::construct(allocator_, data_ + size_);
                    ++size_;
                }
            }
        }

        void resize(size_type count, const T& value) {
            if (count < size_) {
                while (size_ > count) {
                    pop_back();
                }
            }
            else if (count > size_) {
                reserve(count);
                while (size_ < count) {
                    alloc_traits::construct(allocator_, data_ + size_, value);
                    ++size_;
                }
            }
        }

        void swap(small_vector& other) noexcept(std::is_nothrow_swappable_v<T>) {
            if (this == &other) return;

            if constexpr (alloc_traits::propagate_on_container_swap::value) {
                std::swap(allocator_, other.allocator_);
            }

            small_vector temp(std::move(*this));
            *this = std::move(other);
            other = std::move(temp);
        }

        friend void swap(small_vector& lhs, small_vector& rhs) noexcept(noexcept(lhs.swap(rhs))) {
            lhs.swap(rhs);
        }

        // ==========================================
        // 순수 ASCII 기반 Small Vector Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            std::string storage_type_str = is_on_stack() ? "STACK (SBO Active)" : "HEAP (Dynamically Allocated)";
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << "/" << capacity_ << ", SBO Limit: " << N << ") ===\n";
            }
            else {
                std::cout << "=== Small Vector Dump (Size: " << size_ << "/" << capacity_ << ", SBO Limit: " << N << ") ===\n";
            }

            std::cout << "Storage: [" << storage_type_str << "]\n";
            if (empty()) {
                std::cout << "  \\-- <Empty Vector>\n\n";
                return;
            }

            std::cout << "Elements: ";
            for (size_type i = 0; i < size_; ++i) {
                std::cout << "[" << data_[i] << "]" << (i + 1 == size_ ? "" : " -> ");
            }
            std::cout << "\n\n";
        }
    };

} // namespace mino::core::container
