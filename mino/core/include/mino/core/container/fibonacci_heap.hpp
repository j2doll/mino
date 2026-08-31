#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <optional>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  이진 힙 (Binary)  |   피보나치 힙      |  페어링 힙 (Pairing)
//             |  priority_queue    |  fibonacci_heap    |  pairing_heap
// ------------+--------------------+--------------------+------------------
// 개념 분류   | 자료 구조(구현체)  | 자료 구조(구현체)  | 자료 구조(구현체)
// 내부 형태   | 완전 이진 트리     | 원형 이중 연결     | 다항 트리 (N진)
//             | (배열 기반)        | 리스트 트리 숲     | (Child-Sibling)
// ------------+--------------------+--------------------+------------------
// 최상위 확인 | O(1)               | O(1)               | O(1)
// 삽입(Push)  | O(log N)           | O(1)               | O(1)
// 삭제(Pop)   | O(log N)           | O(log N) [상환]    | O(log N) [상환]
// 값 감소     | 지원 안 하거나 느림| O(1) [상환]        | O(log N) [상환]
// 병합(Merge) | O(N)               | O(1)               | O(1)
// ------------+--------------------+--------------------+------------------
// 메모리 소모 | 매우 적음 (배열)   | 큼 (노드당 포인터4)| 보통 (포인터 3개)
// 실무 추천   | 단순 큐 작업       | 값 감소가 빈번한   | 대부분의 고성능
//             |                    | 이론적 특수 알고리즘| 우선순위 큐 작업
// ============+====================+====================+==================
//
// fibonacci_heap은 원형 이중 연결 리스트 기반의 트리 숲(Forest) 구조를 가지며,
// 지연 병합(Lazy Consolidation)을 통해 O(1) 삽입과 O(1) 상환 병합을 지원합니다.
//
// fibonacci_heap<int> heap; // 기본: Max-Heap (std::less<int>)
// 
// // [1] 요소 삽입 (O(1))
// auto h1 = heap.push(10);
// auto h2 = heap.push(5);
// auto h3 = heap.push(20);
// auto h4 = heap.push(15);
// auto h5 = heap.push(3);
// 
// // [2] 상태 확인
// std::cout << "Size: " << heap.size() << std::endl;         // 5
// std::cout << "Top: " << heap.top().value() << std::endl;   // 20
// 
// // [3] 최상위 요소 제거 (O(log N) 상환: Consolidate 수행)
// heap.pop(); // 20 제거 후 트리들이 차수별로 정리됨
// std::cout << "After pop: " << heap.top().value() << std::endl; // 15
// std::cout << "Size: " << heap.size() << std::endl;              // 4
// 
// // [4] emplace를 이용한 직접 생성
// heap.emplace(25);
// std::cout << "After emplace: " << heap.top().value() << std::endl; // 25
// 
// // [5] 두 힙 병합 (O(1))
// fibonacci_heap<int> heap2;
// heap2.push(30);
// heap2.push(8);
// heap2.push(12);
// 
// heap.merge(heap2);
// std::cout << "After merge size: " << heap.size() << std::endl;        // 8 (5 - 1 + 1 + 3)
// std::cout << "After merge top: " << heap.top().value() << std::endl;  // 30
// 
// // [6] 초기화
// heap.clear();
// std::cout << "Is empty: " << heap.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class fibonacci_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        struct node {
            value_type value;
            size_type degree = 0;
            bool marked = false;
            node* parent = nullptr;
            node* child = nullptr;
            node* left = this;
            node* right = this;
            template <typename... Args>
            node(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        using handle_type = node*;

        fibonacci_heap() : max_node_(nullptr), size_(0), comp_(Compare()) {}
        ~fibonacci_heap() { clear(); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return max_node_->value;
        }

        template <typename... Args>
        handle_type emplace(Args&&... args) {
            handle_type new_node = new node(std::forward<Args>(args)...);
            max_node_ = merge_lists(max_node_, new_node);
            ++size_;
            return new_node;
        }

        handle_type push(const value_type& value) { return emplace(value); }

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            handle_type z = max_node_;
            if (z->child) {
                handle_type child = z->child;
                do {
                    handle_type next = child->right;
                    child->parent = nullptr;
                    child = next;
                } while (child != z->child);
                max_node_ = merge_lists(max_node_, z->child);
            }

            if (z->right == z) {
                max_node_ = nullptr;
            }
            else {
                z->left->right = z->right;
                z->right->left = z->left;
                max_node_ = z->right;
                consolidate();
            }

            delete z;
            --size_;
            return true;
        }

        void merge(fibonacci_heap& other) {
            if (this == &other || other.empty()) return;
            max_node_ = merge_lists(max_node_, other.max_node_);
            size_ += other.size_;
            other.max_node_ = nullptr;
            other.size_ = 0;
        }

        void clear() noexcept {
            if (max_node_) {
                destroy_list(max_node_);
                max_node_ = nullptr;
                size_ = 0;
            }
        }

        // ==========================================
        // 순수 ASCII 기반 Fibonacci Tree Forest Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << ") ===\n";
            }
            else {
                std::cout << "=== Fibonacci Heap Dump (Size: " << size_ << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Heap>\n\n";
                return;
            }

            std::vector<node*> roots;
            node* curr = max_node_;
            do {
                roots.push_back(curr);
                curr = curr->right;
            } while (curr != max_node_);

            for (node* r : roots) {
                std::cout << "* Root Tree (Root: [" << r->value << "], Degree: " << r->degree << ")\n";
                std::cout << "  [" << r->value << "]\n";
                dump_children(r, "  ");
            }
            std::cout << "\n";
        }

    private:
        node* max_node_;
        size_type size_;
        Compare comp_;

        void dump_children(node* parent, const std::string& prefix) const {
            if (!parent->child) return;

            std::vector<node*> children;
            node* curr = parent->child;
            do {
                children.push_back(curr);
                curr = curr->right;
            } while (curr != parent->child);

            for (size_t i = 0; i < children.size(); ++i) {
                bool is_last = (i == children.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "[" << children[i]->value << "]\n";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_children(children[i], next_prefix);
            }
        }

        node* merge_lists(node* a, node* b) {
            if (!a) return b;
            if (!b) return a;
            node* a_next = a->right;
            node* b_prev = b->left;
            a->right = b; b->left = a;
            a_next->left = b_prev; b_prev->right = a_next;
            return comp_(a->value, b->value) ? b : a;
        }

        void consolidate() {
            if (!max_node_) return;

            // 황금비 기반 최대 차수(1.4404 * log2(N) + 8) 계산
            size_type max_deg = static_cast<size_type>(2.0 * std::log2(static_cast<double>(size_ + 1))) + 8;
            std::vector<node*> degree_array(max_deg, nullptr);

            std::vector<node*> root_nodes;
            node* curr = max_node_;
            do {
                root_nodes.push_back(curr);
                curr = curr->right;
            } while (curr != max_node_);

            for (node* w : root_nodes) {
                node* x = w;
                size_type d = x->degree;
                while (d < degree_array.size() && degree_array[d] != nullptr) {
                    node* y = degree_array[d];
                    if (comp_(x->value, y->value)) std::swap(x, y);
                    link_nodes(y, x);
                    degree_array[d] = nullptr;
                    d++;
                }
                if (d >= degree_array.size()) {
                    degree_array.resize(d + 8, nullptr);
                }
                degree_array[d] = x;
            }

            max_node_ = nullptr;
            for (node* y : degree_array) {
                if (y) {
                    y->left = y; y->right = y;
                    max_node_ = merge_lists(max_node_, y);
                }
            }
        }

        void link_nodes(node* y, node* x) {
            y->left->right = y->right;
            y->right->left = y->left;
            y->parent = x;
            y->left = y;
            y->right = y;
            x->child = merge_lists(x->child, y);
            x->degree++;
            y->marked = false;
        }

        void destroy_list(node* start) noexcept {
            if (!start) return;
            start->left->right = nullptr; // 원형 순환 고리 해제
            node* curr = start;
            while (curr) {
                node* next = curr->right;
                if (curr->child) destroy_list(curr->child);
                delete curr;
                curr = next;
            }
        }
    };

} // namespace mino::core::container

