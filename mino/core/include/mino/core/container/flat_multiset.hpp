#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace mino::core::container {

    template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
    class  flat_multiset {
    public:
        // 타입 정의 (Types)
        using key_type = Key;
        using value_type = Key;
        using key_compare = Compare;
        using value_compare = Compare;
        using allocator_type = Allocator;
        using container_type = std::vector<Key, Allocator>;

        using pointer = typename container_type::pointer;
        using const_pointer = typename container_type::const_pointer;
        using reference = typename container_type::reference;
        using const_reference = typename container_type::const_reference;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;

        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using reverse_iterator = typename container_type::reverse_iterator;
        using const_reverse_iterator = typename container_type::const_reverse_iterator;

    private:
        container_type data_;
        key_compare comp_;

    public:
        // 생성자 및 대입 연산자 (Constructors & Assignment)
        flat_multiset() : data_(), comp_() {}
        explicit flat_multiset(const Compare& comp, const Allocator& alloc = Allocator()) : data_(alloc), comp_(comp) {}
        explicit flat_multiset(const Allocator& alloc) : data_(alloc), comp_() {}

        template <typename InputIt>
        flat_multiset(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : data_(first, last, alloc), comp_(comp) {
            std::sort(data_.begin(), data_.end(), comp_);
        }

        flat_multiset(std::initializer_list<value_type> init, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : data_(init, alloc), comp_(comp) {
            std::sort(data_.begin(), data_.end(), comp_);
        }

        // 반복자 (Iterators)
        iterator begin() noexcept { return data_.begin(); }
        const_iterator begin() const noexcept { return data_.begin(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }

        iterator end() noexcept { return data_.end(); }
        const_iterator end() const noexcept { return data_.end(); }
        const_iterator cend() const noexcept { return data_.cend(); }

        reverse_iterator rbegin() noexcept { return data_.rbegin(); }
        const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
        const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }

        reverse_iterator rend() noexcept { return data_.rend(); }
        const_reverse_iterator rend() const noexcept { return data_.rend(); }
        const_reverse_iterator crend() const noexcept { return data_.crend(); }

        // 용량 (Capacity)
        bool empty() const noexcept { return data_.empty(); }
        size_type size() const noexcept { return data_.size(); }
        size_type max_size() const noexcept { return data_.max_size(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }
        size_type capacity() const noexcept { return data_.capacity(); }
        void shrink_to_fit() { data_.shrink_to_fit(); }

        // 수정자 (Modifiers)
        void clear() noexcept { data_.clear(); }

        iterator insert(const value_type& value) {
            auto it = std::upper_bound(data_.begin(), data_.end(), value, comp_);
            return data_.insert(it, value);
        }

        iterator insert(value_type&& value) {
            auto it = std::upper_bound(data_.begin(), data_.end(), value, comp_);
            return data_.insert(it, std::move(value));
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last) {
            for (; first != last; ++first) {
                insert(*first);
            }
        }

        void insert(std::initializer_list<value_type> ilist) {
            insert(ilist.begin(), ilist.end());
        }

        template <typename... Args>
        iterator emplace(Args&&... args) {
            value_type elem(std::forward<Args>(args)...);
            auto it = std::upper_bound(data_.begin(), data_.end(), elem, comp_);
            return data_.insert(it, std::move(elem));
        }

        iterator erase(const_iterator pos) {
            return data_.erase(pos);
        }

        iterator erase(const_iterator first, const_iterator last) {
            return data_.erase(first, last);
        }

        size_type erase(const key_type& key) {
            auto range = std::equal_range(data_.begin(), data_.end(), key, comp_);
            size_type count = std::distance(range.first, range.second);
            data_.erase(range.first, range.second);
            return count;
        }

        void swap(flat_multiset& other) noexcept {
            using std::swap;
            swap(data_, other.data_);
            swap(comp_, other.comp_);
        }

        // 검색 (Lookup)
        size_type count(const key_type& key) const {
            auto range = std::equal_range(data_.begin(), data_.end(), key, comp_);
            return std::distance(range.first, range.second);
        }

        iterator find(const key_type& key) {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, comp_);
            if (it != data_.end() && !comp_(key, *it)) {
                return it;
            }
            return data_.end();
        }

        const_iterator find(const key_type& key) const {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, comp_);
            if (it != data_.end() && !comp_(key, *it)) {
                return it;
            }
            return data_.end();
        }

        bool contains(const key_type& key) const {
            return std::binary_search(data_.begin(), data_.end(), key, comp_);
        }

        std::pair<iterator, iterator> equal_range(const key_type& key) {
            return std::equal_range(data_.begin(), data_.end(), key, comp_);
        }

        std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
            return std::equal_range(data_.begin(), data_.end(), key, comp_);
        }

        iterator lower_bound(const key_type& key) {
            return std::lower_bound(data_.begin(), data_.end(), key, comp_);
        }

        const_iterator lower_bound(const key_type& key) const {
            return std::lower_bound(data_.begin(), data_.end(), key, comp_);
        }

        iterator upper_bound(const key_type& key) {
            return std::upper_bound(data_.begin(), data_.end(), key, comp_);
        }

        const_iterator upper_bound(const key_type& key) const {
            return std::upper_bound(data_.begin(), data_.end(), key, comp_);
        }

        // 옵저버 (Observers)
        key_compare key_comp() const { return comp_; }
        value_compare value_comp() const { return comp_; }
    };

} 
