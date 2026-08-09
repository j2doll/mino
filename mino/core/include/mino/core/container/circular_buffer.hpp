#pragma once

#include <vector>
#include <optional>
#include <cstddef>

namespace mino::core::container {

    template <typename T>
    class  circular_buffer {
    public:
        // Constructor: non-throwing. If capacity == 0, buffer will be invalid (is_valid() == false).
        explicit circular_buffer(size_t capacity)
            : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0), valid_(capacity != 0) {
        }

        [[nodiscard]] bool is_valid() const noexcept { return valid_; }

        // data addition
        void push_back(const T& item) {
            if (!valid_) return;
            buffer_[tail_] = item;
            if (is_full()) {
                head_ = (head_ + 1) % capacity_;
            }
            else {
                ++size_;
            }
            tail_ = (tail_ + 1) % capacity_;
        }

        // pop front
        std::optional<T> pop_front() noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            T item = buffer_[head_];
            head_ = (head_ + 1) % capacity_;
            --size_;
            return item;
        }

        // front/back as optional
        std::optional<T> front() const noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            return buffer_[head_];
        }

        std::optional<T> back() const noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            size_t last_idx = (tail_ == 0) ? capacity_ - 1 : tail_ - 1;
            return buffer_[last_idx];
        }

        bool is_empty() const noexcept { return size_ == 0; }
        bool is_full() const noexcept { return size_ == capacity_; }
        size_t size() const noexcept { return size_; }
        size_t capacity() const noexcept { return capacity_; }

        void clear() noexcept {
            head_ = 0;
            tail_ = 0;
            size_ = 0;
        }

        // operator[]: non-throwing, unchecked access (matches typical operator[] semantics)
        // Caller must ensure index < size().
        T& operator[](size_t index) noexcept {
            return buffer_[(head_ + index) % capacity_];
        }

        const T& operator[](size_t index) const noexcept {
            return buffer_[(head_ + index) % capacity_];
        }

    private:
        std::vector<T> buffer_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        size_t size_;
        bool valid_;
    };

} // namespace mino::core::container
