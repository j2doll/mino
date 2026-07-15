#pragma once

#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <initializer_list>
  
namespace mino::core::container {

    template <typename Key, typename T, typename Compare = std::less<Key>>
    class  flat_multimap {
    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<key_type, mapped_type>;
        using key_compare = Compare;

        using container_type = std::vector<value_type>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using size_type = typename container_type::size_type;

        // 구조체 비교를 위한 내부 컴포레이터
        struct value_compare {
            key_compare comp;
            bool operator()(const value_type& lhs, const value_type& rhs) const {
                return comp(lhs.first, rhs.first);
            }
            bool operator()(const value_type& lhs, const key_type& rhs) const {
                return comp(lhs.first, rhs);
            }
            bool operator()(const key_type& lhs, const value_type& rhs) const {
                return comp(lhs, rhs.first);
            }
        };

    private:
        container_type data_;
        key_compare comp_;
        value_compare val_comp_{ comp_ };

    public:
        // 생성자 함수군
        flat_multimap() = default;
        explicit flat_multimap(const key_compare& comp) : comp_(comp), val_comp_{ comp } {}

        template <typename InputIt>
        flat_multimap(InputIt first, InputIt last, const key_compare& comp = key_compare())
            : comp_(comp), val_comp_{ comp } {
            insert(first, last);
        }

        flat_multimap(std::initializer_list<value_type> init, const key_compare& comp = key_compare())
            : comp_(comp), val_comp_{ comp } {
            insert(init.begin(), init.end());
        }

        // 반복자 관련 함수
        iterator begin() noexcept { return data_.begin(); }
        const_iterator begin() const noexcept { return data_.begin(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }

        iterator end() noexcept { return data_.end(); }
        const_iterator end() const noexcept { return data_.end(); }
        const_iterator cend() const noexcept { return data_.cend(); }

        // 용량 관련 함수
        bool empty() const noexcept { return data_.empty(); }
        size_type size() const noexcept { return data_.size(); }
        size_type capacity() const noexcept { return data_.capacity(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }
        void clear() noexcept { data_.clear(); }

        // 삽입 함수 (multimap이므로 중복 키 허용, 항상 upper_bound 위치에 삽입하여 안정성 유지)
        iterator insert(const value_type& value) {
            auto it = std::upper_bound(data_.begin(), data_.end(), value.first, val_comp_);
            return data_.insert(it, value);
        }

        iterator insert(value_type&& value) {
            auto it = std::upper_bound(data_.begin(), data_.end(), value.first, val_comp_);
            return data_.insert(it, std::move(value));
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last) {
            for (auto it = first; it != last; ++it) {
                insert(*it);
            }
        }

        void insert(std::initializer_list<value_type> ilist) {
            insert(ilist.begin(), ilist.end());
        }

        template <typename... Args>
        iterator emplace(Args&&... args) {
            value_type val(std::forward<Args>(args)...);
            return insert(std::move(val));
        }

        // 삭제 함수
        iterator erase(const_iterator pos) {
            return data_.erase(pos);
        }

        iterator erase(const_iterator first, const_iterator last) {
            return data_.erase(first, last);
        }

        size_type erase(const key_type& key) {
            auto range = equal_range(key);
            size_type count = std::distance(range.first, range.second);
            data_.erase(range.first, range.second);
            return count;
        }

        // 탐색 함수
        iterator find(const key_type& key) {
            auto it = lower_bound(key);
            if (it != end() && !comp_(key, it->first)) {
                return it;
            }
            return end();
        }

        const_iterator find(const key_type& key) const {
            auto it = lower_bound(key);
            if (it != end() && !comp_(key, it->first)) {
                return it;
            }
            return end();
        }

        size_type count(const key_type& key) const {
            auto range = equal_range(key);
            return std::distance(range.first, range.second);
        }

        iterator lower_bound(const key_type& key) {
            return std::lower_bound(data_.begin(), data_.end(), key, val_comp_);
        }

        const_iterator lower_bound(const key_type& key) const {
            return std::lower_bound(data_.begin(), data_.end(), key, val_comp_);
        }

        iterator upper_bound(const key_type& key) {
            return std::upper_bound(data_.begin(), data_.end(), key, val_comp_);
        }

        const_iterator upper_bound(const key_type& key) const {
            return std::upper_bound(data_.begin(), data_.end(), key, val_comp_);
        }

        std::pair<iterator, iterator> equal_range(const key_type& key) {
            return std::equal_range(data_.begin(), data_.end(), key, val_comp_);
        }

        std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
            return std::equal_range(data_.begin(), data_.end(), key, val_comp_);
        }
    };

} // namespace mino::core::container
