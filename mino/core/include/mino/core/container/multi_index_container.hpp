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
// 키(Key) 개수| 1개 (단일 키)      | 1개 (단일 키)      | n 개 (독립된 n중 고유 키)
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

    template <typename Value, typename... Keys>
    class multi_index_container {
        static_assert(sizeof...(Keys) > 0, "At least one key type is required.");

    public:
        using value_type = Value;
        using size_type = std::size_t;

        // N번째 키의 타입 조회
        template <std::size_t I>
        using key_type_at = std::tuple_element_t<I, std::tuple<Keys...>>;

        struct entry {
            value_type value;
            std::tuple<Keys...> keys;

            template <typename V, typename... Ks>
            entry(V&& val, Ks&&... ks)
                : value(std::forward<V>(val)), keys(std::forward<Ks>(ks)...) {
            }
        };

    private:
        using storage_type = std::list<entry>;
        using storage_iterator = typename storage_type::iterator;
        using const_storage_iterator = typename storage_type::const_iterator;

        storage_type storage_;

        // 가변 인자 Keys에 대응하는 std::unordered_map들의 튜플 생성 헬퍼
        template <typename... Ts>
        struct map_tuple_helper;

        template <typename T, typename... Rest>
        struct map_tuple_helper<T, Rest...> {
            using type = decltype(std::tuple_cat(
                std::tuple<std::unordered_map<T, storage_iterator>>{},
                typename map_tuple_helper<Rest...>::type{}
            ));
        };

        template <>
        struct map_tuple_helper<> {
            using type = std::tuple<>;
        };

        using maps_tuple_type = typename map_tuple_helper<Keys...>::type;
        maps_tuple_type index_maps_;

        // 모든 키에 대해 중복 검사 (Fold Expression 활용)
        template <std::size_t... Is, typename... Ks>
        bool check_keys_exist_impl(std::index_sequence<Is...>, const Ks&... ks) const {
            bool exists = false;
            ((exists = exists || (std::get<Is>(index_maps_).find(ks) != std::get<Is>(index_maps_).end())), ...);
            return exists;
        }

        template <typename... Ks>
        bool check_keys_exist(const Ks&... ks) const {
            return check_keys_exist_impl(std::index_sequence_for<Keys...>{}, ks...);
        }

        // 모든 인덱스 맵에 반복자 등록
        template <std::size_t... Is, typename... Ks>
        void insert_maps_impl(std::index_sequence<Is...>, storage_iterator it, const Ks&... ks) {
            (std::get<Is>(index_maps_).emplace(ks, it), ...);
        }

        template <typename... Ks>
        void insert_maps(storage_iterator it, const Ks&... ks) {
            insert_maps_impl(std::index_sequence_for<Keys...>{}, it, ks...);
        }

        // 모든 인덱스 맵에서 키 제거
        template <std::size_t... Is>
        void erase_maps_impl(std::index_sequence<Is...>, const std::tuple<Keys...>& k_tuple) {
            (std::get<Is>(index_maps_).erase(std::get<Is>(k_tuple)), ...);
        }

        void erase_maps(const std::tuple<Keys...>& k_tuple) {
            erase_maps_impl(std::index_sequence_for<Keys...>{}, k_tuple);
        }

    public:
        multi_index_container() = default;
        ~multi_index_container() = default;

        // --------------------------------------------------------------------
        // 1. 데이터 삽입 및 인플레이스 생성
        // --------------------------------------------------------------------
        bool insert(const value_type& value, const Keys&... keys) {
            if (check_keys_exist(keys...)) {
                return false; // 어떤 인덱스든 키가 중복되면 실패
            }

            storage_.emplace_back(value, keys...);
            storage_iterator it = std::prev(storage_.end());
            insert_maps(it, keys...);
            return true;
        }

        bool insert(value_type&& value, const Keys&... keys) {
            if (check_keys_exist(keys...)) {
                return false;
            }

            storage_.emplace_back(std::move(value), keys...);
            storage_iterator it = std::prev(storage_.end());
            insert_maps(it, keys...);
            return true;
        }

        template <typename... Args>
        bool emplace(const std::tuple<Keys...>& keys_tuple, Args&&... args) {
            // 튜플 형태로 키를 받을 수 있는 emplace 오버로드
            bool inserted = false;
            auto apply_emplace = [&](const auto&... ks) {
                if (check_keys_exist(ks...)) {
                    return;
                }
                storage_.emplace_back(value_type(std::forward<Args>(args)...), ks...);
                storage_iterator it = std::prev(storage_.end());
                insert_maps(it, ks...);
                inserted = true;
                };
            std::apply(apply_emplace, keys_tuple);
            return inserted;
        }

        // --------------------------------------------------------------------
        // 2. 데이터 검색 (N번째 인덱스 기준 O(1))
        // --------------------------------------------------------------------
        template <std::size_t N>
        [[nodiscard]] const value_type* find_by_index(const key_type_at<N>& key) const {
            const auto& map = std::get<N>(index_maps_);
            auto it = map.find(key);
            if (it == map.end()) return nullptr;
            return &(it->second->value);
        }

        template <std::size_t N>
        [[nodiscard]] value_type* find_by_index(const key_type_at<N>& key) {
            auto& map = std::get<N>(index_maps_);
            auto it = map.find(key);
            if (it == map.end()) return nullptr;
            return &(it->second->value);
        }

        template <std::size_t N>
        [[nodiscard]] bool contains(const key_type_at<N>& key) const {
            return std::get<N>(index_maps_).find(key) != std::get<N>(index_maps_).end();
        }

        // --------------------------------------------------------------------
        // 3. 데이터 삭제 (N번째 인덱스 기준)
        // --------------------------------------------------------------------
        template <std::size_t N>
        bool erase_by_index(const key_type_at<N>& key) {
            auto& map = std::get<N>(index_maps_);
            auto it = map.find(key);
            if (it == map.end()) return false;

            storage_iterator sit = it->second;
            erase_maps(sit->keys); // 모든 인덱스 맵에서 제거
            storage_.erase(sit);   // 실제 스토리지에서 제거
            return true;
        }

        // --------------------------------------------------------------------
        // 4. 용량 및 초기화
        // --------------------û------------------------------------------------
        [[nodiscard]] size_type size() const noexcept { return storage_.size(); }
        [[nodiscard]] bool empty() const noexcept { return storage_.empty(); }

        void clear() noexcept {
            storage_.clear();
            std::apply([](auto&... maps) { (maps.clear(), ...); }, index_maps_);
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
    };

} // namespace mino::core::container
