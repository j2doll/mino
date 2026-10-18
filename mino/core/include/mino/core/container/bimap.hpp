#pragma once

#include <unordered_map>
#include <optional>
#include <utility>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>

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
        using iterator = const_iterator;

        bimap() noexcept = default;
        ~bimap() noexcept = default;
        bimap(const bimap&) = default;
        bimap(bimap&&) noexcept = default;
        bimap& operator=(const bimap&) = default;
        bimap& operator=(bimap&&) noexcept = default;

        // ------------------------------------------------------------------------
        // 1. 데이터 삽입 멤버 함수 (메모리 할당 실패 시 noexcept 유지 및 false/실패 처리)
        // ------------------------------------------------------------------------

        // 복사 삽입
        bool insert(const TLeft& left, const TRight& right) noexcept {
            try {
                if (left_map_.find(left) != left_map_.end() || right_map_.find(right) != right_map_.end()) {
                    return false;
                }
                auto it_left = left_map_.emplace(left, right);
                try {
                    right_map_.emplace(right, left);
                }
                catch (...) {
                    left_map_.erase(it_left.first);
                    return false;
                }
                return true;
            }
            catch (...) {
                return false;
            }
        }

        // 이동 삽입
        bool insert(TLeft&& left, TRight&& right) noexcept {
            try {
                if (left_map_.find(left) != left_map_.end() || right_map_.find(right) != right_map_.end()) {
                    return false;
                }
                auto it_left = left_map_.emplace(std::move(left), right);
                try {
                    right_map_.emplace(std::move(right), it_left.first->first);
                }
                catch (...) {
                    left_map_.erase(it_left.first);
                    return false;
                }
                return true;
            }
            catch (...) {
                return false;
            }
        }

        // 강제 데이터 삽입
        bool force_insert(const TLeft& left, const TRight& right) noexcept {
            try {
                TLeft l_val = left;
                TRight r_val = right;
                return force_insert_impl(std::move(l_val), std::move(r_val));
            }
            catch (...) {
                return false;
            }
        }

        bool force_insert(TLeft&& left, TRight&& right) noexcept {
            return force_insert_impl(std::move(left), std::move(right));
        }

        // ------------------------------------------------------------------------
        // 2. 데이터 조회 멤버 함수
        // ------------------------------------------------------------------------

        [[nodiscard]] bool contains_left(const TLeft& left) const noexcept {
            try {
                return left_map_.find(left) != left_map_.end();
            }
            catch (...) {
                return false;
            }
        }

        [[nodiscard]] bool contains_right(const TRight& right) const noexcept {
            try {
                return right_map_.find(right) != right_map_.end();
            }
            catch (...) {
                return false;
            }
        }

        [[nodiscard]] const TRight* find_by_left(const TLeft& left) const noexcept {
            try {
                auto it = left_map_.find(left);
                return (it != left_map_.end()) ? &(it->second) : nullptr;
            }
            catch (...) {
                return nullptr;
            }
        }

        [[nodiscard]] const TLeft* find_by_right(const TRight& right) const noexcept {
            try {
                auto it = right_map_.find(right);
                return (it != right_map_.end()) ? &(it->second) : nullptr;
            }
            catch (...) {
                return nullptr;
            }
        }

        [[nodiscard]] std::optional<TRight> get_by_left(const TLeft& left) const noexcept {
            try {
                auto it = left_map_.find(left);
                if (it != left_map_.end()) return it->second;
            }
            catch (...) {}
            return std::nullopt;
        }

        [[nodiscard]] std::optional<TLeft> get_by_right(const TRight& right) const noexcept {
            try {
                auto it = right_map_.find(right);
                if (it != right_map_.end()) return it->second;
            }
            catch (...) {}
            return std::nullopt;
        }

        // ------------------------------------------------------------------------
        // 3. 데이터 삭제 멤버 함수
        // ------------------------------------------------------------------------

        bool erase_by_left(const TLeft& left) noexcept {
            try {
                auto it = left_map_.find(left);
                if (it == left_map_.end()) return false;

                right_map_.erase(it->second);
                left_map_.erase(it);
                return true;
            }
            catch (...) {
                return false;
            }
        }

        bool erase_by_right(const TRight& right) noexcept {
            try {
                auto it = right_map_.find(right);
                if (it == right_map_.end()) return false;

                left_map_.erase(it->second);
                right_map_.erase(it);
                return true;
            }
            catch (...) {
                return false;
            }
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
        // 5. 반복자
        // ------------------------------------------------------------------------

        const_iterator begin() const noexcept { return left_map_.cbegin(); }
        const_iterator end() const noexcept { return left_map_.cend(); }
        const_iterator cbegin() const noexcept { return left_map_.cbegin(); }
        const_iterator cend() const noexcept { return left_map_.cend(); }

        // ------------------------------------------------------------------------
        // 6. 터미널 출력
        // ------------------------------------------------------------------------
        void dump(const std::string& title = "") const noexcept {
            try {
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
            catch (...) {}
        }

    private:
        bool force_insert_impl(TLeft&& left, TRight&& right) noexcept {
            try {
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
                try {
                    right_map_.emplace(std::move(right), it_inserted.first->first);
                }
                catch (...) {
                    left_map_.erase(it_inserted.first);
                    return false;
                }
                return true;
            }
            catch (...) {
                return false;
            }
        }
    };

    template <typename TLeft, typename TRight, typename HL, typename HR, typename EL, typename ER>
    void swap(bimap<TLeft, TRight, HL, HR, EL, ER>& lhs, bimap<TLeft, TRight, HL, HR, EL, ER>& rhs) noexcept {
        lhs.swap(rhs);
    }

} // namespace mino::core::container
