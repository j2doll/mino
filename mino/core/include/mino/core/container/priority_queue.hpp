#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <optional>
#include <initializer_list>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  우선순위 큐       |   D진 힙 (D-ary)   |  피보나치 힙
//             |  priority_queue    |  d_ary_heap        |  fibonacci_heap
// ------------+--------------------+--------------------+------------------
// 개념 분류   | ADT / 기본 힙      | 자료 구조(구현체)  | 자료 구조(구현체)
// 내부 형태   | 완전 이진 트리     | D진 완전 트리      | 원형 이중 연결
// (메모리 구조| (단일 연속 배열)   | (단일 연속 배열)   | 리스트 트리 숲
// ------------+--------------------+--------------------+------------------
// 최상위 확인 | O(1)               | O(1)               | O(1)
// 삽입(Push)  | O(log_2 N)         | O(log_D N)         | O(1)
// 삭제(Pop)   | O(log_2 N)         | O(D * log_D N)     | O(log N) [상환]
// 병합(Merge) | O(N)               | O(N)               | O(1)
// ------------+--------------------+--------------------+------------------
// 메모리 소모 | 매우 적음 (배열)   | 매우 적음 (배열)   | 매우 큼 (포인터 4개)
// 실무 추천   | 범용 기본 우선순위 큐| Push 빈도가 높고   | 값 감소가 빈번한
//             |                    | 캐시 성능 극대화   | 특수 알고리즘
// ============+====================+====================+==================
//
// priority_queue는 연속 메모리(std::vector) 기반의 완전 이진 힙으로 동작하는 컨테이너입니다.
//
// // [1] 최대값 우선 우선순위 큐 (기본: Max-Heap)
// priority_queue<int> pq;
// 
// // 요소 삽입 (O(log N))
// pq.push(10);
// pq.push(5);
// pq.push(20);
// pq.push(15);
// pq.push(3);
// 
// // 상태 확인
// std::cout << "Size: " << pq.size() << std::endl;             // 5
// std::cout << "Top (max): " << pq.top().value() << std::endl; // 20
// 
// // [2] 최상위 요소 제거 (내림차순 정렬 추출)
// while (!pq.empty()) {
//     auto val = pq.top();
//     std::cout << val.value() << " ";
//     pq.pop();
// }
// std::cout << std::endl; // 20 15 10 5 3
// 
// // [3] 최소값 우선 우선순위 큐 (Min-Heap)
// priority_queue<int, std::greater<int>> min_pq;
// min_pq.push(10);
// min_pq.push(5);
// min_pq.push(20);
// min_pq.push(15);
// min_pq.push(3);
// 
// std::cout << "Min PQ Top: " << min_pq.top().value() << std::endl; // 3
// 
// // [4] emplace를 이용한 직접 생성
// priority_queue<int> pq2;
// pq2.emplace(7);
// pq2.emplace(2);
// pq2.emplace(9);
// std::cout << "PQ2 Top: " << pq2.top().value() << std::endl; // 9
// 
// // [5] Swap 및 Clear
// priority_queue<int> pq3, pq4;
// pq3.push(100);
// pq3.push(200);
// pq4.push(1);
// pq4.push(2);
// 
// pq3.swap(pq4);
// std::cout << "After swap - pq3 top: " << pq3.top().value() << ", pq4 top: " << pq4.top().value() << std::endl;
// // After swap - pq3 top: 2, pq4 top: 200
// 
// pq3.clear();
// std::cout << "After clear, is empty: " << pq3.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>, typename Container = std::vector<T>>
    class priority_queue {
    public:
        using value_type = T;
        using container_type = Container;
        using compare_type = Compare;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;

        // 1. 기본 생성자
        priority_queue() : c_(), comp_() {}

        // 2. 비교자 전달 생성자
        explicit priority_queue(const Compare& comp) : c_(), comp_(comp) {}

        // 3. 컨테이너 및 비교자 생성자
        priority_queue(const Compare& comp, const Container& cont) : c_(cont), comp_(comp) {
            std::make_heap(c_.begin(), c_.end(), comp_);
        }

        priority_queue(const Compare& comp, Container&& cont) : c_(std::move(cont)), comp_(comp) {
            std::make_heap(c_.begin(), c_.end(), comp_);
        }

        // 4. 범위 기반 생성자 (O(N) make_heap)
        template <typename InputIt>
        priority_queue(InputIt first, InputIt last, const Compare& comp = Compare())
            : c_(first, last), comp_(comp) {
            std::make_heap(c_.begin(), c_.end(), comp_);
        }

        // 5. Initializer list 생성자
        priority_queue(std::initializer_list<value_type> init, const Compare& comp = Compare())
            : c_(init), comp_(comp) {
            std::make_heap(c_.begin(), c_.end(), comp_);
        }

        [[nodiscard]] bool empty() const noexcept { return c_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return c_.size(); }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return c_.front();
        }

        void push(const value_type& value) {
            c_.push_back(value);
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        void push(value_type&& value) {
            c_.push_back(std::move(value));
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            c_.emplace_back(std::forward<Args>(args)...);
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            std::pop_heap(c_.begin(), c_.end(), comp_);
            c_.pop_back();
            return true;
        }

        void clear() noexcept {
            c_.clear();
        }

        void swap(priority_queue& other) noexcept {
            using std::swap;
            swap(c_, other.c_);
            swap(comp_, other.comp_);
        }

        // ==========================================
        // 순수 ASCII 기반 Binary Tree Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << c_.size() << ") ===\n";
            }
            else {
                std::cout << "=== Priority Queue Dump (Size: " << c_.size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Queue>\n\n";
                return;
            }

            std::cout << "Array: [";
            for (size_type i = 0; i < c_.size(); ++i) {
                std::cout << c_[i] << (i + 1 == c_.size() ? "" : ", ");
            }
            std::cout << "]\n";

            std::cout << "Tree:\n[" << c_[0] << "]\n";
            dump_children(0, "");
            std::cout << "\n";
        }

    private:
        void dump_children(size_type parent_idx, const std::string& prefix) const {
            size_type left = 2 * parent_idx + 1;
            size_type right = 2 * parent_idx + 2;

            std::vector<size_type> children;
            if (left < c_.size()) children.push_back(left);
            if (right < c_.size()) children.push_back(right);

            for (size_t i = 0; i < children.size(); ++i) {
                bool is_last = (i == children.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "[" << c_[children[i]] << "]\n";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_children(children[i], next_prefix);
            }
        }

        Container c_;
        Compare comp_;
    };

} // namespace mino::core::container
