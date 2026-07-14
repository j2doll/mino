#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <vector>

/*
========================================================================
항목            우선순위 큐          이진 힙              피보나치 힙
               priority_queue        pairing_heap        fibonacci_heap
------------------------------------------------------------------------
개념 분류       추상 자료형(ADT)     자료 구조(구현체)    자료 구조(구현체)
기능의 정의          실제 구현체          실제 구현체

내부 형태       정의되지 않음        완전 이진 트리       원형 이중 연결
(배열,         (주로 배열로 구현)   리스트 트리 집합
리스트 등)

삽입(Push)      구현에 따라 다름     O(log N)             O(1)

삭제(Pop)       구현에 따라 다름     O(log N)             O(log N)[상환]

값 감소         지원 안 하거나 느림  O(log N)             O(1)[상환]

병합(Merge)     지원하지 않음        O(N)                 O(1)

메모리 소모     없음(구현체별 다름)  매우 적음            매우 큼
(인덱스 기반   (다중 포인터 사용)
 탐색)

실무 추천       인터페이스 명세      대부분의 상황        특수 알고리즘
                설계 시 사용         (캐시 효율 우수)     (다익스트라 등)
========================================================================
*/

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class  fibonacci_heap {
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

        const value_type& top() const {
            if (empty()) throw std::runtime_error("Heap is empty");
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

        void pop() {
            if (empty()) throw std::runtime_error("Heap is empty");
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

    private:
        node* max_node_;
        size_type size_;
        Compare comp_;

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
            size_type max_deg = static_cast<size_type>(std::log2(size_) + 2);
            std::vector<node*> degree_array(max_deg, nullptr);
            std::vector<node*> root_nodes;
            node* curr = max_node_;
            if (curr) {
                do { root_nodes.push_back(curr); curr = curr->right; } while (curr != max_node_);
            }
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
                if (d < degree_array.size()) degree_array[d] = x;
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
            y->left->right = y->right; y->right->left = y->left;
            y->parent = x; y->left = y; y->right = y;
            x->child = merge_lists(x->child, y);
            x->degree++;
            y->marked = false;
        }

        void destroy_list(node* start) noexcept {
            if (!start) return;
            node* curr = start;
            do {
                node* next = curr->right;
                if (curr->child) destroy_list(curr->child);
                delete curr;
                curr = next;
            } while (curr != start);
        }
    };

} 
