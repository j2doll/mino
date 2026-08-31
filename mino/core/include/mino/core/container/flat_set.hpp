#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <memory>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::set          |  std::unordered_set|  flat_set
// ------------+--------------------+--------------------+------------------
// 내부 구조   | Red-Black Tree     | Hash Table (버킷)  | 정렬된 연속 배열 (Vector)
// 메모리 할당 | 노드당 개별 동적할당| 노드/버킷 동적할당 | 단일 연속 메모리 블록
// 캐시 효율성 | 낮음 (포인터 추적) | 보통 (해시 체이닝) | 최상 (연속 메모리 순회)
// ------------+--------------------+--------------------+------------------
// 탐색(Find)  | O(log N)           | O(1) [평균]        | O(log N) (이진 탐색)
// 범위(Range) | O(log N + K)       | O(N)               | O(log N + K) (연속 슬라이스)
// 삽입(Insert)| O(log N)           | O(1) [평균]        | O(N) (배열 요소 시프트)
// 삭제(Erase) | O(log N)           | O(1) [평균]        | O(N) (배열 요소 시프트)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 잦은 삽입/삭제가   | 대용량 데이터의    | 탐색과 순회가 압도적으로
//             | 발생하는 경우      | 빠른 단순 값 조회  | 많고 메모리가 제한된 경우
// ============+====================+====================+==================
//
// flat_set은 std::vector를 내부 저장소로 사용하여 항상 고유한(Unique) 키를
// 정렬된 상태로 유지하는 연속 메모리 연관 컨테이너입니다.
//
// flat_set<int> set;
// 
// // [1] 데이터 삽입 (O(N) - 정렬 위치 이진 탐색 후 고유값만 삽입)
// auto [it1, ok1] = set.insert(10); // {10}, ok1 = true
// auto [it2, ok2] = set.insert(5);  // {5, 10}, ok2 = true
// auto [it3, ok3] = set.insert(15); // {5, 10, 15}, ok3 = true
// auto [it4, ok4] = set.insert(10); // 중복, 삽입 실패(ok4 = false)
// 
// // [2] 상태 확인
// std::cout << "Size: " << set.size() << std::endl; // 3
// 
// // [3] find 및 contains를 이용한 이진 탐색 (O(log N))
// auto it = set.find(10);
// if (it != set.end()) {
//     std::cout << "Found: " << *it << std::endl; // 10
// }
// std::cout << "Contains 5: " << set.contains(5) << std::endl;   // 1 (true)
// std::cout << "Contains 20: " << set.contains(20) << std::endl; // 0 (false)
// 
// // [4] 범위 기반 for문 순회 (연속 메모리 순회로 캐시 최적화)
// for (const auto& val : set) {
//     std::cout << val << " "; // 5 10 15
// }
// std::cout << std::endl;
// 
// // [5] 값 기준 삭제 (O(N))
// size_t erased = set.erase(10); // 값 10 삭제
// std::cout << "Erased: " << erased << std::endl;     // 1
// std::cout << "Size: " << set.size() << std::endl;   // 2 (5, 15 남음)
// 
// // [6] 초기화
// set.clear();
// std::cout << "Is empty: " << set.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>, typename Allocator = std::allocator<T>>
    class flat_set {
    public:
        // 타입 정의 (Type Aliases)
        using key_type = T;
        using value_type = T;
        using key_compare = Compare;
        using value_compare = Compare;
        using allocator_type = Allocator;

        using container_type = std::vector<T, Allocator>;
        using pointer = typename container_type::pointer;
        using const_pointer = typename container_type::const_pointer;
        using reference = typename container_type::reference;
        using const_reference = typename container_type::const_reference;
        using iterator = typename container_type::const_iterator; // 원소의 불변성 보장
        using const_iterator = typename container_type::const_iterator;
        using reverse_iterator = typename container_type::const_reverse_iterator;
        using const_reverse_iterator = typename container_type::const_reverse_iterator;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;

    private:
        container_type data_;
        key_compare comp_;

    public:
        // 생성자 (Constructors)
        flat_set() = default;
        explicit flat_set(const key_compare& comp, const allocator_type& alloc = allocator_type())
            : data_(alloc), comp_(comp) {
        }
        explicit flat_set(const allocator_type& alloc)
            : data_(alloc), comp_() {
        }

        template <typename InputIt>
        flat_set(InputIt first, InputIt last, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
            : data_(alloc), comp_(comp) {
            for (; first != last; ++first) {
                insert(*first);
            }
        }

        flat_set(std::initializer_list<value_type> init, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
            : data_(alloc), comp_(comp) {
            for (const auto& item : init) {
                insert(item);
            }
        }

        // 반복자 (Iterators)
        const_iterator begin()  const noexcept { return data_.cbegin(); }
        const_iterator end()    const noexcept { return data_.cend(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }
        const_iterator cend()   const noexcept { return data_.cend(); }

        const_reverse_iterator rbegin()  const noexcept { return data_.crbegin(); }
        const_reverse_iterator rend()    const noexcept { return data_.crend(); }
        const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }
        const_reverse_iterator crend()   const noexcept { return data_.crend(); }

        // 용량 (Capacity)
        [[nodiscard]] bool      empty()    const noexcept { return data_.empty(); }
        [[nodiscard]] size_type size()     const noexcept { return data_.size(); }
        [[nodiscard]] size_type max_size() const noexcept { return data_.max_size(); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        void reserve(size_type new_cap) { data_.reserve(new_cap); }
        void shrink_to_fit() { data_.shrink_to_fit(); }

        // 수정자 (Modifiers)
        void clear() noexcept { data_.clear(); }

        std::pair<iterator, bool> insert(const value_type& value) {
            auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
            if (it != data_.end() && !comp_(value, *it)) {
                return { it, false };
            }
            auto inserted_it = data_.insert(it, value);
            return { inserted_it, true };
        }

        std::pair<iterator, bool> insert(value_type&& value) {
            auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
            if (it != data_.end() && !comp_(value, *it)) {
                return { it, false };
            }
            auto inserted_it = data_.insert(it, std::move(value));
            return { inserted_it, true };
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
        std::pair<iterator, bool> emplace(Args&&... args) {
            value_type tmp(std::forward<Args>(args)...);
            return insert(std::move(tmp));
        }

        iterator erase(const_iterator pos) {
            return data_.erase(pos);
        }

        iterator erase(const_iterator first, const_iterator last) {
            return data_.erase(first, last);
        }

        size_type erase(const key_type& key) {
            auto it = std::lower_bound(data_.begin(), data_.end(), key, comp_);
            if (it != data_.end() && !comp_(key, *it)) {
                data_.erase(it);
                return 1;
            }
            return 0;
        }

        void swap(flat_set& other) noexcept {
            using std::swap;
            swap(data_, other.data_);
            swap(comp_, other.comp_);
        }

        // 검색 (Lookup)
        const_iterator find(const key_type& key) const {
            auto it = lower_bound(key);
            if (it != end() && !comp_(key, *it)) {
                return it;
            }
            return end();
        }

        [[nodiscard]] size_type count(const key_type& key) const {
            return find(key) != end() ? 1 : 0;
        }

        [[nodiscard]] bool contains(const key_type& key) const {
            return std::binary_search(data_.begin(), data_.end(), key, comp_);
        }

        const_iterator lower_bound(const key_type& key) const {
            return std::lower_bound(data_.begin(), data_.end(), key, comp_);
        }

        const_iterator upper_bound(const key_type& key) const {
            return std::upper_bound(data_.begin(), data_.end(), key, comp_);
        }

        std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
            return std::equal_range(data_.begin(), data_.end(), key, comp_);
        }

        // 옵저버 (Observers)
        key_compare key_comp() const { return comp_; }
        value_compare value_comp() const { return comp_; }

        // ==========================================
        // 순수 ASCII 기반 Flat Set Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Flat Set Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Flat Set>\n\n";
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

