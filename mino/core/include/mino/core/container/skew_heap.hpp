#pragma once

#include <functional>
#include <utility>
#include <stdexcept>

// 스큐 힙(Skew Heap)은 이진 트리 구조를 가진 자가 조절(Self-adjusting) 힙 자료구조입니다.
// 레프트이스트 힙(Leftist Heap)과 기능적으로 매우 유사하지만,
// 트리 구조의 균형을 맞추기 위해 복잡한 조건(rank 등)을 유지할 필요가 없어
// 구현이 매우 간단하다는 강력한 장점을 가지고 있습니다.
// 마치 이진 탐색 트리에서 스플레이 트리(Splay Tree)가 가진 포지션과 비슷합니다.

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class  skew_heap {
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

        skew_heap() : root_(nullptr), size_(0), comp_(Compare()) {}
        ~skew_heap() { clear(); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        const value_type& top() const {
            if (empty()) throw std::runtime_error("Heap is empty");
            return root_->value;
        }

        template <typename... Args>
        node* emplace(Args&&... args) {
            node* new_node = new node(std::forward<Args>(args)...);
            root_ = merge_nodes(root_, new_node);
            ++size_;
            return new_node;
        }

        node* push(const value_type& value) { return emplace(value); }

        void pop() {
            if (empty()) throw std::runtime_error("Heap is empty");
            node* old_root = root_;
            root_ = merge_nodes(root_->left, root_->right);
            delete old_root;
            --size_;
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

    private:
        node* root_;
        size_type size_;
        Compare comp_;

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
