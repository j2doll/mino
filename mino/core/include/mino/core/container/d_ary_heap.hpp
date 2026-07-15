#pragma once

#include <vector>
#include <stdexcept>
#include <utility>
#include <algorithm>

// D-ary 힙(d-ary heap)은 이진 힙(Binary heap)을 일반화한 트리 기반의 자료구조입니다.
// 이진 힙은 모든 내부 노드가 최대 2개의 자식 노드를 가질 수 있는 반면,
// d-ary 힙은 각 노드가 최대 $d$개의 자식 노드를 가질 수 있습니다.

namespace mino::core::container { 

    template <typename T, std::size_t D = 4, typename Compare = std::less<T>>
    class d_ary_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        d_ary_heap() : comp_(Compare()) {}
        explicit d_ary_heap(const Compare& comp) : comp_(comp) {}

        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }

        const T& top() const {
            if (empty()) throw std::runtime_error("Heap is empty");
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

        void pop() {
            if (empty()) throw std::runtime_error("Heap is empty");
            if (data_.size() == 1) {
                data_.pop_back();
                return;
            }
            data_.front() = std::move(data_.back());
            data_.pop_back();
            sift_down(0);
        }

        void clear() noexcept {
            data_.clear();
        }

    private:
        std::vector<T> data_;
        Compare comp_;

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
            size_type size = data_.size();
            while (true) {
                size_type best = index;
                size_type first_child = index * D + 1;

                for (size_type i = 0; i < D; ++i) {
                    size_type child = first_child + i;
                    if (child < size && comp_(data_[best], data_[child])) {
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
