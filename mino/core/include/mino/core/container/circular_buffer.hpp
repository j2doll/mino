#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include <utility>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::deque        |  circular_buffer
// ------------+--------------------+--------------------+------------------
// 용량 특성   | 동적 확장 (재할당) | 동적 청크 확장     | 고정 크기 (재할당 없음)
// 메모리 할당 | 힙 (불연속 재할당) | 다중 청크 할당     | 생성 시 1회 고정 할당
// 오버플로우  | 크기 자동 증가     | 크기 자동 증가     | 오래된 데이터 덮어쓰기
// ------------+--------------------+--------------------+------------------
// 맨 뒤 삽입  | O(1) [상환]        | O(1)               | O(1)
// 맨 앞 제거  | O(N) (데이터 이동) | O(1)               | O(1) (포인터 이동)
// 인덱스 접근 | O(1)               | O(1)               | O(1) (모듈로 연산)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 범용 연속 컨테이너 | 양방향 큐          | 실시간 로그/오디오 스트림
//             |                    |                    | 네트워크 링 버퍼
// ============+====================+====================+==================
//
// circular_buffer는 고정된 크기(Capacity) 내에서 데이터를 FIFO 형태로 관리하며,
// 용량 초과 시 가장 오래된 데이터를 자동으로 덮어쓰는(Overwrite) 원형 링 버퍼입니다.
//
// circular_buffer<int> buf(5); // 용량 5인 원형 버퍼 생성
// 
// // [1] 데이터 추가 (O(1))
// buf.push_back(10);
// buf.push_back(20);
// buf.push_back(30);
// 
// // [2] 상태 확인 (std::optional 반환)
// std::cout << "Size: " << buf.size() << std::endl;             // 3
// std::cout << "Front: " << buf.front().value() << std::endl;   // 10
// std::cout << "Back: " << buf.back().value() << std::endl;     // 30
// 
// // [3] 인덱스 접근 (O(1))
// std::cout << buf[0] << " " << buf[1] << " " << buf[2] << std::endl; // 10 20 30
// 
// // [4] 데이터 제거 (O(1))
// auto val = buf.pop_front();
// std::cout << "Popped: " << val.value() << std::endl;          // 10
// std::cout << "Size: " << buf.size() << std::endl;              // 2
// 
// // [5] 용량 초과 시 자동 덮어쓰기
// buf.push_back(40);
// buf.push_back(50);
// buf.push_back(60);
// buf.push_back(70);
// buf.push_back(80); // 용량 초과 시 head_ 이동
// 
// std::cout << "Is Full: " << buf.is_full() << std::endl;       // 1 (true)
// std::cout << "Front: " << buf.front().value() << std::endl;   // 40
// 
// // [6] 초기화
// buf.clear();
// std::cout << "Is Empty: " << buf.is_empty() << std::endl;     // 1 (true)
// 

namespace mino::core::container {

    template <typename T>
    class circular_buffer {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using reference = T&;
        using const_reference = const T&;

        // Constructor: capacity가 0이면 is_valid() == false
        explicit circular_buffer(size_type capacity)
            : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0), valid_(capacity != 0) {
        }

        [[nodiscard]] bool is_valid() const noexcept { return valid_; }
        [[nodiscard]] bool is_empty() const noexcept { return size_ == 0; }
        [[nodiscard]] bool is_full() const noexcept { return size_ == capacity_; }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

        // --------------------------------------------------------------------
        // 1. 데이터 삽입
        // --------------------------------------------------------------------
        void push_back(const T& item) {
            if (!valid_) return;
            buffer_[tail_] = item;
            advance_tail();
        }

        void push_back(T&& item) {
            if (!valid_) return;
            buffer_[tail_] = std::move(item);
            advance_tail();
        }

        template <typename... Args>
        void emplace_back(Args&&... args) {
            if (!valid_) return;
            buffer_[tail_] = T(std::forward<Args>(args)...);
            advance_tail();
        }

        // --------------------------------------------------------------------
        // 2. 데이터 제거
        // --------------------------------------------------------------------
        std::optional<T> pop_front() noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            T item = std::move(buffer_[head_]);
            head_ = (head_ + 1) % capacity_;
            --size_;
            return item;
        }

        // --------------------------------------------------------------------
        // 3. 데이터 조회 (std::optional 반환으로 안전성 보장)
        // --------------------------------------------------------------------
        [[nodiscard]] std::optional<T> front() const noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            return buffer_[head_];
        }

        [[nodiscard]] std::optional<T> back() const noexcept {
            if (!valid_ || is_empty()) return std::nullopt;
            size_type last_idx = (tail_ == 0) ? capacity_ - 1 : tail_ - 1;
            return buffer_[last_idx];
        }

        // --------------------------------------------------------------------
        // 4. 인덱스 접근
        // --------------------------------------------------------------------
        reference operator[](size_type index) noexcept {
            return buffer_[(head_ + index) % capacity_];
        }

        const_reference operator[](size_type index) const noexcept {
            return buffer_[(head_ + index) % capacity_];
        }

        void clear() noexcept {
            head_ = 0;
            tail_ = 0;
            size_ = 0;
        }

        // --------------------------------------------------------------------
        // 5. 순수 ASCII 기반 Buffer Dump
        // --------------------------------------------------------------------
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << "/" << capacity_ << ") ===\n";
            }
            else {
                std::cout << "=== Circular Buffer Dump (Size: " << size_ << "/" << capacity_ << ") ===\n";
            }

            if (!valid_ || capacity_ == 0) {
                std::cout << "  \\-- <Invalid Buffer>\n\n";
                return;
            }

            std::cout << "Logical Order: ";
            if (is_empty()) {
                std::cout << "<Empty>\n";
            }
            else {
                for (size_type i = 0; i < size_; ++i) {
                    std::cout << "[" << (*this)[i] << "]" << (i + 1 == size_ ? "" : " -> ");
                }
                std::cout << "\n";
            }

            std::cout << "Buffer Array:\n";
            for (size_type i = 0; i < capacity_; ++i) {
                bool is_last = (i == capacity_ - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << i << "] : ";

                bool occupied = false;
                if (!is_empty()) {
                    if (head_ < tail_) {
                        occupied = (i >= head_ && i < tail_);
                    }
                    else {
                        occupied = (i >= head_ || i < tail_);
                    }
                }

                if (occupied) {
                    std::cout << buffer_[i];
                }
                else {
                    std::cout << "<Empty>";
                }

                if (i == head_ && !is_empty()) std::cout << " (HEAD)";
                if (i == tail_) std::cout << " (TAIL)";
                std::cout << "\n";
            }
            std::cout << "\n";
        }

    private:
        void advance_tail() noexcept {
            if (is_full()) {
                head_ = (head_ + 1) % capacity_;
            }
            else {
                ++size_;
            }
            tail_ = (tail_ + 1) % capacity_;
        }

        std::vector<T> buffer_;
        size_type capacity_;
        size_type head_;
        size_type tail_;
        size_type size_;
        bool valid_;
    };

} // namespace mino::core::container
