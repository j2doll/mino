#include "mino/core/schedule/task/period_strategy.hpp"

namespace mino::core::schedule::task {

    date_time_parts date_time_parts::from_time_point(std::chrono::system_clock::time_point tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_info{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_info, &t);
#else
        localtime_r(&t, &tm_info);
#endif
        return {
            tm_info.tm_year + 1900,
            tm_info.tm_mon + 1,
            tm_info.tm_mday,
            tm_info.tm_wday,
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
    weekly_strategy::weekly_strategy(int weekday, int hour, int minute, int second)
        : target_weekday_(weekday), target_hour_(hour), target_minute_(minute), target_second_(second) {
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

        int days_diff = target_weekday_ - parts.weekday;
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
