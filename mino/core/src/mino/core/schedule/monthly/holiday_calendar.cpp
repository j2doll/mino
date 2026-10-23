#include "mino/core/schedule/monthly/holiday_calendar.hpp"

namespace mino::core::schedule::monthly {

    static bool is_leap_year(int year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int days_in_month(int year, int month) noexcept {
        if (month < 1 || month > 12) return 30;
        constexpr int dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2 && is_leap_year(year)) return 29;
        return dim[month - 1];
    }

    weekday get_weekday(const date_ymd& d) noexcept {
        static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
        int y = d.year;
        if (d.month < 3) y -= 1;
        int w = (y + y / 4 - y / 100 + y / 400 + t[d.month - 1] + d.day) % 7;
        int mw = (w + 6) % 7;
        return static_cast<weekday>(mw);
    }

    date_ymd add_days(const date_ymd& d, int days) noexcept {
        date_ymd cur = d;
        if (days > 0) {
            while (days > 0) {
                int max_d = days_in_month(cur.year, cur.month);
                int rem = max_d - cur.day;
                if (days <= rem) {
                    cur.day += days;
                    break;
                }
                days -= (rem + 1);
                cur.day = 1;
                cur.month += 1;
                if (cur.month > 12) {
                    cur.month = 1;
                    cur.year += 1;
                }
            }
        }
        else if (days < 0) {
            int to_sub = -days;
            while (to_sub > 0) {
                if (cur.day > to_sub) {
                    cur.day -= to_sub;
                    break;
                }
                to_sub -= cur.day;
                cur.month -= 1;
                if (cur.month < 1) {
                    cur.month = 12;
                    cur.year -= 1;
                }
                cur.day = days_in_month(cur.year, cur.month);
            }
        }
        return cur;
    }

    holiday_calendar::holiday_calendar(bool exclude_sat, bool exclude_sun)
        : exclude_sat_(exclude_sat), exclude_sun_(exclude_sun) {
    }

    void holiday_calendar::add_holiday(const date_ymd& d) {
        if (!std::binary_search(holidays_.begin(), holidays_.end(), d)) {
            holidays_.push_back(d);
            std::sort(holidays_.begin(), holidays_.end());
        }
    }

    void holiday_calendar::add_recurring_holiday(const date_md& md) {
        if (!std::binary_search(recurring_holidays_.begin(), recurring_holidays_.end(), md)) {
            recurring_holidays_.push_back(md);
            std::sort(recurring_holidays_.begin(), recurring_holidays_.end());
        }
    }

    void holiday_calendar::add_recurring_holiday(int month, int day) {
        add_recurring_holiday(date_md{ month, day });
    }

    void holiday_calendar::clear_holidays() noexcept {
        holidays_.clear();
        recurring_holidays_.clear();
    }

    void holiday_calendar::set_weekend_exclusions(bool exclude_sat, bool exclude_sun) noexcept {
        exclude_sat_ = exclude_sat;
        exclude_sun_ = exclude_sun;
    }

    bool holiday_calendar::is_holiday(const date_ymd& d) const {
        if (std::binary_search(holidays_.begin(), holidays_.end(), d)) {
            return true;
        }
        date_md md{ d.month, d.day };
        if (std::binary_search(recurring_holidays_.begin(), recurring_holidays_.end(), md)) {
            return true;
        }
        return false;
    }

    bool holiday_calendar::is_business_day(const date_ymd& d) const {
        weekday wd = get_weekday(d);
        if (exclude_sat_ && wd == weekday::sat) return false;
        if (exclude_sun_ && wd == weekday::sun) return false;
        return !is_holiday(d);
    }

    std::optional<date_ymd> holiday_calendar::adjust_by_policy(const date_ymd& scheduled, holiday_policy policy) const {
        if (policy == holiday_policy::exact) {
            return scheduled;
        }

        if (is_business_day(scheduled)) {
            return scheduled;
        }

        if (policy == holiday_policy::skip) {
            return std::nullopt;
        }

        date_ymd cur = scheduled;
        int step = (policy == holiday_policy::previous_business_day) ? -1 : 1;

        for (int i = 0; i < 31; ++i) {
            cur = add_days(cur, step);
            if (is_business_day(cur)) {
                return cur;
            }
        }

        return std::nullopt;
    }

} // namespace mino::core::schedule::monthly
