#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <string>
#include <vector>

// ============+====================+====================+==================
// 항목        |  우선순위 큐       |   페어링 힙        |  피보나치 힙
//             |  priority_queue    |  pairing_heap      |  fibonacci_heap
// ------------+--------------------+--------------------+------------------
// 개념 분류   | 추상 자료형(ADT)   | 자료 구조(구현체)  | 자료 구조(구현체)
// 기능의 정의 | 인터페이스 명세    | 실제 구현체        | 실제 구현체
// ------------+--------------------+--------------------+------------------
// 내부 형태   | 구현에 따라 다름   | N진 트리 집합      | 원형 이중 연결
// (메모리 구조| (주로 배열 이진 힙)| (Child-Sibling 체인| 리스트 트리 집합
//             |  구조로 구현)      |  포인터 트리 구조) |
// ------------+--------------------+--------------------+------------------
// 삽입(Push)  | O(log N)           | O(1)               |  O(1)
// ------------+--------------------+--------------------+------------------
// 삭제(Pop)   | O(log N)           | O(log N) [상환]    |  O(log N) [상환]
// ------------+--------------------+--------------------+------------------
// 값 갱신     | 지원 안 하거나 느림| O(log N) [상환]    |  O(1) [상환]
// (Update)    |                    | (실측 매우 빠름)   |
// ------------+--------------------+--------------------+------------------
// 병합(Merge) | O(N)               | O(1)               |  O(1)
// ------------+--------------------+--------------------+------------------
// 메모리 소모 | 매우 적음(배열)    | 보통 (포인터 3개)  |  매우 큼 (포인터 4개
//             |                    |                    |  + 차수 + 마킹)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 단순 큐 작업       | 대부분의 상황      |  이론적 특수 상황
//             |                    | (캐시 효율/구현 간결)| (다익스트라 등)
// ============+====================+====================+==================
//
// 페어링 힙(Pairing Heap)은 Left-Child / Next-Sibling 방식의 N진 트리 구조입니다.
//
// pairing_heap<int> heap; // 기본: Max-Heap (std::less<int>)
// 
// // [1] 요소 삽입 (O(1)): 새 노드를 기존 루트와 직접 병합(merge_nodes)
// auto h1 = heap.push(10);
// auto h2 = heap.push(5);
// auto h3 = heap.push(20);
// auto h4 = heap.push(15);
// auto h5 = heap.push(3);
// 
// // [2] 상태 확인
// std::cout << "Size: " << heap.size() << std::endl;        // 5
// std::cout << "Top: " << heap.top().value() << std::endl;  // 20
// 
// // [3] 최상위 요소 제거 (O(log N) 상환: Two-Pass 병합 알고리즘)
// heap.pop();
// std::cout << "After pop Top: " << heap.top().value() << std::endl; // 15
// std::cout << "After pop Size: " << heap.size() << std::endl;        // 4
// 
// // [4] emplace를 이용한 직접 생성 (O(1))
// auto h6 = heap.emplace(25);
// 
// // [5] 두 힙 병합 (O(1))
// pairing_heap<int> heap2;
// heap2.push(30);
// heap2.push(8);
// heap2.push(12);
// heap.merge(heap2);
// std::cout << "After merge size: " << heap.size() << std::endl;        // 8
// std::cout << "After merge top: " << heap.top().value() << std::endl;  // 30
// 
// // [6] 핸들을 통한 값 갱신 (Update)
// heap.update(h1, 50); // h1(10 -> 50으로 증가)
// std::cout << "After update h1 to 50: " << heap.top().value() << std::endl; // 50
// 
// // [7] 특정 핸들 노드 삭제 (Erase)
// heap.erase(h4);
// std::cout << "After erase h4(15), size: " << heap.size() << std::endl; // 7
// 
// // [8] 초기화
// heap.clear();
// std::cout << "After clear, is empty: " << heap.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class pairing_heap {
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

        // 복사 방지 (이중 해제 방지)
        pairing_heap(const pairing_heap&) = delete;
        pairing_heap& operator=(const pairing_heap&) = delete;

        // 이동 시맨틱 지원
        pairing_heap(pairing_heap&& other) noexcept
            : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
            other.root_ = nullptr;
            other.size_ = 0;
        }

        pairing_heap& operator=(pairing_heap&& other) noexcept {
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

        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            handle_type old_root = root_;
            root_ = merge_children(root_->child);
            delete old_root;
            --size_;
            return true;
        }

        void merge(pairing_heap& other) {
            if (this == &other || other.empty()) return;
            root_ = merge_nodes(root_, other.root_);
            size_ += other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }

        void update(handle_type handle, const value_type& new_value) {
            if (!handle) return;
            handle->value = new_value;
            update(handle);
        }

        void update(handle_type handle) {
            if (!handle) return;
            if (handle == root_) {
                handle_type ch = handle->child;
                handle->child = nullptr;
                root_ = merge_nodes(handle, merge_children(ch));
                return;
            }
            detach_node(handle);
            handle_type ch = handle->child;
            handle->child = nullptr;
            root_ = merge_nodes(root_, merge_nodes(handle, merge_children(ch)));
        }

        void erase(handle_type handle) {
            if (!handle) return;
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

        // ==========================================
        // 순수 ASCII 기반 Tree Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size_ << ") ===\n";
            }
            else {
                std::cout << "=== Pairing Heap Dump (Size: " << size_ << ") ===\n";
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
            std::vector<node*> children;
            for (node* curr = parent->child; curr != nullptr; curr = curr->next) {
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

        node* merge_nodes(node* n1, node* n2) {
            if (!n1) return n2;
            if (!n2) return n1;
            if (n1 == n2) return n1;

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
