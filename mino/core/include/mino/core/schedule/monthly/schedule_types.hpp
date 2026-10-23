#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "mino/core/schedule/weekly/schedule_types.hpp"

namespace mino::core::schedule::monthly {

    using weekly::weekday;
    using weekly::time_hm;

    // 매년 반복되는 공휴일용 (월, 일)
    struct date_md {
        int month;  // 1 ~ 12
        int day;    // 1 ~ 31

        bool operator==(const date_md& o) const noexcept {
            return month == o.month && day == o.day;
        }
        bool operator<(const date_md& o) const noexcept {
            if (month != o.month) return month < o.month;
            return day < o.day;
        }
    };

    // 단발성 공휴일 및 일반 날짜용 (연, 월, 일)
    struct date_ymd {
        int year;   // e.g., 2026
        int month;  // 1 ~ 12
        int day;    // 1 ~ 31

        bool operator==(const date_ymd& o) const noexcept {
            return year == o.year && month == o.month && day == o.day;
        }
        bool operator!=(const date_ymd& o) const noexcept {
            return !(*this == o);
        }
        bool operator<(const date_ymd& o) const noexcept {
            if (year != o.year) return year < o.year;
            if (month != o.month) return month < o.month;
            return day < o.day;
        }
        bool operator<=(const date_ymd& o) const noexcept {
            return *this < o || *this == o;
        }
    };

    // 시:분:초 구조체
    struct time_hms {
        int hour;   // 0 ~ 23
        int minute; // 0 ~ 59
        int second; // 0 ~ 59

        bool operator==(const time_hms& o) const noexcept {
            return hour == o.hour && minute == o.minute && second == o.second;
        }
        bool operator!=(const time_hms& o) const noexcept {
            return !(*this == o);
        }
        bool operator<(const time_hms& o) const noexcept {
            if (hour != o.hour) return hour < o.hour;
            if (minute != o.minute) return minute < o.minute;
            return second < o.second;
        }
        bool operator<=(const time_hms& o) const noexcept {
            return *this < o || *this == o;
        }
    };

    // 요일 기반 주차 (1~4주차 또는 마지막 주)
    enum class week_ordinal : uint8_t {
        first = 1,
        second = 2,
        third = 3,
        fourth = 4,
        last = 5
    };

    // 스케줄 반복 모드
    enum class pattern_type : uint8_t {
        day_of_month,    // 특정 일자 (예: 매월 10일)
        ordinal_weekday  // 특정 요일/주차 (예: 2주째 목요일)
    };

    // 공휴일 충돌 시 실행 정책
    enum class holiday_policy : uint8_t {
        exact,                  // 그대로 실행
        previous_business_day,  // 이전 영업일로 이동
        next_business_day,      // 다음 영업일로 이동
        skip                    // 실행 건너뛰기
    };

    // 특정 시간대 제한 (예: 09:00:00 ~ 18:00:00)
    struct active_time_window {
        bool enabled = false;
        time_hms start_time{ 0, 0, 0 };
        time_hms end_time{ 23, 59, 59 };

        bool contains(const time_hms& t) const noexcept {
            if (!enabled) return true;
            return !(t < start_time) && (t <= end_time);
        }
    };

    // 일회성 일정 변경
    struct one_time_override {
        date_ymd target_date;       // 원래 예정 날짜
        date_ymd rescheduled_date;  // 변경된 실행 날짜
        time_hms rescheduled_time;  // 변경된 실행 시간
    };

} // namespace mino::core::schedule::monthly
