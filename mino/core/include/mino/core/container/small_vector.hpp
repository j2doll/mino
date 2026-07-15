#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <iterator>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>

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

        alignas(T) char stack_buffer_[N * sizeof(T)];

        pointer data_ = reinterpret_cast<pointer>(stack_buffer_);
        size_type size_ = 0;
        size_type capacity_ = N;

        Allocator allocator_;

        bool is_on_stack() const noexcept {
            return data_ == reinterpret_cast<const_pointer>(stack_buffer_);
        }

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

        small_vector(const small_vector& other) : allocator_(alloc_traits::select_on_container_copy_construction(other.allocator_)) {
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

        small_vector(small_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) : allocator_(std::move(other.allocator_)) {
            if (other.is_on_stack()) {
                for (size_type i = 0; i < other.size_; ++i) {
                    alloc_traits::construct(allocator_, data_ + i, std::move(other.data_[i]));
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
                    allocator_ = std::move(other.allocator_); // 오타 수정 완료
                }

                if (other.is_on_stack()) {
                    data_ = reinterpret_cast<pointer>(stack_buffer_);
                    capacity_ = N;
                    for (size_type i = 0; i < other.size_; ++i) {
                        alloc_traits::construct(allocator_, data_ + i, std::move(other.data_[i]));
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

        // 용량 (Capacity)
        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }
        size_type max_size() const noexcept { return alloc_traits::max_size(allocator_); }
        size_type capacity() const noexcept { return capacity_; }

        void reserve(size_type new_cap) {
            if (new_cap <= capacity_) return;

            // Growth strategy:
            //  - prefer doubling the current capacity when growing
            //  - ensure at least new_cap
            //  - if the computed allocation equals new_cap, add an extra 1.5x buffer to provide headroom
            size_type allocate_cap = std::max(new_cap, capacity_ == 0 ? static_cast<size_type>(1) : capacity_ * 2);

            if (allocate_cap == new_cap) {
                // add additional headroom (1.5x) to avoid allocating exactly the requested size
                size_type extra = new_cap / 2;
                if (extra == 0) extra = 1;
                size_type bumped = new_cap + extra;
                if (bumped > allocate_cap) allocate_cap = bumped;
            }

            pointer new_data = alloc_traits::allocate(allocator_, allocate_cap);
            for (size_type i = 0; i < size_; ++i) {
                alloc_traits::construct(allocator_, new_data + i, std::move(data_[i]));
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
                    alloc_traits::construct(allocator_, data_ + i, std::move(old_data[i]));
                    alloc_traits::destroy(allocator_, old_data + i);
                }
                alloc_traits::deallocate(allocator_, old_data, old_cap);
            }
            else {
                pointer new_data = alloc_traits::allocate(allocator_, size_);
                for (size_type i = 0; i < size_; ++i) {
                    alloc_traits::construct(allocator_, new_data + i, std::move(data_[i]));
                }
                destroy_elements();
                alloc_traits::deallocate(allocator_, data_, capacity_);
                data_ = new_data;
                capacity_ = size_;
            }
        }

        // 원소 접근 (Element Access)
        reference operator[](size_type pos) { return data_[pos]; }
        const_reference operator[](size_type pos) const { return data_[pos]; }

        reference at(size_type pos) {
            if (pos >= size_) throw std::out_of_range("small_vector::at out of range");
            return data_[pos];
        }
        const_reference at(size_type pos) const {
            if (pos >= size_) throw std::out_of_range("small_vector::at out of range");
            return data_[pos];
        }

        reference front() { return data_[0]; }
        const_reference front() const { return data_[0]; }

        reference back() { return data_[size_ - 1]; }
        const_reference back() const { return data_[size_ - 1]; }

        pointer data() noexcept { return data_; }
        const_pointer data() const noexcept { return data_; }

        // 수정자 (Modifiers)
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
    };

} // namespace mino::core::container
