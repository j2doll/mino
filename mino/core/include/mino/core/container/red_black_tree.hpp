#pragma once

// ============================================================================
// Red-Black Tree(레드-블랙 트리) 구현체입니다.
//
// 1. 센티넬 노드(NIL Sentinel)를 활용하여 경계 조건 및 트리 균형 유지(회전/Fixup).
// 2. 경로 기반 위치 탐색(std::string_view "L", "R", "LR" 등) 및 색상 판별(RED/BLACK).
// 3. 순수 ASCII 기반 트리 구조 시각화 출력 (print_tree_structure).
// 4. 트리 간 병합(merge) 및 이동 시맨틱(Move semantics) 지원.
//
// 사용 예제:
// ----------------------------------------------------------------------------
// #include <iostream>
// #include "red_black_tree.hpp"
//
// int main() {
//     using namespace mino::core::container;
//
//     red_black_tree<int> rbt;
//
//     // 1. 노드 삽입
//     for (int v : {20, 10, 30, 5, 15}) {
//         rbt.insert(v);
//     }
//
//     // 2. 트리 ASCII 구조 출력
//     rbt.print_tree_structure();
//
//     // 3. 루트 및 특정 경로 노드 확인
//     const auto* root = rbt.get_root();
//     if (root) {
//         std::cout << "Root: " << root->key 
//                   << " (is_black: " << rbt.is_black(root) << ")\n";
//     }
//
//     const auto* left_child = rbt.get_node_at_path("L");
//     if (!rbt.is_nil(left_child)) {
//         std::cout << "Left Child: " << left_child->key 
//                   << " (is_red: " << rbt.is_red(left_child) << ")\n";
//     }
//
//     // 4. 트리 병합
//     red_black_tree<int> other;
//     other.insert(25);
//     rbt.merge(other);
//
//     return 0;
// }
// ============================================================================

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mino::core::container {

    enum class node_color { red, black };

    template <typename Key, typename Compare = std::less<Key>>
    class red_black_tree {
    public:
        struct node {
            Key key;
            node_color color{ node_color::red };
            node* parent{ nullptr };
            node* left{ nullptr };
            node* right{ nullptr };

            explicit node(const Key& k) : key(k) {}
        };

    private:
        node* root_node{ nullptr };
        node* nil_node{ nullptr }; // 센티넬(NIL/Leaf) 노드
        Compare comp;
        std::size_t element_count{ 0 };

        void left_rotate(node* x) {
            node* y = x->right;
            x->right = y->left;
            if (y->left != nil_node) {
                y->left->parent = x;
            }
            y->parent = x->parent;
            if (x->parent == nil_node) {
                root_node = y;
            }
            else if (x == x->parent->left) {
                x->parent->left = y;
            }
            else {
                x->parent->right = y;
            }
            y->left = x;
            x->parent = y;
        }

        void right_rotate(node* y) {
            node* x = y->left;
            y->left = x->right;
            if (x->right != nil_node) {
                x->right->parent = y;
            }
            x->parent = y->parent;
            if (y->parent == nil_node) {
                root_node = x;
            }
            else if (y == y->parent->right) {
                y->parent->right = x;
            }
            else {
                y->parent->left = x;
            }
            x->right = y;
            y->parent = x;
        }

        void insert_fixup(node* z) {
            while (z->parent->color == node_color::red) {
                if (z->parent == z->parent->parent->left) {
                    node* y = z->parent->parent->right; // Uncle
                    if (y->color == node_color::red) {
                        // Case 1: 삼촌이 RED
                        z->parent->color = node_color::black;
                        y->color = node_color::black;
                        z->parent->parent->color = node_color::red;
                        z = z->parent->parent;
                    }
                    else {
                        if (z == z->parent->right) {
                            // Case 2: 삼촌이 BLACK & z가 오른쪽 자식 (꺾인 형태)
                            z = z->parent;
                            left_rotate(z);
                        }
                        // Case 3: 삼촌이 BLACK & z가 왼쪽 자식 (직선 형태)
                        z->parent->color = node_color::black;
                        z->parent->parent->color = node_color::red;
                        right_rotate(z->parent->parent);
                    }
                }
                else {
                    node* y = z->parent->parent->left; // Uncle
                    if (y->color == node_color::red) {
                        // Case 1: 삼촌이 RED
                        z->parent->color = node_color::black;
                        y->color = node_color::black;
                        z->parent->parent->color = node_color::red;
                        z = z->parent->parent;
                    }
                    else {
                        if (z == z->parent->left) {
                            // Case 2: 삼촌이 BLACK & z가 왼쪽 자식
                            z = z->parent;
                            right_rotate(z);
                        }
                        // Case 3: 삼촌이 BLACK & z가 오른쪽 자식
                        z->parent->color = node_color::black;
                        z->parent->parent->color = node_color::red;
                        left_rotate(z->parent->parent);
                    }
                }
            }
            root_node->color = node_color::black;
        }

        void destroy_tree(node* current) {
            if (current != nil_node && current != nullptr) {
                destroy_tree(current->left);
                destroy_tree(current->right);
                delete current;
            }
        }

        void print_subtree_ascii(const node* current, const std::string& prefix, bool is_left) const {
            if (current != nil_node) {
                std::cout << prefix;
                std::cout << (is_left ? "|-- [L] " : "\\-- [R] ");
                std::cout << current->key << " ("
                    << (current->color == node_color::red ? "RED" : "BLACK") << ")\n";
                print_subtree_ascii(current->left, prefix + (is_left ? "|   " : "    "), true);
                print_subtree_ascii(current->right, prefix + (is_left ? "|   " : "    "), false);
            }
        }

    public:
        red_black_tree() {
            nil_node = new node(Key{});
            nil_node->color = node_color::black;
            nil_node->left = nil_node;
            nil_node->right = nil_node;
            nil_node->parent = nil_node;
            root_node = nil_node;
        }

        ~red_black_tree() {
            destroy_tree(root_node);
            delete nil_node;
        }

        red_black_tree(const red_black_tree&) = delete;
        red_black_tree& operator=(const red_black_tree&) = delete;

        red_black_tree(red_black_tree&& other) noexcept
            : root_node(other.root_node), nil_node(other.nil_node), element_count(other.element_count) {
            other.root_node = nullptr;
            other.nil_node = nullptr;
            other.element_count = 0;
        }

        red_black_tree& operator=(red_black_tree&& other) noexcept {
            if (this != &other) {
                destroy_tree(root_node);
                delete nil_node;

                root_node = other.root_node;
                nil_node = other.nil_node;
                element_count = other.element_count;

                other.root_node = nullptr;
                other.nil_node = nullptr;
                other.element_count = 0;
            }
            return *this;
        }

        bool insert(const Key& key) {
            node* y = nil_node;
            node* x = root_node;

            while (x != nil_node) {
                y = x;
                if (comp(key, x->key)) {
                    x = x->left;
                }
                else if (comp(x->key, key)) {
                    x = x->right;
                }
                else {
                    return false; // 중복 삽입 방지
                }
            }

            node* z = new node(key);
            z->parent = y;
            z->left = nil_node;
            z->right = nil_node;
            z->color = node_color::red;

            if (y == nil_node) {
                root_node = z;
            }
            else if (comp(z->key, y->key)) {
                y->left = z;
            }
            else {
                y->right = z;
            }

            insert_fixup(z);
            ++element_count;
            return true;
        }

        void merge(const red_black_tree& other) {
            if (this == &other || other.empty()) return;

            auto traverse = [this, &other](auto& self, const node* cur) -> void {
                if (cur == other.get_nil() || cur == nullptr) return;
                self(self, cur->left);
                this->insert(cur->key);
                self(self, cur->right);
                };

            traverse(traverse, other.get_root());
        }

        // 노드 접근 및 탐색
        [[nodiscard]] const node* get_root() const noexcept {
            return (root_node == nil_node) ? nullptr : root_node;
        }

        [[nodiscard]] const node* get_nil() const noexcept {
            return nil_node;
        }

        [[nodiscard]] const node* get_node_at_path(std::string_view path) const {
            const node* current = root_node;
            for (char dir : path) {
                if (is_nil(current)) return nullptr;
                if (dir == 'L' || dir == 'l') {
                    current = current->left;
                }
                else if (dir == 'R' || dir == 'r') {
                    current = current->right;
                }
                else {
                    return nullptr;
                }
            }
            return is_nil(current) ? nullptr : current;
        }

        [[nodiscard]] const node* find_node(const Key& key) const {
            const node* current = root_node;
            while (!is_nil(current)) {
                if (comp(key, current->key)) {
                    current = current->left;
                }
                else if (comp(current->key, key)) {
                    current = current->right;
                }
                else {
                    return current;
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool contains(const Key& key) const {
            return find_node(key) != nullptr;
        }

        // 상태 및 색상 판별
        [[nodiscard]] bool is_nil(const node* target) const noexcept {
            return target == nullptr || target == nil_node;
        }

        [[nodiscard]] bool is_red(const node* target) const noexcept {
            return !is_nil(target) && (target->color == node_color::red);
        }

        [[nodiscard]] bool is_black(const node* target) const noexcept {
            return is_nil(target) || (target->color == node_color::black);
        }

        // 트리 정보 및 출력
        [[nodiscard]] std::size_t size() const noexcept { return element_count; }
        [[nodiscard]] bool empty() const noexcept { return element_count == 0; }

        void print_tree_structure() const {
            if (root_node == nil_node) {
                std::cout << "(Empty Tree)\n";
                return;
            }
            std::cout << "[ROOT] " << root_node->key << " ("
                << (root_node->color == node_color::red ? "RED" : "BLACK") << ")\n";
            print_subtree_ascii(root_node->left, " ", true);
            print_subtree_ascii(root_node->right, " ", false);
        }
    };

} // namespace mino::core::container
