#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::multiset     |  std::unordered_   |  flat_multiset
//             |                    |  multiset          |
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
// 실무 추천   | 잦은 삽입/삭제가   | 대용량 데이터의    | 중복 값 범위 탐색 및
//             | 발생하는 경우      | 빠른 단순 값 조회  | 순회가 압도적으로 많은 경우
// ============+====================+====================+==================
//
// flat_multiset은 std::vector를 내부 저장소로 사용하여 항상 정렬된 상태를
// 유지하며, 중복 값(Duplicate Values)을 허용하는 연속 메모리 연관 컨테이너입니다.
//
// flat_multiset<int> multiset;
// 
// // [1] 데이터 삽입 (O(N) - upper_bound 위치에 삽입하여 삽입 순서 유지)
// multiset.insert(10); // {10}
// multiset.insert(5);  // {5, 10}
// multiset.insert(10); // {5, 10, 10}
// multiset.insert(3);  // {3, 5, 10, 10}
// multiset.insert(5);  // {3, 5, 5, 10, 10}
// multiset.insert(15); // {3, 5, 5, 10, 10, 15}
// 
// // [2] 상태 확인
// std::cout << "Size: " << multiset.size() << std::endl; // 6
// 
// // [3] count를 이용한 특정 값의 개수 확인 (O(log N + K))
// std::cout << "Count of 5: " << multiset.count(5) << std::endl;   // 2
// std::cout << "Count of 10: " << multiset.count(10) << std::endl; // 2
// 
// // [4] find 및 contains를 이용한 검색 (O(log N))
// auto it = multiset.find(10);
// if (it != multiset.end()) {
//     std::cout << "Found: " << *it << std::endl; // 10
// }
// std::cout << "Contains 5: " << multiset.contains(5) << std::endl;   // 1 (true)
// std::cout << "Contains 20: " << multiset.contains(20) << std::endl; // 0 (false)
// 
// // [5] equal_range를 이용한 동일 값 범위 순회
// auto range = multiset.equal_range(5);
// for (auto iter = range.first; iter != range.second; ++iter) {
//     std::cout << *iter << " "; // 5 5
// }
// std::cout << std::endl;
// 
// // [6] 값 기준 일괄 삭제 (O(N))
// size_t erased = multiset.erase(5); // 값 5인 요소 모두 삭제
// std::cout << "Erased: " << erased << std::endl;          // 2
// std::cout << "Size after erase: " << multiset.size() << std::endl; // 4
// 
// // [7] 초기화
// multiset.clear();
// std::cout << "Is empty: " << multiset.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
    class flat_multiset {
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
        // 생성자 (Constructors)
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
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] size_type max_size() const noexcept { return data_.max_size(); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }
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
        [[nodiscard]] size_type count(const key_type& key) const {
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

        [[nodiscard]] bool contains(const key_type& key) const {
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

        // ==========================================
        // 순수 ASCII 기반 Flat Multiset Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Flat Multiset Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Flat Multiset>\n\n";
                return;
            }

            for (size_type i = 0; i < data_.size(); ++i) {
                bool is_last = (i == data_.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << i << "] : " << data_[i] << "\n";
            }
            std::cout << "\n";
        }
    };

} // namespace mino::core::container
