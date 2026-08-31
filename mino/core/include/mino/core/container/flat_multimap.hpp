#pragma once

#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::multimap     |  std::unordered_   |  flat_multimap
//             |                    |  multimap          |
// ------------+--------------------+--------------------+------------------
// 내부 구조   | Red-Black Tree     | Hash Table (버킷)  | 정렬된 연속 배열 (Vector)
// 메모리 할당 | 노드당 개별 동적할당| 노드/버킷 동적할당 | 단일 연속 메모리 블록
// 캐시 효율성 | 낮음 (포인터 추적) | 보통 (해시 체이닝) | 최상 (연속 메모리 순회)
// ------------+--------------------+--------------------+------------------
// 탐색(Find)  | O(log N)           | O(1) [평균]        | O(log N) (이진 탐색)
// 범위(Range) | O(log N + K)       | O(N)               | O(log N + K) (연속 슬라이스)
// 삽입(Insert)| O(log N)           | O(1) [평균]        | O(N) (배열 요소 시프트)
// 삭제(Erase) | O(log N + K)       | O(K) [평균]        | O(N) (배열 요소 시프트)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 잦은 삽입/삭제가   | 대용량 데이터의    | 중복 키 범위 탐색 및
//             | 발생하는 경우      | 빠른 단순 키 조회  | 순회가 압도적으로 많은 경우
// ============+====================+====================+==================
//
// flat_multimap은 std::vector를 내부 저장소로 사용하여 항상 Key 기준으로
// 정렬된 상태를 유지하며, 중복 키(Duplicate Keys)를 허용하는 연관 컨테이너입니다.
//
// flat_multimap<int, std::string> multimap;
// 
// // [1] 데이터 삽입 (O(N) - upper_bound 위치에 삽입하여 삽입 순서 유지)
// multimap.insert({ 1, "Alice" });
// multimap.insert({ 1, "Alpha" });
// multimap.insert({ 2, "Bob" });
// multimap.insert({ 2, "Beta" });
// multimap.insert({ 3, "Charlie" });
// 
// // [2] count를 이용한 특정 키의 개수 확인 (O(log N + K))
// std::cout << "Count of key 1: " << multimap.count(1) << std::endl; // 2
// std::cout << "Count of key 2: " << multimap.count(2) << std::endl; // 2
// 
// // [3] find를 이용한 첫 번째 요소 검색 (O(log N))
// auto it = multimap.find(1);
// if (it != multimap.end()) {
//     std::cout << "Found key 1: " << it->second << std::endl; // Alice
// }
// 
// // [4] equal_range를 이용한 동일 키 범위 순회 (O(log N))
// auto range = multimap.equal_range(2);
// for (auto it = range.first; it != range.second; ++it) {
//     std::cout << "  " << it->second << std::endl; // Bob, Beta
// }
// 
// // [5] 범위 기반 for문 순회 (연속 메모리 순회로 캐시 최적화)
// for (const auto& [key, value] : multimap) {
//     std::cout << key << " => " << value << std::endl;
// }
// 
// // [6] 특정 키의 모든 요소 삭제 (O(N))
// size_t erased = multimap.erase(1); // Key 1을 가진 모든 요소 삭제
// std::cout << "Erased " << erased << " elements with key 1" << std::endl; // 2
// std::cout << "Size after erase: " << multimap.size() << std::endl;        // 3
// 
// // [7] 초기화
// multimap.clear();
// std::cout << "After clear, is empty: " << multimap.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename Key, typename T, typename Compare = std::less<Key>>
    class flat_multimap {
    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<key_type, mapped_type>;
        using key_compare = Compare;

        using container_type = std::vector<value_type>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;

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

        // 반복자 (Iterators)
        iterator begin() noexcept { return data_.begin(); }
        const_iterator begin() const noexcept { return data_.begin(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }

        iterator end() noexcept { return data_.end(); }
        const_iterator end() const noexcept { return data_.end(); }
        const_iterator cend() const noexcept { return data_.cend(); }

        // 용량 (Capacity)
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }
        void clear() noexcept { data_.clear(); }

        // 삽입 (Modifiers)
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

        // 삭제 (Erase)
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

        // 탐색 (Lookup)
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

        [[nodiscard]] bool contains(const key_type& key) const {
            return find(key) != end();
        }

        [[nodiscard]] size_type count(const key_type& key) const {
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

        // ==========================================
        // 순수 ASCII 기반 Flat Multimap Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Flat Multimap Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Flat Multimap>\n\n";
                return;
            }

            for (size_type i = 0; i < data_.size(); ++i) {
                bool is_last = (i == data_.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << data_[i].first << "] => " << data_[i].second << "\n";
            }
            std::cout << "\n";
        }
    };

} // namespace mino::core::container
