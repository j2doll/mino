#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <string>
#include <vector>

// ============+====================+====================+==================
// 항목        |  이진 힙 (Binary)  |   이항 힙 (Binomial)|  페어링 힙 (Pairing)
// ------------+--------------------+--------------------+------------------
// 개념 분류   | 자료 구조(구현체)  | 자료 구조(구현체)  | 자료 구조(구현체)
// 내부 형태   | 완전 이진 트리     | 이항 트리 숲       | 다항 트리 (N진)
//             | (배열 기반)        | (포인터 체인 집합)| (Child-Sibling)
// ------------+--------------------+--------------------+------------------
// 최상위 확인 | O(1)               | O(log N) (루트탐색)| O(1)
// 삽입(Push)  | O(log N)           | O(1) [상환]        | O(1)
// 삭제(Pop)   | O(log N)           | O(log N)           | O(log N) [상환]
// 병합(Merge) | O(N)               | O(log N)           | O(1)
// ------------+--------------------+--------------------+------------------
// 특징        | 단순 큐 작업 적합  | 이진수 덧셈식 병합 | 다익스트라 등 추천
// ============+====================+====================+==================
//
// binomial_heap은 이항 트리(B_0, B_1, B_2, ...)들의 숲(Forest)으로 구성된 힙입니다.
//
// binomial_heap<int> heap; // 기본: Max-Heap (std::less<int>)
// 
// // [1] 요소 삽입 (Push)
// heap.push(5);
// heap.push(10);
// heap.push(3);
// heap.push(15);
// // 4개 삽입 완료 -> 이진수 4 = 100(2) 이므로 B_2 트리 1개로 구성됨
// 
// // [2] 최상위 요소 확인 (Top)
// std::cout << "Top: " << heap.top().value() << std::endl; // 15
// 
// // [3] 최상위 요소 제거 (Pop)
// heap.pop(); // 15 제거 -> 3개 남음 (이진수 3 = 11(2) -> B_1, B_0 트리로 분할)
// std::cout << "Size: " << heap.size() << std::endl;        // 3
// 
// // [4] 두 힙 병합 (Merge)
// binomial_heap<int> heap2;
// heap2.push(20);
// heap2.push(8); // 2개 -> B_1 트리 1개
// heap.merge(heap2); // 3 + 2 = 5개 (이진수 5 = 101(2) -> B_2, B_0 트리로 병합)
// 
// std::cout << "After merge size: " << heap.size() << std::endl; // 5
// std::cout << "Top: " << heap.top().value() << std::endl;        // 20
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class binomial_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        struct node {
            value_type value;
            size_type degree = 0;
            node* child = nullptr;
            node* sibling = nullptr;
            template <typename... Args>
            node(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        using handle_type = node*;

        binomial_heap() : head_(nullptr), size_(0), comp_(Compare()) {}
        ~binomial_heap() { clear(); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return find_max_node()->value;
        }

        template <typename... Args>
        handle_type emplace(Args&&... args) {
            handle_type new_node = new node(std::forward<Args>(args)...);
            binomial_heap temp_heap;
            temp_heap.head_ = new_node;
            temp_heap.size_ = 1;
            this->merge(temp_heap);
            return new_node;
        }

        handle_type push(const value_type& value) { return emplace(value); }

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;

            // 1. 루트 리스트에서 우선순위 최상위 노드 탐색
            node* max_prev = nullptr;
            node* max_curr = head_;
            node* prev = nullptr;
            node* curr = head_;
            value_type max_val = head_->value;

            while (curr) {
                if (comp_(max_val, curr->value)) {
                    max_val = curr->value;
                    max_prev = prev;
                    max_curr = curr;
                }
                prev = curr;
                curr = curr->sibling;
            }

            // 2. 루트 리스트에서 분리
            if (max_prev) max_prev->sibling = max_curr->sibling;
            else head_ = max_curr->sibling;

            // 3. 자식 노드 리스트를 역순으로 뒤집음
            node* child_curr = max_curr->child;
            node* reversed_child_head = nullptr;
            while (child_curr) {
                node* next = child_curr->sibling;
                child_curr->sibling = reversed_child_head;
                reversed_child_head = child_curr;
                child_curr = next;
            }

            // 4. B_k의 자식 노드 개수는 O(1) 산술 연산 (2^degree - 1)으로 계산
            size_type child_count = (size_type(1) << max_curr->degree) - 1;
            size_ -= (1 + child_count);

            binomial_heap temp_heap;
            temp_heap.head_ = reversed_child_head;
            temp_heap.size_ = child_count;

            delete max_curr;

            // 5. 자식 서브트리 병합
            this->merge(temp_heap);
            return true;
        }

        void merge(binomial_heap& other) {
            if (this == &other || other.empty()) return;

            head_ = merge_roots(head_, other.head_);
            size_ += other.size_;
            other.head_ = nullptr;
            other.size_ = 0;

            if (!head_) return;

            node* prev = nullptr;
            node* curr = head_;
            node* next = curr->sibling;

            while (next) {
                if ((curr->degree != next->degree) || (next->sibling && next->sibling->degree == curr->degree)) {
                    prev = curr;
                    curr = next;
                }
                else if (comp_(next->value, curr->value)) {
                    curr->sibling = next->sibling;
                    link_trees(next, curr);
                }
                else {
                    if (!prev) head_ = next;
                    else prev->sibling = next;
                    link_trees(curr, next);
                    curr = next;
                }
                next = curr->sibling;
            }
        }

        void clear() noexcept {
            destroy_nodes(head_);
            head_ = nullptr;
            size_ = 0;
        }

        // ==========================================
        // 순수 ASCII 기반 Tree Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << ") ===\n";
            }
            else {
                std::cout << "=== Binomial Heap Dump (Size: " << size_ << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Heap>\n\n";
                return;
            }

            node* curr_tree = head_;
            while (curr_tree) {
                std::cout << "* Tree B" << curr_tree->degree << " (Root: [" << curr_tree->value << "])\n";
                std::cout << "  [" << curr_tree->value << "]\n";
                dump_children(curr_tree, "  ");
                curr_tree = curr_tree->sibling;
            }
            std::cout << "\n";
        }

    private:
        node* head_;
        size_type size_;
        Compare comp_;

        void dump_children(node* parent, const std::string& prefix) const {
            std::vector<node*> children;
            for (node* curr = parent->child; curr != nullptr; curr = curr->sibling) {
                children.push_back(curr);
            }

            for (size_t i = 0; i < children.size(); ++i) {
                bool is_last = (i == children.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "[" << children[i]->value << "]\n";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_children(children[i], next_prefix);
            }
        }

        node* find_max_node() const {
            node* curr = head_;
            node* max_node = head_;
            while (curr) {
                if (comp_(max_node->value, curr->value)) max_node = curr;
                curr = curr->sibling;
            }
            return max_node;
        }

        void link_trees(node* child, node* parent) {
            child->sibling = parent->child;
            parent->child = child;
            parent->degree++;
        }

        node* merge_roots(node* h1, node* h2) {
            if (!h1) return h2;
            if (!h2) return h1;
            node* root_head = nullptr;
            node* root_tail = nullptr;
            while (h1 && h2) {
                node*& chosen = (h1->degree <= h2->degree) ? h1 : h2;
                if (!root_head) { root_head = root_tail = chosen; }
                else { root_tail->sibling = chosen; root_tail = chosen; }
                chosen = chosen->sibling;
            }
            if (h1) root_tail->sibling = h1;
            if (h2) root_tail->sibling = h2;
            return root_head;
        }

        void destroy_nodes(node* n) noexcept {
            while (n) {
                node* next = n->sibling;
                if (n->child) destroy_nodes(n->child);
                delete n;
                n = next;
            }
        }
    };

} // namespace mino::core::container

