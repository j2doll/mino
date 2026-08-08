#include <iostream>
#include <cassert>
#include <ctime>
#include <chrono>
#include <string>

#include "mino/core/datetime/unit/date_time.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// -------------------------------------------------------------
// 1. time_span 클래스의 모든 퍼블릭 멤버 테스트
// -------------------------------------------------------------
void test_time_span_all() {
    namespace dtunit = mino::core::datetime::unit;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test 1/4] time_span 모든 퍼블릭 멤버 검증 중...\n");

    // Constructor: explicit time_span(std::chrono::milliseconds ms)
    dtunit::time_span ts_raw(std::chrono::milliseconds(5000));
    assert(ts_raw.total_msecs() == 5000);

    // total_msecs() & total_secs()
    dtunit::time_span ts_ms = dtunit::time_span::from_msecs(1000);
    assert(ts_ms.total_msecs() == 1000);
    assert(ts_ms.total_secs() == 1); // 1000밀리초 = 1초

    dtunit::time_span ts_sec = dtunit::time_span::from_seconds(60);
    assert(ts_sec.total_secs() == 60); // 60초 = 1분

    dtunit::time_span ts_min = dtunit::time_span::from_minutes(60);
    assert(ts_min.total_secs() == 3600); // 60분 = 3600초

    dtunit::time_span ts_hr = dtunit::time_span::from_hours(24);
    assert(ts_hr.total_secs() == 86400); // 24시간 = 86400초

    dtunit::time_span ts_day = dtunit::time_span::from_days(1);
    assert(ts_day.total_secs() == 86400); // 1일 = 24시간 = 86400초

    // duration() 
    assert(ts_sec.duration() == std::chrono::milliseconds(60000)); // 60초 = 60000밀리초

    // operator== 
    assert(ts_hr == ts_day);
    assert(!(ts_ms == ts_sec));

    // date_time과 time_span 연산 테스트
    auto same_timezone = dtunit::time_zone::local_time;
    // NOTICE: span을 하기 전에 가능하면 두 시간의 타임존을 통일시킨다.
    // 타임존이 다른 date_time의 span도 계산은 가능하지만, 결과가 직관적이지 않을 수 있다.

    dtunit::date_time dt1 = dtunit::date_time::from_datetime(2026, 1, 1, 0, 0, 0, 0, same_timezone).value(); // 2026-01-01 00:00:00.000
    dtunit::date_time dt2 = dtunit::date_time::from_datetime(2026, 1, 2, 0, 0, 0, 0, same_timezone).value(); // 2026-01-02 00:00:00.000

    dtunit::time_span span_diff1 = dt2 - dt1; // dt1.secs_to(dt2)와 동일
    dtunit::time_span span_diff2 = dt1 - dt2; // dt2.secs_to(dt1)와 동일

    assert(span_diff1.total_secs() == 86400); // 1일(day) 차이 = 86400초
    assert(span_diff2.total_secs() == -86400); // -1일(day) 차이 = -86400초

    std::cout << to_console_encoding("  -> time_span 검증 완료!\n\n");
}

// -------------------------------------------------------------
// 2. date_time 생성자 및 팩토리 메서드 테스트 
// -------------------------------------------------------------
void test_date_time_constructors_and_factories() {
    namespace dtunit = mino::core::datetime::unit;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test 2/4] date_time 생성자 및 팩토리 메서드 검증 중...\n");

    // date_time(time_zone zone = time_zone::local_time) 
    dtunit::date_time dt_default;
    dtunit::date_time dt_utc(dtunit::time_zone::utc);
    assert(!dt_default.is_utc());
    assert(dt_utc.is_utc());

    // date_time(time_point tp, time_zone zone) 
    auto now_tp = std::chrono::time_point_cast<std::chrono::milliseconds>(dtunit::date_time::clock::now());
    dtunit::date_time dt_from_tp_ctor(now_tp, dtunit::time_zone::local_time);

    // current_date_time() 
    dtunit::date_time dt_curr = dtunit::date_time::current_date_time();
    assert(dt_curr.year() >= 2026);

    // from_time_point 
    dtunit::date_time dt_fp = dtunit::date_time::from_time_point(now_tp, dtunit::time_zone::utc);
    assert(dt_fp.is_utc());

    // from_time_t 
    std::time_t t_now = std::time(nullptr);
    dtunit::date_time dt_ft = dtunit::date_time::from_time_t(t_now, dtunit::time_zone::local_time);
    assert(dt_ft.to_time_t() == t_now);

    // from_tm 
    std::tm sample_tm = {};
    sample_tm.tm_year = 126; // 2026년 (1900 + 126)
    sample_tm.tm_mon = 7;    // 8월 (0-indexed)
    sample_tm.tm_mday = 15;
    sample_tm.tm_hour = 12;
    sample_tm.tm_min = 30;
    sample_tm.tm_sec = 0;
    {
        auto odt_ftm = dtunit::date_time::from_tm(&sample_tm, dtunit::time_zone::local_time);
        assert(odt_ftm.has_value());
        dtunit::date_time dt_ftm = *odt_ftm;
        assert(dt_ftm.year() == 2026);
        assert(dt_ftm.month() == 8);
        assert(dt_ftm.day() == 15);
    }

    // from_epoch_msecs 
    long long target_epoch = 1700000000000LL; // 샘플 epoch ms
    std::optional<dtunit::date_time> dt_epoch = dtunit::date_time::from_epoch_msecs(target_epoch, dtunit::time_zone::utc);
    assert(dt_epoch.has_value());
    assert(dt_epoch->to_epoch_msecs() == target_epoch);

    std::cout << to_console_encoding("  -> 생성자 및 팩토리 메서드 검증 완료!\n\n");
}

// -------------------------------------------------------------
// 3. date_time 변환, 오프셋, 구성 요소 액세서 테스트 
// -------------------------------------------------------------
void test_date_time_conversions_and_accessors() {
    namespace dtunit = mino::core::datetime::unit;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test 3/4] date_time 변환 및 구성 요소 액세서 검증 중...\n");

    dtunit::date_time dt_local = dtunit::date_time::current_date_time();
    // to_time_point, to_time_t, to_tm, to_epoch_msecs 
    dtunit::date_time::time_point tp = dt_local.to_time_point();
    std::time_t t = dt_local.to_time_t();
    std::tm tm_struct = dt_local.to_tm();
    long long epoch_ms = dt_local.to_epoch_msecs();
    (void)tp; (void)t; (void)tm_struct; (void)epoch_ms; // 미사용 경고 방지

    // to_utc() & to_local_time() 
    dtunit::date_time dt_utc = dt_local.to_utc();
    dtunit::date_time dt_back_local = dt_utc.to_local_time();
    assert(dt_utc.is_utc());
    assert(!dt_back_local.is_utc());

    // spec() & is_utc() 
    assert(dt_local.spec() == dtunit::time_zone::local_time);
    assert(dt_utc.spec() == dtunit::time_zone::utc);

    // 정적 오프셋 함수들: utc_offset_seconds, minutes, hours, string 
    long long off_sec = dtunit::date_time::utc_offset_seconds();
    long long off_min = dtunit::date_time::utc_offset_minutes();
    int off_hr = dtunit::date_time::utc_offset_hours();
    std::string off_str_long = dtunit::date_time::utc_offset_string(false);
    std::string off_str_short = dtunit::date_time::utc_offset_string(true);

    assert(off_min == off_sec / 60);
    assert(off_hr == static_cast<int>(off_min / 60));
    assert(!off_str_long.empty());
    assert(!off_str_short.empty());

    // 날짜/시간 성분 액세서: year, month, day, hour, minute, second, msec
    assert(dt_local.year() >= 1970);
    assert(dt_local.month() >= 1 && dt_local.month() <= 12);
    assert(dt_local.day() >= 1 && dt_local.day() <= 31);
    assert(dt_local.hour() >= 0 && dt_local.hour() <= 23);
    assert(dt_local.minute() >= 0 && dt_local.minute() <= 59);
    assert(dt_local.second() >= 0 && dt_local.second() <= 60);
    assert(dt_local.msec() >= 0 && dt_local.msec() <= 999);

    // day_of_week() & day_name() 
    int dow = dt_local.day_of_week();
    assert(dow >= 0 && dow <= 6);
    std::string name_full = dt_local.day_name(false);
    std::string name_short = dt_local.day_name(true);
    assert(!name_full.empty() && !name_short.empty());

    // to_string()  
    std::string str_default = dt_local.to_string();
    std::string str_custom = dt_local.to_string("%Y/%m/%d");
    assert(!str_default.empty());
    assert(str_custom.length() == 10);

    std::cout << to_console_encoding("  -> 변환 및 구성 요소 액세서 검증 완료!\n\n");
}

// -------------------------------------------------------------
// 4. date_time 연산 메서드 및 오버로딩 연산자 테스트 
// -------------------------------------------------------------
void test_date_time_arithmetic_and_operators() {
    namespace dtunit = mino::core::datetime::unit;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test 4/4] date_time 연산 및 오버로딩 연산자 검증 중...\n");

    // 특정 일시로 기준점 생성 (2026-01-01 10:00:00.500)
    std::tm base_tm = {};
    base_tm.tm_year = 126;
    base_tm.tm_mon = 0;
    base_tm.tm_mday = 1;
    base_tm.tm_hour = 10;
    base_tm.tm_min = 0;
    base_tm.tm_sec = 0;
    {
        auto obase = dtunit::date_time::from_tm(&base_tm, dtunit::time_zone::local_time);
        assert(obase.has_value());
        dtunit::date_time base = obase->add_msecs(500);

        // add_msecs, add_secs, add_minutes, add_hours, add_days 
        assert(base.add_msecs(500).msec() == 0);
        assert(base.add_secs(30).second() == 30);
        assert(base.add_minutes(30).minute() == 30);
        assert(base.add_hours(2).hour() == 12);
        assert(base.add_days(5).day() == 6);

        // add_months, add_years 
        assert(base.add_months(2).month() == 3);
        assert(base.add_years(1).year() == 2027);

        // secs_to 
        dtunit::date_time future = base.add_secs(100);
        dtunit::time_span span_diff = base.secs_to(future);
        assert(span_diff.total_secs() == 100);

        // operator- (date_time - date_time) -> time_span
        dtunit::time_span op_minus_dt = future - base;
        assert(op_minus_dt.total_secs() == 100);

        // operator+ (date_time + time_span) -> date_time
        dtunit::time_span add_span = dtunit::time_span::from_hours(5);
        dtunit::date_time plus_result = base + add_span;
        assert(plus_result.hour() == 15);

        // operator- (date_time - time_span) -> date_time
        dtunit::date_time minus_result = plus_result - add_span;
        assert(minus_result == base);

        // operator==
        assert(base == base);
        assert(!(base == future));
    }

    std::cout << to_console_encoding("  -> 연산 및 오버로딩 연산자 검증 완료!\n\n");
}

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("===========================================\n");
    std::cout << to_console_encoding(" mino::core::datetime::unit 테스트\n");
    std::cout << to_console_encoding("===========================================\n\n");

    try {
        test_time_span_all();
        test_date_time_constructors_and_factories();
        test_date_time_conversions_and_accessors();
        test_date_time_arithmetic_and_operators();

        std::cout << to_console_encoding("===========================================\n");
        std::cout << to_console_encoding(" 모든 퍼블릭 멤버 테스트 통과!\n");
        std::cout << to_console_encoding("===========================================\n");
    }
    catch (const std::exception& e) {
        std::cerr << to_console_encoding("테스트 수행 중 예외 발생: ") << e.what() << "\n";
        return 1;
    }

    return 0;
}
