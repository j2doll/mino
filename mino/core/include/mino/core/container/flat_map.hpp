#pragma once

#include <vector>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include <stdexcept>

namespace mino::core::container {

    template <typename Key, typename T, typename Compare = std::less<Key>>
    class  flat_map {
    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<key_type, mapped_type>;
        using key_compare = Compare;

        using container_type = std::vector<value_type>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using size_type = typename container_type::size_type;

    private:
        container_type data_;
        key_compare comp_;

        struct value_compare {
            key_compare comp;
            bool operator()(const value_type& lhs, const value_type& rhs) const { return comp(lhs.first, rhs.first); }
            bool operator()(const value_type& lhs, const key_type& rhs) const { return comp(lhs.first, rhs); }
            bool operator()(const key_type& lhs, const value_type& rhs) const { return comp(lhs, rhs.first); }
        };

    public:
        flat_map() = default;
        explicit flat_map(const key_compare& comp) : comp_(comp) {}

        flat_map(std::initializer_list<value_type> init, const key_compare& comp = key_compare()) : comp_(comp) {
            for (const auto& item : init) {
                insert(item);
            }
        }

        // 반복자 (Iterators)
        iterator begin() noexcept { return data_.begin(); }
        iterator end() noexcept { return data_.end(); }
        const_iterator begin() const noexcept { return data_.begin(); }
        const_iterator end() const noexcept { return data_.end(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }
        const_iterator cend() const noexcept { return data_.cend(); }

        // 용량 (Capacity)
        bool empty() const noexcept { return data_.empty(); }
        size_type size() const noexcept { return data_.size(); }
        void clear() noexcept { data_.clear(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }

        // 원소 접근 (Element Access)
        mapped_type& at(const key_type& key) {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("flat_map::at: key not found");
            }
            return it->second;
        }

        const mapped_type& at(const key_type& key) const {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("flat_map::at: key not found");
            }
            return it->second;
        }

        mapped_type& operator[](const key_type& key) {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, value_compare{ comp_ });
            if (it == data_.end() || comp_(key, it->first)) {
                it = data_.insert(it, std::make_pair(key, mapped_type{}));
            }
            return it->second;
        }

        mapped_type& operator[](key_type&& key) {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, value_compare{ comp_ });
            if (it == data_.end() || comp_(key, it->first)) {
                it = data_.insert(it, std::make_pair(std::move(key), mapped_type{}));
            }
            return it->second;
        }

        // 수정자 (Modifiers)
        std::pair<iterator, bool> insert(const value_type& value) {
            auto it = std::lower_bound(data_.begin(), data_.end(), value.first, value_compare{ comp_ });
            if (it != data_.end() && !comp_(value.first, it->first)) {
                return { it, false };
            }
            return { data_.insert(it, value), true };
        }

        std::pair<iterator, bool> insert(value_type&& value) {
            auto it = std::lower_bound(data_.begin(), data_.end(), value.first, value_compare{ comp_ });
            if (it != data_.end() && !comp_(value.first, it->first)) {
                return { it, false };
            }
            return { data_.insert(it, std::move(value)), true };
        }

        template <typename... Args>
        std::pair<iterator, bool> emplace(Args&&... args) {
            value_type value(std::forward<Args>(args)...);
            return insert(std::move(value));
        }

        iterator erase(const_iterator pos) {
            return data_.erase(pos);
        }

        size_type erase(const key_type& key) {
            auto it = find(key);
            if (it != end()) {
                data_.erase(it);
                return 1;
            }
            return 0;
        }

        // 탐색 (Lookup)
        iterator find(const key_type& key) {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, value_compare{ comp_ });
            if (it != data_.end() && !comp_(key, it->first)) {
                return it;
            }
            return end();
        }

        const_iterator find(const key_type& key) const {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, value_compare{ comp_ });
            if (it != data_.end() && !comp_(key, it->first)) {
                return it;
            }
            return end();
        }

        bool contains(const key_type& key) const {
            return find(key) != end();
        }
    };

} 
