#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <tuple>
#include <optional>
#include <chrono>

#include "mino/core/enum/enum.hpp"
#include "mino/core/schedule/task/period_strategy.hpp"

// (6) 공휴일 처리 옵션 정책
MINO_DEFINE_ENUM_NAMESPACE(mino::core::schedule::task, holiday_policy,
    skip,               // (6.1) 옵션 1) 공휴일이면 건너뜀 (디폴트)
    shift_to_previous,  // (6.2) 옵션 2) 직전 비휴일로 앞당김
    shift_to_next       // (6.3) 옵션 3) 직후 비휴일로 미룸
)

namespace mino::core::schedule::task {

    class holiday_calendar {
    public:
        holiday_calendar();
        explicit holiday_calendar(time_base base, holiday_policy policy = holiday_policy::skip);

        // --- (8), (16) 시간 기준 및 정책 설정/조회 ---
        void set_time_base(time_base base);
        time_base get_time_base() const;

        void set_policy(holiday_policy p);
        holiday_policy get_policy() const;

        // --- (1) 토요일, 일요일 등 주말 휴무 설정 ---
        void set_weekend_holiday(weekday wd, bool is_holiday);
        bool is_weekend_holiday(weekday wd) const;

        // --- (2), (10) 매년 반복형 공휴일 {월, 일} ---
        void add_recurring_holiday(int month, int day);
        void remove_recurring_holiday(int month, int day);

        // --- (3), (10) 1회성 공휴일 {년, 월, 일} ---
        void add_specific_holiday(int year, int month, int day);
        void remove_specific_holiday(int year, int month, int day);

        // --- (4), (10) 1회성 제외일 {년, 월, 일} ---
        void add_specific_exclusion(int year, int month, int day);
        void remove_specific_exclusion(int year, int month, int day);

        // --- (5), (10) 매년 반복형 제외일 {월, 일} ---
        void add_recurring_exclusion(int month, int day);
        void remove_recurring_exclusion(int month, int day);

        // --- 판정 로직 ---
        bool is_holiday(int year, int month, int day, weekday wd) const;
        bool is_holiday(const date_time_parts& parts) const;
        bool is_holiday(std::chrono::system_clock::time_point tp) const;

        // --- (9), (11), (12), (13) 독립 JSON Import / Export ---
        std::string to_json_string(int indent = 2) const;
        bool to_json_file(const std::string& file_path, int indent = 2) const;

        static std::optional<holiday_calendar> from_json_string(std::string_view json_str);
        static std::optional<holiday_calendar> from_json_file(const std::string& file_path);

        // 내부 세트 조회용 Getter
        const std::set<std::pair<int, int>>& get_recurring_holidays() const { return recurring_holidays_; }
        const std::set<std::tuple<int, int, int>>& get_specific_holidays() const { return specific_holidays_; }
        const std::set<std::pair<int, int>>& get_recurring_exclusions() const { return recurring_exclusions_; }
        const std::set<std::tuple<int, int, int>>& get_specific_exclusions() const { return specific_exclusions_; }

    private:
        time_base time_basis_;
        holiday_policy policy_;
        bool weekend_holidays_[7]; // 0: sun ~ 6: sat

        std::set<std::pair<int, int>> recurring_holidays_;         // {month, day}
        std::set<std::tuple<int, int, int>> specific_holidays_;    // {year, month, day}
        std::set<std::pair<int, int>> recurring_exclusions_;       // {month, day}
        std::set<std::tuple<int, int, int>> specific_exclusions_;  // {year, month, day}
    };

} // namespace mino::core::schedule::task
