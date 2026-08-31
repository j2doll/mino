#pragma once

#include <unordered_map>
#include <optional>
#include <tuple>
#include <utility>
#include <list>
#include <cstddef>
#include <string>
#include <iostream>
#include <functional>

// ============+====================+====================+==================
// 항목        |  std::unordered_map|  std::map          |  multi_index_
//             |                    |                    |  container
// ------------+--------------------+--------------------+------------------
// 키(Key) 개수| 1개 (단일 키)      | 1개 (단일 키)      | 2개 (독립된 2중 고유 키)
// 검색 지원   | Key -> Value       | Key -> Value       | Key1/Key2 -> Value 양방향
// 고유성 보장 | Key 고유           | Key 고유           | Key1, Key2 각각 100% 고유
// ------------+--------------------+--------------------+------------------
// 삽입(Insert)| O(1) [평균]        | O(log N)           | O(1) [평균] (2중 인덱싱)
// 검색(Lookup)| O(1) [평균]        | O(log N)           | O(1) [평균] (Key1/Key2 모두)
// 삭제(Erase) | O(1) [평균]        | O(log N)           | O(1) [평균] (반복자 즉시 삭제)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 단일 식별자 관리   | 정렬이 필요한 단일 | RDBMS의 기본키/보조키 복합
//             |                    | 식별자 관리        | 식별자 메모리 캐싱 (ID+이메일)
// ============+====================+====================+==================
//
// multi_index_container는 단일 Value에 대해 서로 다른 두 개의 고유 키(Key1, Key2)를
// 연결하여 두 키 모두에서 O(1) 빠른 검색 및 삭제를 지원하는 2중 인덱스 컨테이너입니다.
//
// struct User {
//     std::string name;
//     int age;
//     User(std::string n, int a) : name(std::move(n)), age(a) {}
// };
// 
// // ID(int)와 이메일(std::string)로 User를 관리하는 2중 인덱스 컨테이너
// multi_index_container<User, int, std::string> users;
// 
// // [1] 데이터 삽입 (Key1과 Key2 모두 고유할 때만 성공, O(1))
// users.insert(User("Alice", 25), 1, "alice@example.com");
// users.insert(User("Bob", 30), 2, "bob@example.com");
// users.insert(User("Charlie", 28), 3, "charlie@example.com");
// 
// std::cout << "Size: " << users.size() << std::endl; // 3
// 
// // [2] Key1 (ID) 또는 Key2 (Email)로 O(1) 검색
// if (const auto* u = users.find_by_key1(1)) {
//     std::cout << "Found by ID 1: " << u->name << ", Age: " << u->age << std::endl;
// }
// if (const auto* u = users.find_by_key2("charlie@example.com")) {
//     std::cout << "Found by Email: " << u->name << ", Age: " << u->age << std::endl;
// }
// 
// // [3] Key1 또는 Key2 기준으로 O(1) 즉시 삭제
// users.erase_by_key1(2);                      // Bob 삭제
// users.erase_by_key2("charlie@example.com"); // Charlie 삭제
// std::cout << "Size after erase: " << users.size() << std::endl; // 1 (Alice만 남음)
// 
// // [4] 중복 키 삽입 방지 검증
// bool ok1 = users.insert(User("David", 35), 1, "david@example.com"); // ID 1 중복 -> false
// bool ok2 = users.insert(User("Eve", 29), 4, "alice@example.com");   // Email 중복 -> false
// 
// // [5] 초기화
// users.clear();
// std::cout << "Is empty: " << users.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <
        typename Value,
        typename Key1,
        typename Key2,
        typename Hash1 = std::hash<Key1>,
        typename Hash2 = std::hash<Key2>,
        typename Equal1 = std::equal_to<Key1>,
        typename Equal2 = std::equal_to<Key2>
    >
    class multi_index_container {
    public:
        using value_type = Value;
        using key1_type = Key1;
        using key2_type = Key2;
        using size_type = std::size_t;

        struct entry {
            value_type value;
            key1_type k1;
            key2_type k2;

            template <typename V, typename K1, typename K2>
            entry(V&& val, K1&& key1, K2&& key2)
                : value(std::forward<V>(val)), k1(std::forward<K1>(key1)), k2(std::forward<K2>(key2)) {
            }
        };

    private:
        using storage_type = std::list<entry>;
        using storage_iterator = typename storage_type::iterator;
        using const_storage_iterator = typename storage_type::const_iterator;

        storage_type storage_;
        std::unordered_map<key1_type, storage_iterator, Hash1, Equal1> by_key1_;
        std::unordered_map<key2_type, storage_iterator, Hash2, Equal2> by_key2_;

    public:
        multi_index_container() = default;
        ~multi_index_container() = default;

        // --------------------------------------------------------------------
        // 1. 데이터 삽입 및 인플레이스 생성
        // --------------------------------------------------------------------
        bool insert(const value_type& value, const key1_type& k1, const key2_type& k2) {
            if (by_key1_.find(k1) != by_key1_.end() || by_key2_.find(k2) != by_key2_.end()) {
                return false;
            }

            storage_.emplace_back(value, k1, k2);
            storage_iterator it = std::prev(storage_.end());

            by_key1_.emplace(k1, it);
            by_key2_.emplace(k2, it);
            return true;
        }

        bool insert(value_type&& value, const key1_type& k1, const key2_type& k2) {
            if (by_key1_.find(k1) != by_key1_.end() || by_key2_.find(k2) != by_key2_.end()) {
                return false;
            }

            storage_.emplace_back(std::move(value), k1, k2);
            storage_iterator it = std::prev(storage_.end());

            by_key1_.emplace(k1, it);
            by_key2_.emplace(k2, it);
            return true;
        }

        template <typename... Args>
        bool emplace(const key1_type& k1, const key2_type& k2, Args&&... args) {
            if (by_key1_.find(k1) != by_key1_.end() || by_key2_.find(k2) != by_key2_.end()) {
                return false;
            }

            storage_.emplace_back(value_type(std::forward<Args>(args)...), k1, k2);
            storage_iterator it = std::prev(storage_.end());

            by_key1_.emplace(k1, it);
            by_key2_.emplace(k2, it);
            return true;
        }

        // --------------------------------------------------------------------
        // 2. 데이터 검색 (O(1) 포인터 반환)
        // --------------------------------------------------------------------
        [[nodiscard]] const value_type* find_by_key1(const key1_type& k1) const {
            auto it = by_key1_.find(k1);
            if (it == by_key1_.end()) return nullptr;
            return &(it->second->value);
        }

        [[nodiscard]] value_type* find_by_key1(const key1_type& k1) {
            auto it = by_key1_.find(k1);
            if (it == by_key1_.end()) return nullptr;
            return &(it->second->value);
        }

        [[nodiscard]] const value_type* find_by_key2(const key2_type& k2) const {
            auto it = by_key2_.find(k2);
            if (it == by_key2_.end()) return nullptr;
            return &(it->second->value);
        }

        [[nodiscard]] value_type* find_by_key2(const key2_type& k2) {
            auto it = by_key2_.find(k2);
            if (it == by_key2_.end()) return nullptr;
            return &(it->second->value);
        }

        [[nodiscard]] bool contains_key1(const key1_type& k1) const {
            return by_key1_.find(k1) != by_key1_.end();
        }

        [[nodiscard]] bool contains_key2(const key2_type& k2) const {
            return by_key2_.find(k2) != by_key2_.end();
        }

        // --------------------------------------------------------------------
        // 3. 데이터 삭제 (반복자를 통한 O(1) 즉시 삭제)
        // --------------------------------------------------------------------
        bool erase_by_key1(const key1_type& k1) {
            auto it1 = by_key1_.find(k1);
            if (it1 == by_key1_.end()) return false;

            storage_iterator sit = it1->second;
            by_key2_.erase(sit->k2);
            by_key1_.erase(it1);
            storage_.erase(sit);
            return true;
        }

        bool erase_by_key2(const key2_type& k2) {
            auto it2 = by_key2_.find(k2);
            if (it2 == by_key2_.end()) return false;

            storage_iterator sit = it2->second;
            by_key1_.erase(sit->k1);
            by_key2_.erase(it2);
            storage_.erase(sit);
            return true;
        }

        // --------------------------------------------------------------------
        // 4. 용량 및 초기화
        // --------------------------------------------------------------------
        [[nodiscard]] size_type size() const noexcept { return storage_.size(); }
        [[nodiscard]] bool empty() const noexcept { return storage_.empty(); }

        void clear() noexcept {
            storage_.clear();
            by_key1_.clear();
            by_key2_.clear();
        }

        // --------------------------------------------------------------------
        // 5. 반복자 (범위 기반 for문 지원)
        // --------------------------------------------------------------------
        storage_iterator begin() noexcept { return storage_.begin(); }
        storage_iterator end() noexcept { return storage_.end(); }
        const_storage_iterator begin() const noexcept { return storage_.cbegin(); }
        const_storage_iterator end() const noexcept { return storage_.cend(); }
        const_storage_iterator cbegin() const noexcept { return storage_.cbegin(); }
        const_storage_iterator cend() const noexcept { return storage_.cend(); }

        // --------------------------------------------------------------------
        // 6. 순수 ASCII 기반 Multi Index Dump
        // --------------------------------------------------------------------
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Multi Index Container Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Container>\n\n";
                return;
            }

            size_type count = 0;
            size_type total = storage_.size();
            for (const auto& entry_item : storage_) {
                bool is_last = (++count == total);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "Key1: [" << entry_item.k1 << "] | Key2: ["
                    << entry_item.k2 << "]\n";
            }
            std::cout << "\n";
        }
    };

} // namespace mino::core::container
