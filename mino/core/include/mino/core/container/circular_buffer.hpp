#pragma once

#include <vector>
#include <optional>
#include <stdexcept>
#include <cstddef>

namespace mino::core::container {

    template <typename T>
    class  circular_buffer {
    public:
        // 생성자
        explicit circular_buffer(size_t capacity)
            : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {
            if (capacity == 0) {
                throw std::invalid_argument("Capacity must be greater than zero.");
            }
        }

        // 데이터 추가
        void push_back(const T& item) {
            buffer_[tail_] = item;

            if (is_full()) {
                head_ = (head_ + 1) % capacity_;
            }
            else {
                ++size_;
            }

            tail_ = (tail_ + 1) % capacity_;
        }

        // 데이터 꺼내기
        std::optional<T> pop_front() {
            if (is_empty()) {
                return std::nullopt;
            }

            T item = buffer_[head_];
            head_ = (head_ + 1) % capacity_;
            --size_;
            return item;
        }

        // 첫 번째 원소 참조
        std::optional<T> front() const {
            if (is_empty()) return std::nullopt;
            return buffer_[head_];
        }

        // 마지막 원소 참조
        std::optional<T> back() const {
            if (is_empty()) return std::nullopt;
            size_t last_idx = (tail_ == 0) ? capacity_ - 1 : tail_ - 1;
            return buffer_[last_idx];
        }

        // 상태 확인 메서드
        bool is_empty() const { return size_ == 0; }
        bool is_full() const { return size_ == capacity_; }
        size_t size() const { return size_; }
        size_t capacity() const { return capacity_; }

        // 버퍼 초기화
        void clear() {
            head_ = 0;
            tail_ = 0;
            size_ = 0;
        }

        // 인덱스 기반 접근 (Non-const)
        T& operator[](size_t index) {
            if (index >= size_) {
                throw std::out_of_range("Index out of range");
            }
            return buffer_[(head_ + index) % capacity_];
        }

        // 인덱스 기반 접근 (Const)
        const T& operator[](size_t index) const {
            if (index >= size_) {
                throw std::out_of_range("Index out of range");
            }
            return buffer_[(head_ + index) % capacity_];
        }

    private:
        std::vector<T> buffer_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        size_t size_;
    };

}  
