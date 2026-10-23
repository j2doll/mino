#pragma once

#include <string>
#include <vector>
#include "mino/core/schedule/monthly/schedule_types.hpp"

namespace mino::core::schedule::monthly {

    struct monthly_rule {
        std::string id;

        pattern_type pattern = pattern_type::day_of_month;
        int day_of_month = 1;
        weekday target_weekday = weekday::mon;
        week_ordinal target_ordinal = week_ordinal::first;

        std::vector<int> target_months; // 비어있으면 모든 월, {1}이면 매년 1월
        int interval_months = 1;        // n개월 주기 (1=매월, 3=분기)
        int anchor_year = 2026;
        int anchor_month = 1;

        time_hms execution_time{ 0, 0, 0 };
        holiday_policy policy = holiday_policy::exact;
        active_time_window window;

        std::vector<date_ymd> excluded_dates;
        std::vector<one_time_override> overrides;

        bool is_month_applicable(int year, int month) const noexcept {
            if (!target_months.empty()) {
                bool found = false;
                for (int m : target_months) {
                    if (m == month) { found = true; break; }
                }
                if (!found) return false;
            }
            if (interval_months > 1) {
                int diff = (year - anchor_year) * 12 + (month - anchor_month);
                if (diff < 0 || (diff % interval_months != 0)) {
                    return false;
                }
            }
            return true;
        }
    };

} // namespace mino::core::schedule::monthly
