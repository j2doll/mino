#pragma once

#include <unordered_map>
#include <optional>
#include <utility>

namespace mino::core::container {

    template <typename TLeft, typename TRight>
    class  bimap {
    private:
        std::unordered_map<TLeft, TRight> left_map;
        std::unordered_map<TRight, TLeft> right_map;

    public:
        // ------------------------------------------------------------------------
        // 1. 데이터 삽입 및 수정 멤버 함수
        // ------------------------------------------------------------------------

        // 데이터 삽입 (성공 시 true, 이미 키나 값이 존재하면 false 반환)
        bool insert(const TLeft& left, const TRight& right) {
            if (left_map.find(left) != left_map.end() || right_map.find(right) != right_map.end()) {
                return false;
            }
            left_map[left] = right;
            right_map[right] = left;
            return true;
        }

        // 강제 데이터 삽입 (기존 매핑 관계를 깨부수고 무조건 고유한 1:1 매핑 연결)
        void force_insert(const TLeft& left, const TRight& right) {
            auto it_left = left_map.find(left);
            if (it_left != left_map.end()) {
                right_map.erase(it_left->second);
            }

            auto it_right = right_map.find(right);
            if (it_right != right_map.end()) {
                left_map.erase(it_right->second);
            }

            left_map[left] = right;
            right_map[right] = left;
        }

        // ------------------------------------------------------------------------
        // 2. 데이터 조회 멤버 함수 (C++17 std::optional 활용)
        // ------------------------------------------------------------------------

        // 왼쪽 키로 오른쪽 값 찾기
        std::optional<TRight> get_by_left(const TLeft& left) const {
            auto it = left_map.find(left);
            if (it != left_map.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        // 오른쪽 값으로 왼쪽 키 찾기
        std::optional<TLeft> get_by_right(const TRight& right) const {
            auto it = right_map.find(right);
            if (it != right_map.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        // ------------------------------------------------------------------------
        // 3. 데이터 삭제 멤버 함수
        // ------------------------------------------------------------------------

        // 왼쪽 키 기준으로 양방향 연쇄 삭제
        bool erase_by_left(const TLeft& left) {
            auto it = left_map.find(left);
            if (it == left_map.end()) return false;

            right_map.erase(it->second);
            left_map.erase(it);
            return true;
        }

        // 오른쪽 값 기준으로 양방향 연쇄 삭제
        bool erase_by_right(const TRight& right) {
            auto it = right_map.find(right);
            if (it == right_map.end()) return false;

            left_map.erase(it->second);
            right_map.erase(it);
            return true;
        }

        // ------------------------------------------------------------------------
        // 4. 용량 및 초기화 멤버 함수
        // ------------------------------------------------------------------------

        size_t size() const noexcept { return left_map.size(); }
        bool empty() const noexcept { return left_map.empty(); }

        void clear() noexcept {
            left_map.clear();
            right_map.clear();
        }

        // ------------------------------------------------------------------------
        // 5. 반복자 (C++17 구조체 분해 및 범위 기반 for문 지원)
        // ------------------------------------------------------------------------

        auto begin() const -> typename std::unordered_map<TLeft, TRight>::const_iterator {
            return left_map.begin();
        }

        auto end() const -> typename std::unordered_map<TLeft, TRight>::const_iterator {
            return left_map.end();
        }
    };

} // namespace mino::core::container 

