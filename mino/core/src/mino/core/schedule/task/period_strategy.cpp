#include <cctype>
#include <algorithm>

#include "mino/core/schedule/task/period_strategy.hpp"

namespace mino::core::schedule::task {

    std::optional<weekday> weekday_from_string(std::string_view str) {
        if (str.empty()) return std::nullopt;

        char lower_buf[16];
        if (str.size() >= sizeof(lower_buf)) return std::nullopt;

        for (size_t i = 0; i < str.size(); ++i) {
            lower_buf[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
        }
        std::string_view lower(lower_buf, str.size());

        // 1. smart_enum 리플렉션을 통한 전체 이름 매칭 ("sunday", "monday", ...)
        auto full_match = mino::core::enums::enum_cast<weekday>(lower);
        if (full_match.has_value()) {
            return full_match;
        }

        // 2. 약어 이름 매칭 ("sun", "mon", ...)
        if (lower == "sun") return weekday::sunday;
        if (lower == "mon") return weekday::monday;
        if (lower == "tue") return weekday::tuesday;
        if (lower == "wed") return weekday::wednesday;
        if (lower == "thu") return weekday::thursday;
        if (lower == "fri") return weekday::friday;
        if (lower == "sat") return weekday::saturday;

        return std::nullopt;
    }

    date_time_parts date_time_parts::from_time_point(std::chrono::system_clock::time_point tp, time_base base) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_info{};

#if defined(_WIN32) || defined(_WIN64)
        if (base == time_base::utc) {
            gmtime_s(&tm_info, &t);
        }
        else {
            localtime_s(&tm_info, &t);
        }
#else
        if (base == time_base::utc) {
            gmtime_r(&t, &tm_info);
        }
        else {
            localtime_r(&t, &tm_info);
        }
#endif

        return {
            tm_info.tm_year + 1900,
            tm_info.tm_mon + 1,
            tm_info.tm_mday,
            static_cast<task::weekday>(tm_info.tm_wday),
            tm_info.tm_hour,
            tm_info.tm_min,
            tm_info.tm_sec
        };
    }

    // Monthly
    monthly_strategy::monthly_strategy(int day, int hour, int minute)
        : target_day_(day), target_hour_(hour), target_minute_(minute) {
    }

    std::chrono::system_clock::time_point monthly_strategy::get_next_run_time(std::chrono::system_clock::time_point current_time) {
        auto parts = date_time_parts::from_time_point(current_time);
        std::tm target_tm{};
        target_tm.tm_year = parts.year - 1900;
        target_tm.tm_mon = parts.month - 1;
        target_tm.tm_mday = target_day_;
        target_tm.tm_hour = target_hour_;
        target_tm.tm_min = target_minute_;
        target_tm.tm_sec = 0;

        std::time_t target_t = std::mktime(&target_tm);
        auto target_tp = std::chrono::system_clock::from_time_t(target_t);

        if (target_tp <= current_time) {
            target_tm.tm_mon += 1;
            target_t = std::mktime(&target_tm);
            target_tp = std::chrono::system_clock::from_time_t(target_t);
        }
        return target_tp;
    }

    // Daily
    daily_strategy::daily_strategy(int hour, int minute, int second)
        : target_hour_(hour), target_minute_(minute), target_second_(second) {
    }

    std::chrono::system_clock::time_point daily_strategy::get_next_run_time(std::chrono::system_clock::time_point current_time) {
        auto parts = date_time_parts::from_time_point(current_time);
        std::tm target_tm{};
        target_tm.tm_year = parts.year - 1900;
        target_tm.tm_mon = parts.month - 1;
        target_tm.tm_mday = parts.day;
        target_tm.tm_hour = target_hour_;
        target_tm.tm_min = target_minute_;
        target_tm.tm_sec = target_second_;

        std::time_t target_t = std::mktime(&target_tm);
        auto target_tp = std::chrono::system_clock::from_time_t(target_t);

        if (target_tp <= current_time) {
            target_tp += std::chrono::hours(24);
        }
        return target_tp;
    }

    // Weekly
    weekly_strategy::weekly_strategy(weekday wd, int hour, int minute, int second)
        : target_weekday_(wd), target_hour_(hour), target_minute_(minute), target_second_(second) {
    }

    weekly_strategy::weekly_strategy(int weekday_val, int hour, int minute, int second)
        : weekly_strategy(static_cast<task::weekday>(weekday_val), hour, minute, second) {
    }

    std::chrono::system_clock::time_point weekly_strategy::get_next_run_time(std::chrono::system_clock::time_point current_time) {
        auto parts = date_time_parts::from_time_point(current_time);
        std::tm target_tm{};
        target_tm.tm_year = parts.year - 1900;
        target_tm.tm_mon = parts.month - 1;
        target_tm.tm_mday = parts.day;
        target_tm.tm_hour = target_hour_;
        target_tm.tm_min = target_minute_;
        target_tm.tm_sec = target_second_;

        std::time_t target_t = std::mktime(&target_tm);
        auto target_tp = std::chrono::system_clock::from_time_t(target_t);

        int days_diff = static_cast<int>(target_weekday_) - static_cast<int>(parts.weekday);
        if (days_diff < 0 || (days_diff == 0 && target_tp <= current_time)) {
            days_diff += 7;
        }
        target_tp += std::chrono::hours(24 * days_diff);
        return target_tp;
    }

    // Hourly
    hourly_strategy::hourly_strategy(int minute, int second)
        : target_minute_(minute), target_second_(second) {
    }

    std::chrono::system_clock::time_point hourly_strategy::get_next_run_time(std::chrono::system_clock::time_point current_time) {
        auto parts = date_time_parts::from_time_point(current_time);
        std::tm target_tm{};
        target_tm.tm_year = parts.year - 1900;
        target_tm.tm_mon = parts.month - 1;
        target_tm.tm_mday = parts.day;
        target_tm.tm_hour = parts.hour;
        target_tm.tm_min = target_minute_;
        target_tm.tm_sec = target_second_;

        std::time_t target_t = std::mktime(&target_tm);
        auto target_tp = std::chrono::system_clock::from_time_t(target_t);

        if (target_tp <= current_time) {
            target_tp += std::chrono::hours(1);
        }
        return target_tp;
    }

    // Minutely
    minutely_strategy::minutely_strategy(int second) : target_second_(second) {}

    std::chrono::system_clock::time_point minutely_strategy::get_next_run_time(std::chrono::system_clock::time_point current_time) {
        auto parts = date_time_parts::from_time_point(current_time);
        std::tm target_tm{};
        target_tm.tm_year = parts.year - 1900;
        target_tm.tm_mon = parts.month - 1;
        target_tm.tm_mday = parts.day;
        target_tm.tm_hour = parts.hour;
        target_tm.tm_min = parts.minute;
        target_tm.tm_sec = target_second_;

        std::time_t target_t = std::mktime(&target_tm);
        auto target_tp = std::chrono::system_clock::from_time_t(target_t);

        if (target_tp <= current_time) {
            target_tp += std::chrono::minutes(1);
        }
        return target_tp;
    }

}
