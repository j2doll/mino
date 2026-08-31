#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <string>
#include <vector>

// ============+====================+====================+==================
// 항목        |  이진 힙 (Binary)  |   스큐 힙 (Skew)   |  페어링 힙 (Pairing)
// ------------+--------------------+--------------------+------------------
// 개념 분류   | 자료 구조(구현체)  | 자료 구조(구현체)  | 자료 구조(구현체)
// 내부 형태   | 완전 이진 트리     | 비균형 이진 트리   | 다항 트리 (N진)
//             | (배열 기반)        | (자가 조절 이진트리| (Child-Sibling)
// ------------+--------------------+--------------------+------------------
// 최상위 확인 | O(1)               | O(1)               | O(1)
// 삽입(Push)  | O(log N)           | O(log N) [상환]    | O(1)
// 삭제(Pop)   | O(log N)           | O(log N) [상환]    | O(log N) [상환]
// 병합(Merge) | O(N)               | O(log N) [상환]    | O(1)
// ------------+--------------------+--------------------+------------------
// 메모리 소모 | 매우 적음 (배열)   | 적음 (포인터 2개)  | 보통 (포인터 3개)
// 실무 추천   | 단순 큐 작업       | 구현이 매우 단순한 | 최고 성능 우선순위 큐
//             |                    | 병합 가능 힙       | (다익스트라 등)
// ============+====================+====================+==================
//
// skew_heap은 랭크/높이 정보 없이 병합 시 좌우 자식을 무조건 교환(Swap)하여
// O(log N) 상환 복잡도로 자가 균형을 유지하는 이진 트리 기반 힙입니다.
//
// skew_heap<int> heap; // 기본: Max-Heap (std::less<int>)
// 
// // [1] 요소 삽입 (O(log N) 상환)
// heap.push(10);
// heap.push(5);
// heap.push(20);
// heap.push(15);
// heap.push(3);
// heap.push(7);
// 
// // [2] 상태 확인
// std::cout << "Size: " << heap.size() << std::endl;         // 6
// std::cout << "Top: " << heap.top().value() << std::endl;   // 20
// 
// // [3] 최상위 요소 제거 (O(log N) 상환)
// heap.pop();
// std::cout << "After pop: " << heap.top().value() << std::endl; // 15
// std::cout << "Size: " << heap.size() << std::endl;              // 5
// 
// // [4] 두 힙 병합 (O(log N) 상환)
// skew_heap<int> heap1, heap2;
// heap1.push(10);
// heap1.push(5);
// heap1.push(20);
// 
// heap2.push(30);
// heap2.push(8);
// heap2.push(12);
// 
// heap1.merge(heap2);
// std::cout << "After merge size: " << heap1.size() << std::endl;        // 6
// std::cout << "Merged heap top: " << heap1.top().value() << std::endl;  // 30
// 
// // [5] 초기화
// heap1.clear();
// std::cout << "Is empty: " << heap1.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class skew_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        struct node {
            value_type value;
            node* left = nullptr;
            node* right = nullptr;
            template <typename... Args>
            node(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        using handle_type = node*;

        skew_heap() : root_(nullptr), size_(0), comp_(Compare()) {}
        ~skew_heap() { clear(); }

        // 복사 방지 (이중 해제 방지)
        skew_heap(const skew_heap&) = delete;
        skew_heap& operator=(const skew_heap&) = delete;

        // 이동 시맨틱 지원
        skew_heap(skew_heap&& other) noexcept
            : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
            other.root_ = nullptr;
            other.size_ = 0;
        }

        skew_heap& operator=(skew_heap&& other) noexcept {
            if (this != &other) {
                clear();
                root_ = other.root_;
                size_ = other.size_;
                comp_ = std::move(other.comp_);
                other.root_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return root_->value;
        }

        template <typename... Args>
        handle_type emplace(Args&&... args) {
            handle_type new_node = new node(std::forward<Args>(args)...);
            root_ = merge_nodes(root_, new_node);
            ++size_;
            return new_node;
        }

        handle_type push(const value_type& value) { return emplace(value); }

        handle_type push(value_type&& value) {
            handle_type new_node = new node(std::move(value));
            root_ = merge_nodes(root_, new_node);
            ++size_;
            return new_node;
        }

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            node* old_root = root_;
            root_ = merge_nodes(root_->left, root_->right);
            delete old_root;
            --size_;
            return true;
        }

        void merge(skew_heap& other) {
            if (this == &other || other.empty()) return;
            root_ = merge_nodes(root_, other.root_);
            size_ += other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }

        void clear() noexcept {
            destroy_tree(root_);
            root_ = nullptr;
            size_ = 0;
        }

        // ==========================================
        // 순수 ASCII 기반 Binary Tree Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << ") ===\n";
            }
            else {
                std::cout << "=== Skew Heap Dump (Size: " << size_ << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Heap>\n\n";
                return;
            }

            std::cout << "[" << root_->value << "]\n";
            dump_children(root_, "");
            std::cout << "\n";
        }

    private:
        node* root_;
        size_type size_;
        Compare comp_;

        void dump_children(node* parent, const std::string& prefix) const {
            std::vector<std::pair<node*, std::string>> children;
            if (parent->left)  children.push_back({ parent->left, "L" });
            if (parent->right) children.push_back({ parent->right, "R" });

            for (size_t i = 0; i < children.size(); ++i) {
                bool is_last = (i == children.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "(" << children[i].second << ") [" << children[i].first->value << "]\n";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_children(children[i].first, next_prefix);
            }
        }

        node* merge_nodes(node* h1, node* h2) {
            if (!h1) return h2;
            if (!h2) return h1;

            if (comp_(h1->value, h2->value)) {
                std::swap(h1, h2);
            }

            node* temp = h1->right;
            h1->right = h1->left;
            h1->left = merge_nodes(temp, h2);

            return h1;
        }

        void destroy_tree(node* n) noexcept {
            if (!n) return;
            destroy_tree(n->left);
            destroy_tree(n->right);
            delete n;
        }
    };

} // namespace mino::core::container
