#pragma once

#include <vector>
#include <optional>
#include <algorithm>

#include "mino/core/schedule/monthly/schedule_types.hpp"

namespace mino::core::schedule::monthly {

    class holiday_calendar {
    public:
        explicit holiday_calendar(bool exclude_sat = true, bool exclude_sun = true);

        // 단발성 공휴일 (년/월/일)
        void add_holiday(const date_ymd& d);

        // 매년 반복 공휴일 (월/일)
        void add_recurring_holiday(const date_md& md);
        void add_recurring_holiday(int month, int day);

        void clear_holidays() noexcept;

        const std::vector<date_ymd>& holidays() const noexcept { return holidays_; }
        const std::vector<date_md>& recurring_holidays() const noexcept { return recurring_holidays_; }

        void set_weekend_exclusions(bool exclude_sat, bool exclude_sun) noexcept;
        bool is_exclude_sat() const noexcept { return exclude_sat_; }
        bool is_exclude_sun() const noexcept { return exclude_sun_; }

        bool is_holiday(const date_ymd& d) const;
        bool is_business_day(const date_ymd& d) const;

        std::optional<date_ymd> adjust_by_policy(const date_ymd& scheduled, holiday_policy policy) const;

    private:
        std::vector<date_ymd> holidays_;
        std::vector<date_md> recurring_holidays_;
        bool exclude_sat_;
        bool exclude_sun_;
    };

    weekday get_weekday(const date_ymd& d) noexcept;
    int days_in_month(int year, int month) noexcept;
    date_ymd add_days(const date_ymd& d, int days) noexcept;

} // namespace mino::core::schedule::monthly
