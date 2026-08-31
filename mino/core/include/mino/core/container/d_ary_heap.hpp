#pragma once

#include <vector>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <optional>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  이진 힙 (Binary)  |   D진 힙 (D-ary)   |  페어링 힙 (Pairing)
// ------------+--------------------+--------------------+------------------
// 노드당 자식 | 최대 2개           | 최대 D개 (기본: 4) | 제한 없음 (N진)
// 내부 구현   | 연속 배열 (Vector) | 연속 배열 (Vector) | 포인터 체인 (Child-Sibling)
// 캐시 효율   | 우수               | 최상 (연속 블록)   | 보통 (노드 동적 할당)
// ------------+--------------------+--------------------+------------------
// 최상위 확인 | O(1)               | O(1)               | O(1)
// 삽입 (Push) | O(log_2 N)         | O(log_D N) (매우빠름)| O(1)
// 삭제 (Pop)  | O(log_2 N)         | O(D * log_D N)     | O(log N) [상환]
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 범용 기본 큐       | Push가 빈번하고    | 다익스트라 등 감소 연산
//             |                    | 캐시 성능 극대화   | 빈번한 특수 알고리즘
// ============+====================+====================+==================
//
// D-ary 힙(d-ary heap)은 각 노드가 최대 D개의 자식을 가질 수 있는 연속 배열 기반 힙입니다.
//
// // D=4 인 4진 힙 생성 (기본: Max-Heap)
// d_ary_heap<int> heap;
// 
// // [1] 요소 삽입 (O(log_D N))
// heap.push(10); // 배열: [10]
// heap.push(5);  // 배열: [10, 5]
// heap.push(20); // 배열: [20, 5, 10] (20이 루트로 승격)
// heap.push(15); // 배열: [20, 15, 10, 5]
// heap.push(3);  // 배열: [20, 15, 10, 5, 3]
// 
// // [2] 상태 확인
// std::cout << "Size: " << heap.size() << std::endl;         // 5
// std::cout << "Top: " << heap.top().value() << std::endl;   // 20
// 
// // [3] 최상위 요소 제거 (O(D * log_D N))
// heap.pop(); // 배열: [15, 3, 10, 5]
// std::cout << "After pop: " << heap.top().value() << std::endl; // 15
// 
// // [4] emplace를 이용한 직접 생성
// heap.emplace(25); // 배열: [25, 15, 10, 5, 3]
// std::cout << "After emplace: " << heap.top().value() << std::endl; // 25
// 
// // [5] D=3 인 3진 힙 (std::greater로 최소 힙 생성)
// d_ary_heap<int, 3, std::greater<int>> min_heap;
// min_heap.push(10); // [10]
// min_heap.push(5);  // [5, 10]
// min_heap.push(20); // [5, 10, 20]
// min_heap.push(3);  // [3, 10, 20, 5] (3이 루트로 승격)
// 
// std::cout << "Min heap top: " << min_heap.top().value() << std::endl; // 3
// min_heap.pop();    // [5, 10, 20]
// std::cout << "Min heap after pop: " << min_heap.top().value() << std::endl; // 5
// 
// // [6] 초기화
// heap.clear();
// std::cout << "Is empty: " << heap.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, std::size_t D = 4, typename Compare = std::less<T>>
    class d_ary_heap {
        static_assert(D >= 2, "d_ary_heap degree D must be at least 2");

    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        d_ary_heap() : comp_(Compare()) {}
        explicit d_ary_heap(const Compare& comp) : comp_(comp) {}

        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return data_.front();
        }

        void push(const T& value) {
            data_.push_back(value);
            sift_up(data_.size() - 1);
        }

        void push(T&& value) {
            data_.push_back(std::move(value));
            sift_up(data_.size() - 1);
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            data_.emplace_back(std::forward<Args>(args)...);
            sift_up(data_.size() - 1);
        }

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            if (data_.size() == 1) {
                data_.pop_back();
                return true;
            }
            data_.front() = std::move(data_.back());
            data_.pop_back();
            sift_down(0);
            return true;
        }

        void clear() noexcept {
            data_.clear();
        }

        // ==========================================
        // 순수 ASCII 기반 D-ary Tree Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << data_.size() << ", D=" << D << ") ===\n";
            }
            else {
                std::cout << "=== D-ary Heap Dump (Size: " << data_.size() << ", D=" << D << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Heap>\n\n";
                return;
            }

            // 배열 형태 출력
            std::cout << "Array: [";
            for (size_type i = 0; i < data_.size(); ++i) {
                std::cout << data_[i] << (i + 1 == data_.size() ? "" : ", ");
            }
            std::cout << "]\n";

            // 트리 형태 출력
            std::cout << "Tree:\n[" << data_[0] << "]\n";
            dump_children(0, "");
            std::cout << "\n";
        }

    private:
        std::vector<T> data_;
        Compare comp_;

        void dump_children(size_type parent_idx, const std::string& prefix) const {
            size_type first_child = parent_idx * D + 1;
            if (first_child >= data_.size()) return;

            size_type last_child = std::min(first_child + D, data_.size());
            for (size_type i = first_child; i < last_child; ++i) {
                bool is_last = (i == last_child - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "[" << data_[i] << "]\n";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_children(i, next_prefix);
            }
        }

        void sift_up(size_type index) {
            while (index > 0) {
                size_type parent = (index - 1) / D;
                if (comp_(data_[parent], data_[index])) {
                    std::swap(data_[parent], data_[index]);
                    index = parent;
                }
                else {
                    break;
                }
            }
        }

        void sift_down(size_type index) {
            size_type current_size = data_.size();
            while (true) {
                size_type best = index;
                size_type first_child = index * D + 1;

                for (size_type i = 0; i < D; ++i) {
                    size_type child = first_child + i;
                    if (child < current_size && comp_(data_[best], data_[child])) {
                        best = child;
                    }
                }
                if (best != index) {
                    std::swap(data_[index], data_[best]);
                    index = best;
                }
                else {
                    break;
                }
            }
        }
    };

} // namespace mino::core::container

