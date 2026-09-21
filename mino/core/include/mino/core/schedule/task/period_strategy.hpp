#pragma once

#include <chrono>
#include <ctime>
#include <string_view>
#include <optional>

#include "mino/core/enum/enum.hpp"

// task 네임스페이스에 weekday 열거형 정의 (std::tm::tm_wday: 0: 일요일 ~ 6: 토요일 체계)
DEFINE_ENUM_NAMESPACE(mino::core::schedule::task, weekday,
    sunday = 0,
    monday,
    tuesday,
    wednesday,
    thursday,
    friday,
    saturday
)

// 시간 변환 기준 열거형 (로컬 타임, UTC)
DEFINE_ENUM_NAMESPACE(mino::core::schedule::task, time_base,
    localtime,
    utc
)

namespace mino::core::schedule::task {

    // --- 요일 문자열 변환 유틸리티 ---

    // 1. 전체 요일 문자열 반환 (smart_enum의 enum_name 활용: "sunday", "monday", ...)
    constexpr std::string_view to_string(weekday wd) {
        return mino::core::enums::enum_name(wd);
    }

    // 2. 짧은 요일 문자열 반환 ("sun", "mon", ...)
    constexpr std::string_view to_short_string(weekday wd) {
        switch (wd) {
        case weekday::sunday:    return "sun";
        case weekday::monday:    return "mon";
        case weekday::tuesday:   return "tue";
        case weekday::wednesday: return "wed";
        case weekday::thursday:  return "thu";
        case weekday::friday:    return "fri";
        case weekday::saturday:  return "sat";
        }
        return "";
    }

    // 3. 문자열로부터 요일 파싱 (대소문자 무시, 전체 이름 및 약어 지원)
    std::optional<weekday> weekday_from_string(std::string_view str);

    // 시간 계산 편의를 위한 구조체
    struct date_time_parts {
        int year;              // 1900 ~
        int month;             // 1 ~ 12
        int day;               // 1 ~ 31
        task::weekday weekday; // 멤버 변수 이름 충돌 방지를 위해 네임스페이스 한정자 명시
        int hour;              // 0 ~ 23 
        int minute;            // 0 ~ 59
        int second;            // 0 ~ 59

        // 타임포인트를 date_time_parts로 변환 (기본값: 로컬 타임)
        static date_time_parts from_time_point(
            std::chrono::system_clock::time_point tp,
            time_base base = time_base::localtime
        );
    };

    // 주기 전략 인터페이스
    class period_strategy {
    public:
        virtual ~period_strategy() = default;
        virtual std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) = 0;
    };

    // 매월 특정 일/시/분 전략
    class monthly_strategy : public period_strategy {
    public:
        monthly_strategy(int day, int hour, int minute);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_day_;
        int target_hour_;
        int target_minute_;
    };

    // 매일 특정 시/분/초 전략
    class daily_strategy : public period_strategy {
    public:
        daily_strategy(int hour, int minute, int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_hour_;
        int target_minute_;
        int target_second_;
    };

    // 매주 특정 요일/시/분/초 전략
    class weekly_strategy : public period_strategy {
    public:
        weekly_strategy(weekday wd, int hour, int minute, int second);
        weekly_strategy(int weekday_val, int hour, int minute, int second);

        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        weekday target_weekday_;
        int target_hour_;
        int target_minute_;
        int target_second_;
    };

    // 매 시간 특정 분/초 전략
    class hourly_strategy : public period_strategy {
    public:
        hourly_strategy(int minute, int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_minute_;
        int target_second_;
    };

    // 매 분 특정 초 전략
    class minutely_strategy : public period_strategy {
    public:
        explicit minutely_strategy(int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_second_;
    };

}
