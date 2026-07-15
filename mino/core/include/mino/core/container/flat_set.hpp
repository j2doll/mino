#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <memory>

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>, typename Allocator = std::allocator<T>>
    class  flat_set {
    public:
        // 타입 정의 (Type Aliases)
        using key_type = T;
        using value_type = T;
        using key_compare = Compare;
        using value_compare = Compare;
        using allocator_type = Allocator;

        using container_type = std::vector<T, Allocator>;
        using reference = typename container_type::reference;
        using const_reference = typename container_type::const_reference;
        using iterator = typename container_type::const_iterator;
        using const_iterator = typename container_type::const_iterator;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;

    private:
        container_type m_data;
        key_compare    m_comp;

    public:
        // 생성자 (Constructors)
        flat_set() = default;
        explicit flat_set(const key_compare& comp, const allocator_type& alloc = allocator_type())
            : m_data(alloc), m_comp(comp) {
        }

        flat_set(std::initializer_list<value_type> init, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
            : m_data(alloc), m_comp(comp) {
            for (const auto& item : init) {
                insert(item);
            }
        }

        // 반복자 (Iterators)
        const_iterator begin()  const noexcept { return m_data.cbegin(); }
        const_iterator end()    const noexcept { return m_data.cend(); }
        const_iterator cbegin() const noexcept { return m_data.cbegin(); }
        const_iterator cend()   const noexcept { return m_data.cend(); }

        // 용량 (Capacity)
        bool      empty()    const noexcept { return m_data.empty(); }
        size_type size()     const noexcept { return m_data.size(); }
        size_type max_size() const noexcept { return m_data.max_size(); }
        void      reserve(size_type new_cap) { m_data.reserve(new_cap); }
        void      shrink_to_fit() { m_data.shrink_to_fit(); }

        // 수정자 (Modifiers)
        void clear() noexcept { m_data.clear(); }

        // 삽입 (Insert)
        std::pair<iterator, bool> insert(const value_type& value) {
            auto it = std::lower_bound(m_data.begin(), m_data.end(), value, m_comp);
            if (it != m_data.end() && !m_comp(value, *it)) {
                return { it, false };
            }
            auto inserted_it = m_data.insert(it, value);
            return { inserted_it, true };
        }

        std::pair<iterator, bool> insert(value_type&& value) {
            auto it = std::lower_bound(m_data.begin(), m_data.end(), value, m_comp);
            if (it != m_data.end() && !m_comp(value, *it)) {
                return { it, false };
            }
            auto inserted_it = m_data.insert(it, std::move(value));
            return { inserted_it, true };
        }

        // 조건부 삽입 (Emplace)
        template <typename... Args>
        std::pair<iterator, bool> emplace(Args&&... args) {
            value_type tmp(std::forward<Args>(args)...);
            return insert(std::move(tmp));
        }

        // 삭제 (Erase)
        iterator erase(const_iterator pos) {
            return m_data.erase(pos);
        }

        size_type erase(const key_type& key) {
            auto it = find(key);
            if (it != end()) {
                m_data.erase(it);
                return 1;
            }
            return 0;
        }

        // 검색 (Lookup)
        const_iterator find(const key_type& key) const {
            auto it = lower_bound(key);
            if (it != end() && !m_comp(key, *it)) {
                return it;
            }
            return end();
        }

        size_type count(const key_type& key) const {
            return find(key) != end() ? 1 : 0;
        }

        bool contains(const key_type& key) const {
            return find(key) != end();
        }

        const_iterator lower_bound(const key_type& key) const {
            return std::lower_bound(m_data.begin(), m_data.end(), key, m_comp);
        }

        const_iterator upper_bound(const key_type& key) const {
            return std::upper_bound(m_data.begin(), m_data.end(), key, m_comp);
        }

        std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
            return std::equal_range(m_data.begin(), m_data.end(), key, m_comp);
        }
    };

} // namespace mino::core::container 
