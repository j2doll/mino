#pragma once

#include <unordered_map>
#include <optional>
#include <utility>
#include <stdexcept>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ============+====================+====================+==================
// 항목        |  std::map          |  std::unordered_map|  bimap (양방향 맵)
// ------------+--------------------+--------------------+------------------
// 매핑 방향   | 단방향 (Key->Val)  | 단방향 (Key->Val)  | 양방향 (Left <-> Right)
// 고유성 조건 | Key만 고유         | Key만 고유         | Left와 Right 모두 고유(1:1)
// 내부 구조   | Red-Black Tree     | Hash Table         | Dual Hash Tables
// ------------+--------------------+--------------------+------------------
// 삽입(Insert)| O(log N)           | O(1) [평균]        | O(1) [평균]
// 조회(Lookup)| O(log N)           | O(1) [평균]        | O(1) [평균] (양방향 모두)
// 삭제(Erase) | O(log N)           | O(1) [평균]        | O(1) [평균] (양방향 연쇄 삭제)
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 단방향 정렬 맵     | 단순 단방향 키 매핑| 1:1 양방향 고유 매핑
//             |                    |                    | (ID <-> Name / Socket <-> Session)
// ============+====================+====================+==================
//
// bimap(Bidirectional Map)은 왼쪽(Left)과 오른쪽(Right) 간의 완벽한 1:1 고유 매핑을
// 보장하는 양방향 해시 컨테이너입니다.
//
// bimap<int, std::string> map;
// 
// // [1] 데이터 삽입 (O(1) 평균): Left와 Right 모두 중복이 없을 때만 성공
// map.insert(1, "Alice");
// map.insert(2, "Bob");
// bool ok = map.insert(1, "Duplicate"); // 실패 (1이 이미 존재 -> false 반환)
// 
// // [2] 강제 데이터 삽입 (force_insert): 기존 충돌 매핑을 자동으로 끊고 1:1 연결
// map.force_insert(2, "Charlie"); // 기존 {2, "Bob"} 매핑 해제 후 {2, "Charlie"} 연결
// 
// // [3] 데이터 조회 (O(1) 평균)
// // - 존재 여부 확인 (빠른 O(1) 검사)
// if (map.contains_left(1)) { /* ... */ }
// 
// // - 포인터 기반 조회 (복사 비용 0, 미존재 시 nullptr)
// const std::string* name = map.find_by_left(1);
// const int* id = map.find_by_right("Charlie");
// 
// // - std::optional 기반 조회 (값 복사 발생)
// auto opt_name = map.get_by_left(1); // opt_name.value() == "Alice"
// 
// // [4] 순회 (Left 기준 1:1 매핑 순회)
// for (const auto& [left, right] : map) {
//     std::cout << left << " <---> " << right << std::endl;
// }
// 
// // [5] 데이터 삭제 (연쇄 삭제)
// map.erase_by_left(1);          // {1, "Alice"} 양방향 동시 삭제
// map.erase_by_right("Charlie"); // {2, "Charlie"} 양방향 동시 삭제
// 
// std::cout << "Empty: " << map.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <
        typename TLeft,
        typename TRight,
        typename HashLeft = std::hash<TLeft>,
        typename HashRight = std::hash<TRight>,
        typename EqualLeft = std::equal_to<TLeft>,
        typename EqualRight = std::equal_to<TRight>
    >
    class bimap {
    public:
        using left_key_type = TLeft;
        using right_key_type = TRight;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

    private:
        using left_map_type = std::unordered_map<TLeft, TRight, HashLeft, EqualLeft>;
        using right_map_type = std::unordered_map<TRight, TLeft, HashRight, EqualRight>;

        left_map_type left_map_;
        right_map_type right_map_;

    public:
        using const_iterator = typename left_map_type::const_iterator;
        using iterator = const_iterator; // 원소의 불변성을 위해 const_iterator로 제한

        bimap() = default;
        ~bimap() = default;
        bimap(const bimap&) = default;
        bimap(bimap&&) noexcept = default;
        bimap& operator=(const bimap&) = default;
        bimap& operator=(bimap&&) noexcept = default;

        // ------------------------------------------------------------------------
        // 1. 데이터 삽입 멤버 함수
        // ------------------------------------------------------------------------

        // 복사 삽입: Left나 Right 중 하나라도 이미 존재하면 false 반환
        bool insert(const TLeft& left, const TRight& right) {
            if (left_map_.find(left) != left_map_.end() || right_map_.find(right) != right_map_.end()) {
                return false;
            }
            left_map_.emplace(left, right);
            right_map_.emplace(right, left);
            return true;
        }

        // 이동 삽입: Rvalue 전달 시 불필요한 복사 방지
        bool insert(TLeft&& left, TRight&& right) {
            if (left_map_.find(left) != left_map_.end() || right_map_.find(right) != right_map_.end()) {
                return false;
            }
            auto it_left = left_map_.emplace(std::move(left), right);
            right_map_.emplace(std::move(right), it_left.first->first);
            return true;
        }

        // 강제 데이터 삽입: 기존 매핑을 정리하고 무조건 고유한 1:1 매핑 생성 (자가 참조 안전)
        void force_insert(const TLeft& left, const TRight& right) {
            TLeft l_val = left;
            TRight r_val = right;
            force_insert_impl(std::move(l_val), std::move(r_val));
        }

        void force_insert(TLeft&& left, TRight&& right) {
            force_insert_impl(std::move(left), std::move(right));
        }

        // ------------------------------------------------------------------------
        // 2. 데이터 조회 멤버 함수
        // ------------------------------------------------------------------------

        // 존재 여부 확인 (O(1))
        [[nodiscard]] bool contains_left(const TLeft& left) const {
            return left_map_.find(left) != left_map_.end();
        }

        [[nodiscard]] bool contains_right(const TRight& right) const {
            return right_map_.find(right) != right_map_.end();
        }

        // 포인터 기반 조회 (복사 비용 없음, 미존재 시 nullptr 반환)
        [[nodiscard]] const TRight* find_by_left(const TLeft& left) const {
            auto it = left_map_.find(left);
            return (it != left_map_.end()) ? &(it->second) : nullptr;
        }

        [[nodiscard]] const TLeft* find_by_right(const TRight& right) const {
            auto it = right_map_.find(right);
            return (it != right_map_.end()) ? &(it->second) : nullptr;
        }

        // 참조 기반 조회 (미존재 시 std::out_of_range 예외 발생)
        [[nodiscard]] const TRight& at_left(const TLeft& left) const {
            auto it = left_map_.find(left);
            if (it == left_map_.end()) {
                throw std::out_of_range("bimap: left key not found");
            }
            return it->second;
        }

        [[nodiscard]] const TLeft& at_right(const TRight& right) const {
            auto it = right_map_.find(right);
            if (it == right_map_.end()) {
                throw std::out_of_range("bimap: right key not found");
            }
            return it->second;
        }

        // std::optional 기반 조회 (값 복사 발생)
        [[nodiscard]] std::optional<TRight> get_by_left(const TLeft& left) const {
            auto it = left_map_.find(left);
            if (it != left_map_.end()) return it->second;
            return std::nullopt;
        }

        [[nodiscard]] std::optional<TLeft> get_by_right(const TRight& right) const {
            auto it = right_map_.find(right);
            if (it != right_map_.end()) return it->second;
            return std::nullopt;
        }

        // ------------------------------------------------------------------------
        // 3. 데이터 삭제 멤버 함수
        // ------------------------------------------------------------------------

        // 왼쪽 키 기준으로 양방향 연쇄 삭제
        bool erase_by_left(const TLeft& left) {
            auto it = left_map_.find(left);
            if (it == left_map_.end()) return false;

            right_map_.erase(it->second);
            left_map_.erase(it);
            return true;
        }

        // 오른쪽 값 기준으로 양방향 연쇄 삭제
        bool erase_by_right(const TRight& right) {
            auto it = right_map_.find(right);
            if (it == right_map_.end()) return false;

            left_map_.erase(it->second);
            right_map_.erase(it);
            return true;
        }

        // ------------------------------------------------------------------------
        // 4. 용량 및 초기화 멤버 함수
        // ------------------------------------------------------------------------

        [[nodiscard]] size_type size() const noexcept { return left_map_.size(); }
        [[nodiscard]] bool empty() const noexcept { return left_map_.empty(); }

        void clear() noexcept {
            left_map_.clear();
            right_map_.clear();
        }

        void swap(bimap& other) noexcept {
            using std::swap;
            swap(left_map_, other.left_map_);
            swap(right_map_, other.right_map_);
        }

        // ------------------------------------------------------------------------
        // 5. 반복자 (C++17 구조체 분해 및 범위 기반 for문 지원)
        // ------------------------------------------------------------------------

        const_iterator begin() const noexcept { return left_map_.cbegin(); }
        const_iterator end() const noexcept { return left_map_.cend(); }
        const_iterator cbegin() const noexcept { return left_map_.cbegin(); }
        const_iterator cend() const noexcept { return left_map_.cend(); }

        // ------------------------------------------------------------------------
        // 6. 터미널 ASCII 덤프
        // ------------------------------------------------------------------------
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== BiMap Dump (Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty BiMap>\n\n";
                return;
            }

            size_t count = 0;
            size_t total = left_map_.size();
            for (const auto& [l, r] : left_map_) {
                bool is_last = (++count == total);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << l << "] <---> [" << r << "]\n";
            }
            std::cout << "\n";
        }

    private:
        void force_insert_impl(TLeft&& left, TRight&& right) {
            auto it_left = left_map_.find(left);
            if (it_left != left_map_.end()) {
                right_map_.erase(it_left->second);
                left_map_.erase(it_left);
            }

            auto it_right = right_map_.find(right);
            if (it_right != right_map_.end()) {
                left_map_.erase(it_right->second);
                right_map_.erase(it_right);
            }

            auto it_inserted = left_map_.emplace(std::move(left), right);
            right_map_.emplace(std::move(right), it_inserted.first->first);
        }
    };

    template <typename TLeft, typename TRight, typename HL, typename HR, typename EL, typename ER>
    void swap(bimap<TLeft, TRight, HL, HR, EL, ER>& lhs, bimap<TLeft, TRight, HL, HR, EL, ER>& rhs) noexcept {
        lhs.swap(rhs);
    }

} // namespace mino::core::container

