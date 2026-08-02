#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <string>

namespace mino::core::datetime {
    namespace unit {

    // 시간 표기 기준: 로컬 시간인지 UTC인지 명시합니다.
    enum class time_zone {
        utc, // UTC 기준
        local_time // 로컬 타임존 기준 (예: KST)
    };

    // time_span 클래스는 기간을 나타내며, 내부적으로 밀리초 단위로 저장됩니다.
    class  time_span {
    public:
        // 주어진 밀리초 기간으로 time_span 객체를 생성합니다.
        explicit time_span(std::chrono::milliseconds ms);

        // 일/시간/분/초/밀리초 단위의 기간을 생성하는 팩토리 메서드들입니다.
        static time_span from_days(long long d);    // d일을 밀리초로 변환하여 생성
        static time_span from_hours(long long h);   // h시간을 밀리초로 변환하여 생성
        static time_span from_minutes(long long m); // m분을 밀리초로 변환하여 생성
        static time_span from_seconds(long long s); // s초를 밀리초로 변환하여 생성
        static time_span from_msecs(long long ms);  // ms밀리초로 생성

        // 전체 기간을 밀리초(64비트)로 반환합니다.
        long long total_msecs() const;

        // 전체 기간을 초(64비트)로 반환합니다(소수점 이하 무시).
        long long total_secs() const;

        // 내부에 저장된 std::chrono::milliseconds duration을 반환합니다.
        std::chrono::milliseconds duration() const;

        // 두 time_span이 동일한 기간인지 비교합니다.
        bool operator==(const time_span& other) const;

    private:
        // 내부에 저장된 총 밀리초 값입니다.
        std::chrono::milliseconds total_ms_;
    };
    

    // date_time 클래스는 시스템 클럭과 밀리초 정밀도의 time_point를 기반으로 날짜와 시간을 나타냅니다.
    class  date_time {
    public:
        // 시스템 클럭 및 밀리초 정밀도의 time_point 타입 정의입니다.
        using clock = std::chrono::system_clock; // UTC 기준 시스템 클럭
        using time_point = std::chrono::time_point<clock, std::chrono::milliseconds>;

        // NOTE:
        //   time_zone은 생성시 한번 설정되며 이후 변경되지 않습니다.
        //   변경을 원하면, 다른 date_time 객체를 새로 생성해야 합니다.
        date_time(time_zone zone = time_zone::local_time);

        // 주어진 time_point와 time_zone(로컬/UTC)으로 초기화합니다.
        date_time(time_point tp, time_zone zone = time_zone::local_time);

        // 현재 시스템 시각을 date_time으로 반환합니다.
        static date_time current_date_time();

        // Import / Export
        // 주어진 time_point와 spec으로 date_time을 생성합니다.
        static date_time from_time_point(time_point tp, time_zone spec = time_zone::local_time);

        // time_t(초 단위)를 date_time으로 변환합니다(지정된 spec 기준).
        static date_time from_time_t(std::time_t t, time_zone spec = time_zone::local_time);

        // std::tm 구조체로부터 date_time을 생성합니다(지정된 spec 기준).
        static date_time from_tm(std::tm* tm_ptr, time_zone spec = time_zone::local_time);

        // 새로 추가: epoch 기준 밀리초(1970-01-01 UTC)에서 date_time을 생성합니다.
        static date_time from_epoch_msecs(long long epoch_msecs, time_zone spec = time_zone::local_time);

        // 내부 time_point를 반환합니다.
        time_point to_time_point() const;

        // time_t(초 단위)로 변환하여 반환합니다.
        std::time_t to_time_t() const;

        // std::tm 구조체 형태로 변환한 캘린더 구성요소를 반환합니다.
        std::tm to_tm() const;

        // 새로 추가: epoch 기준 밀리초(64비트)를 반환합니다.
        long long to_epoch_msecs() const;

        // Timezone and offset
        // UTC로 변환된 date_time을 반환합니다 (spec을 utc로 설정).
        date_time to_utc() const;

        // 로컬 시간으로 변환된 date_time을 반환합니다 (spec을 local_time으로 설정).
        date_time to_local_time() const;

        // 로컬 타임존의 UTC 오프셋을 초/분/시간 단위로 반환합니다.
        static long long utc_offset_seconds(); // 오프셋을 초 단위로 반환 (예: +3600)
        static long long utc_offset_minutes(); // 오프셋을 분 단위로 반환
        static int utc_offset_hours();         // 오프셋을 시간 단위로 반환 (정수)

        // 오프셋을 문자열로 반환합니다. short_format=true면 축약형 반환.
        static std::string utc_offset_string(bool short_format = false);

        // spec 접근자: 현재 객체가 로컬인지 UTC인지 반환합니다.
        time_zone spec() const; // utc 또는 local_time

        // true면 UTC, false면 로컬 시간입니다.
        bool is_utc() const; // true: utc, false: local_time 

        // Day of week
        // 요일을 정수로 반환합니다. 0=Sunday, 6=Saturday
        int day_of_week() const; // 0=Sunday, 6=Saturday

        // 요일 이름을 반환합니다. short_name=true면 축약형("Sun") 반환.
        std::string day_name(bool short_name = false) const; // "Sunday" or "Sun"(short_name=true)

        // Calendar components
        // 연/월/일/시/분/초/밀리초 구성요소를 반환합니다.
        int year() const;
        int month() const;   // 1..12
        int day() const;     // 1..31
        int hour() const;    // 0..23
        int minute() const;  // 0..59
        int second() const;  // 0..60 (윤초 가능성 포함)
        int msec() const;    // 0..999 (현재 초 내 밀리초)

        // Formatting and arithmetic
        // 포맷 문자열을 사용하여 문자열로 변환합니다.
        // 기본 포맷: "%Y-%m-%d %H:%M:%S" (%Y:year, %m: month, %d: day, %H: hour, %M: minute, %S: second)
        std::string to_string(const std::string& format = "%Y-%m-%d %H:%M:%S") const;

        // 지정한 기간을 더한 새로운 date_time을 반환합니다.
        date_time add_msecs(long long ms) const;
        date_time add_secs(long long s) const;
        date_time add_minutes(long long m) const;
        date_time add_hours(long long h) const;
        date_time add_days(long long d) const;

        // 월/년 단위로 달력을 고려하여 조정한 새로운 date_time을 반환합니다.
        date_time add_months(int months) const; // 월 단위 조정 (예: 1월 + 1개월 = 2월)
        date_time add_years(int years) const;   // 년 단위 조정

        // this에서 other까지의 차이를 time_span으로 반환합니다.
        time_span secs_to(const date_time& other) const;

        // subtract two date_time values -> time_span
        // 두 date_time의 차이를 time_span으로 반환합니다 (this - other).
        time_span operator-(const date_time& other) const;

        // time_span을 더하거나 빼어 새로운 date_time을 반환합니다.
        date_time operator+(const time_span& span) const;
        date_time operator-(const time_span& span) const;

        // 동일한 시점을 나타내는지 비교합니다(타임스탬프와 spec이 같을 때).
        bool operator==(const date_time& other) const;

    private:
        // 내부적으로 밀리초 정밀도의 time_point를 저장합니다.
        time_point tp_;

        // 이 객체의 시간 기준(로컬 또는 UTC)입니다.
        time_zone zone_;

        // 캘린더 구성요소를 직접 조정하기 위한 유틸리티 템플릿입니다.
        // 내부 동작: to_tm()으로 std::tm 얻고 f를 적용한 뒤 mktime/timegm으로 변환하여
        // 밀리초 잔여(msec remainder)를 합쳐 새로운 date_time을 반환합니다.
        template<typename Func>
        date_time adjust_calendar(Func f) const;
    };

    template<typename Func>
    date_time date_time::adjust_calendar(Func f) const {
        std::tm parts = to_tm();
        f(parts);
        std::time_t new_t_c;
    #if defined(_WIN32) || defined(_WIN64)
        new_t_c = (zone_ == time_zone::utc) ? _mkgmtime(&parts) : mktime(&parts);
    #else
        new_t_c = (zone_ == time_zone::utc) ? timegm(&parts) : mktime(&parts);
    #endif
        auto new_tp = clock::from_time_t(new_t_c);
        auto msec_remainder = tp_.time_since_epoch() % std::chrono::seconds(1);
        auto ret = date_time(std::chrono::time_point_cast<std::chrono::milliseconds>(new_tp) + msec_remainder, zone_);
        return ret;
    }
    // Note: implementations for utc_offset_seconds(), utc_offset_minutes(), utc_offset_hours()
    // and utc_offset_string() live in `unit/date_time.cpp` to avoid duplicate-definition (ODR) issues.


    } // namespace unit
} // namespace mino::core::datetime
