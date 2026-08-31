#pragma once

#include <vector>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::map          |  std::unordered_map|  flat_map
// ------------+--------------------+--------------------+------------------
// 내부 구조   | Red-Black Tree     | Hash Table (버킷)  | 정렬된 연속 배열 (Vector)
// 메모리 할당 | 노드당 개별 동적할당| 노드/버킷 동적할당 | 단일 연속 메모리 블록
// 캐시 효율성 | 낮음 (포인터 추적) | 보통 (해시 체이닝) | 최상 (연속 메모리 순회)
// ------------+--------------------+--------------------+------------------
// 탐색(Find)  | O(log N)           | O(1) [평균]        | O(log N) (이진 탐색)
// 삽입(Insert)| O(log N)           | O(1) [평균]        | O(N) (배열 요소 시프트)
// 삭제(Erase) | O(log N)           | O(1) [평균]        | O(N) (배열 요소 시프트)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 잦은 삽입/삭제가   | 대용량 데이터의    | 탐색과 순회가 압도적으로
//             | 발생하는 경우      | 빠른 단순 키 조회  | 많고 메모리가 제한된 경우
// ============+====================+====================+==================
//
// flat_map은 std::vector를 내부 저장소로 사용하여 항상 Key 기준으로
// 정렬된 상태를 유지하는 고성능 연속 메모리 연관 컨테이너입니다.
//
// flat_map<int, std::string> map;
// 
// // [1] 데이터 삽입 (O(N) - 정렬 위치 이진 탐색 후 삽입)
// map.insert({ 1, "Alice" });
// map.insert({ 2, "Bob" });
// map.insert({ 3, "Charlie" });
// 
// // [2] operator[]를 이용한 접근 및 삽입
// map[4] = "David";
// std::cout << "map[1]: " << map[1] << std::endl; // Alice
// 
// // [3] at()을 이용한 안전한 조회 (std::optional 반환)
// if (auto val = map.at(2)) {
//     std::cout << "map.at(2): " << val.value() << std::endl; // Bob
// }
// 
// // [4] find를 이용한 이진 탐색 (O(log N))
// auto it = map.find(3);
// if (it != map.end()) {
//     std::cout << "Found: " << it->second << std::endl; // Charlie
// }
// 
// // [5] contains를 이용한 존재 여부 확인
// std::cout << "Contains 2: " << map.contains(2) << std::endl;   // 1 (true)
// std::cout << "Contains 10: " << map.contains(10) << std::endl; // 0 (false)
// 
// // [6] 범위 기반 for문 순회 (연속 메모리로 최고 속도)
// for (const auto& [key, value] : map) {
//     std::cout << key << " => " << value << std::endl;
// }
// 
// // [7] 상태 확인 및 삭제
// std::cout << "Size: " << map.size() << std::endl; // 4
// map.erase(2);                                     // Key 2 삭제
// std::cout << "After erase(2), size: " << map.size() << std::endl; // 3
// 
// // [8] 초기화
// map.clear();
// std::cout << "Is empty: " << map.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename Key, typename T, typename Compare = std::less<Key>>
    class flat_map {
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
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        void clear() noexcept { data_.clear(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }

        // 원소 접근 (Element Access)
        [[nodiscard]] std::optional<mapped_type> at(const key_type& key) const noexcept {
            auto it = find(key);
            if (it == end()) return std::nullopt;
            return it->second;
        }

        [[nodiscard]] std::optional<mapped_type> at(const key_type& key) noexcept {
            auto it = find(key);
            if (it == end()) return std::nullopt;
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

        [[nodiscard]] bool contains(const key_type& key) const {
            return find(key) != end();
        }

        // ==========================================
        // 순수 ASCII 기반 Flat Map Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Flat Map Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Flat Map>\n\n";
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

