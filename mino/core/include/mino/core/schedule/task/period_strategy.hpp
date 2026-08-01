#pragma once

#include <chrono>
#include <ctime>

namespace mino::core::schedule::task {

    // 시간 계산 편의를 위한 구조체
    struct  date_time_parts {
        int year;    // 1900 ~
        int month;   // 1 ~ 12
        int day;     // 1 ~ 31
        int weekday; // 0 (일) ~ 6 (토)
        int hour;    // 0 ~ 23 
        int minute;  // 0 ~ 59
        int second;  // 0 ~ 59 (60 is possible for leap seconds, but we will ignore that for simplicity.)

        static date_time_parts from_time_point(std::chrono::system_clock::time_point tp);
    };

    // 주기 전략 인터페이스
    class  period_strategy {
    public:
        virtual ~period_strategy() = default;
        virtual std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) = 0;
    };

    // 매월 특정 일/시/분 전략
    class  monthly_strategy : public period_strategy {
    public:
        monthly_strategy(int day, int hour, int minute);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_day_;
        int target_hour_;
        int target_minute_;
    };

    // 매일 특정 시/분/초 전략
    class  daily_strategy : public period_strategy {
    public:
        daily_strategy(int hour, int minute, int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_hour_;
        int target_minute_;
        int target_second_;
    };

    // 매주 특정 요일/시/분/초 전략
    class  weekly_strategy : public period_strategy {
    public:
        weekly_strategy(int weekday, int hour, int minute, int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_weekday_;
        int target_hour_;
        int target_minute_;
        int target_second_;
    };

    // 매 시간 특정 분/초 전략
    class  hourly_strategy : public period_strategy {
    public:
        hourly_strategy(int minute, int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_minute_;
        int target_second_;
    };

    // 매 분 특정 초 전략
    class  minutely_strategy : public period_strategy {
    public:
        explicit minutely_strategy(int second);
        std::chrono::system_clock::time_point get_next_run_time(std::chrono::system_clock::time_point current_time) override;

    private:
        int target_second_;
    };

}  
