#pragma once

#include <unordered_map>
#include <optional>
#include <tuple>
#include <utility>
#include <list>
#include <cstddef>
#include <string>

namespace mino::core::container {

    template <typename Value, typename Key1, typename Key2>
     class multi_index_container {
    public:
        using value_type = Value;
        using key1_type = Key1;
        using key2_type = Key2;

        // 데이터 삽입
        bool insert(value_type value, const key1_type& k1, const key2_type& k2) {
            // [수정] const_find -> find 로 변경
            if (by_key1_.find(k1) != by_key1_.end() || by_key2_.find(k2) != by_key2_.end()) {
                return false;
            }

            // 내부 스토리지에 저장
            auto& inserted_entry = storage_.emplace_back(std::move(value), k1, k2);

            // 인덱스 맵에 포인터 연결
            by_key1_[k1] = &inserted_entry;
            by_key2_[k2] = &inserted_entry;

            return true;
        }

        // Key1으로 검색
        const value_type* find_by_key1(const key1_type& k1) const {
            auto it = by_key1_.find(k1);
            if (it == by_key1_.end()) {
                return nullptr;
            }
            return &(std::get<0>(*(it->second)));
        }

        // Key2로 검색
        const value_type* find_by_key2(const key2_type& k2) const {
            auto it = by_key2_.find(k2);
            if (it == by_key2_.end()) {
                return nullptr;
            }
            return &(std::get<0>(*(it->second)));
        }

        // Key1으로 삭제
        bool erase_by_key1(const key1_type& k1) {
            auto it = by_key1_.find(k1);
            if (it == by_key1_.end()) {
                return false;
            }

            const key2_type& k2 = std::get<2>(*(it->second));
            by_key2_.erase(k2);

            // storage_에서 제거하기 위해 반복자 검색
            for (auto storage_it = storage_.begin(); storage_it != storage_.end(); ++storage_it) {
                if (&(*storage_it) == it->second) {
                    storage_.erase(storage_it);
                    break;
                }
            }

            by_key1_.erase(it);
            return true;
        }

        // 컨테이너 크기 반환
        std::size_t size() const noexcept {
            return storage_.size();
        }

        // 컨테이너 비우기
        void clear() noexcept {
            storage_.clear();
            by_key1_.clear();
            by_key2_.clear();
        }

    private:
        using entry_type = std::tuple<value_type, key1_type, key2_type>;

        std::list<entry_type> storage_;
        std::unordered_map<key1_type, entry_type*> by_key1_;
        std::unordered_map<key2_type, entry_type*> by_key2_;
    };

} // namespace mino::core::container
