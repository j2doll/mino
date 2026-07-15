#pragma once

#include <functional>
#include <utility>
#include <stdexcept>

// ========================================================================
// 항목            우선순위 큐          이진 힙              피보나치 힙
//                priority_queue        pairing_heap        fibonacci_heap
// ------------------------------------------------------------------------
// 개념 분류       추상 자료형(ADT)     자료 구조(구현체)    자료 구조(구현체)
// 기능의 정의          실제 구현체          실제 구현체
// 
// 내부 형태       정의되지 않음        완전 이진 트리       원형 이중 연결
// (배열,         (주로 배열로 구현)   리스트 트리 집합
// 리스트 등)
// 
// 삽입(Push)      구현에 따라 다름     O(log N)             O(1)
// 
// 삭제(Pop)       구현에 따라 다름     O(log N)             O(log N)[상환]
// 
// 값 감소         지원 안 하거나 느림  O(log N)             O(1)[상환]
// 
// 병합(Merge)     지원하지 않음        O(N)                 O(1)
// 
// 메모리 소모     없음(구현체별 다름)  매우 적음            매우 큼
// (인덱스 기반   (다중 포인터 사용)
//  탐색)
// 
// 실무 추천       인터페이스 명세      대부분의 상황        특수 알고리즘
//                 설계 시 사용         (캐시 효율 우수)     (다익스트라 등)
// ========================================================================

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class  pairing_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        struct node {
            value_type value;
            node* child = nullptr;
            node* next = nullptr;
            node* prev = nullptr;
            template <typename... Args>
            node(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        using handle_type = node*;

        pairing_heap() : root_(nullptr), size_(0), comp_(Compare()) {}
        ~pairing_heap() { clear(); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        const value_type& top() const {
            if (empty()) throw std::runtime_error("Heap is empty");
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

        void pop() {
            if (empty()) throw std::runtime_error("Heap is empty");
            handle_type old_root = root_;
            root_ = merge_children(root_->child);
            delete old_root;
            --size_;
        }

        void merge(pairing_heap& other) {
            if (this == &other || other.empty()) return;
            root_ = merge_nodes(root_, other.root_);
            size_ += other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }

        void update(handle_type handle, const value_type& new_value) {
            handle->value = new_value;
            update(handle);
        }

        void update(handle_type handle) {
            if (handle == root_) {
                handle_type ch = handle->child;
                handle->child = nullptr;
                root_ = merge_nodes(handle, merge_children(ch));
                return;
            }
            detach_node(handle);
            root_ = merge_nodes(root_, handle);
        }

        void erase(handle_type handle) {
            if (handle == root_) {
                pop();
                return;
            }
            detach_node(handle);
            handle_type children = handle->child;
            delete handle;
            --size_;
            root_ = merge_nodes(root_, merge_children(children));
        }

        void clear() noexcept {
            destroy_tree(root_);
            root_ = nullptr;
            size_ = 0;
        }

    private:
        node* root_;
        size_type size_;
        Compare comp_;

        node* merge_nodes(node* n1, node* n2) {
            if (!n1) return n2;
            if (!n2) return n1;
            if (comp_(n1->value, n2->value)) std::swap(n1, n2);
            n2->next = n1->child;
            if (n1->child) n1->child->prev = n2;
            n2->prev = n1;
            n1->child = n2;
            return n1;
        }

        node* merge_children(node* first_child) {
            if (!first_child) return nullptr;
            node* current = first_child;
            node* pairs_head = nullptr;
            node* pairs_tail = nullptr;

            while (current) {
                node* n1 = current;
                node* n2 = current->next;
                if (n2) {
                    current = n2->next;
                    n1->next = n1->prev = n2->next = n2->prev = nullptr;
                    node* merged = merge_nodes(n1, n2);
                    if (!pairs_head) { pairs_head = pairs_tail = merged; }
                    else { pairs_tail->next = merged; merged->prev = pairs_tail; pairs_tail = merged; }
                }
                else {
                    n1->next = n1->prev = nullptr;
                    if (!pairs_head) pairs_head = n1;
                    else { pairs_tail->next = n1; n1->prev = pairs_tail; }
                    break;
                }
            }

            node* last = pairs_head;
            while (last && last->next) last = last->next;
            node* result = last;
            if (result) {
                current = result->prev;
                while (current) {
                    node* prev_node = current->prev;
                    current->next = current->prev = result->next = result->prev = nullptr;
                    result = merge_nodes(current, result);
                    current = prev_node;
                }
            }
            return result;
        }

        void detach_node(node* n) {
            if (!n || n == root_) return;
            if (n->prev) {
                if (n->prev->child == n) n->prev->child = n->next;
                else n->prev->next = n->next;
            }
            if (n->next) n->next->prev = n->prev;
            n->next = n->prev = nullptr;
        }

        void destroy_tree(node* n) noexcept {
            if (!n) return;
            node* current = n->child;
            while (current) {
                node* next = current->next;
                destroy_tree(current);
                current = next;
            }
            delete n;
        }
    };

} // namespace mino::core::container
