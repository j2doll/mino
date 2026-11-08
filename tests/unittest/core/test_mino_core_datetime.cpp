#include "catch_amalgamated.hpp"

#include <chrono>
#include <ctime>
#include <optional>
#include <string>

#include "mino/core/datetime/datetime.hpp"

// ----------------------------------------------------------------------------
// 1. unit::time_span 단위 테스트
// ----------------------------------------------------------------------------
TEST_CASE("unit::time_span basic operations", "[time_span]") {
    using mino::core::datetime::unit::time_span;

    SECTION("Factory methods and total conversions") {
        auto ts_days = time_span::from_days(2);
        CHECK(ts_days.total_secs() == 2 * 24 * 3600);
        CHECK(ts_days.total_msecs() == 2LL * 24 * 3600 * 1000);

        auto ts_hours = time_span::from_hours(5);
        CHECK(ts_hours.total_secs() == 5 * 3600);

        auto ts_minutes = time_span::from_minutes(30);
        CHECK(ts_minutes.total_secs() == 30 * 60);

        auto ts_seconds = time_span::from_seconds(45);
        CHECK(ts_seconds.total_secs() == 45);
        CHECK(ts_seconds.total_msecs() == 45000);

        auto ts_ms = time_span::from_msecs(1234);
        CHECK(ts_ms.total_msecs() == 1234);
        CHECK(ts_ms.total_secs() == 1);
        CHECK(ts_ms.duration() == std::chrono::milliseconds(1234));
    }

    SECTION("Equality comparison") {
        auto ts1 = time_span::from_seconds(60);
        auto ts2 = time_span::from_minutes(1);
        auto ts3 = time_span::from_msecs(60000);
        auto ts4 = time_span::from_seconds(59);

        CHECK(ts1 == ts2);
        CHECK(ts1 == ts3);
        CHECK_FALSE(ts1 == ts4);
    }
}

// ----------------------------------------------------------------------------
// 2. unit::date_time 기본 생성 및 변환 테스트
// ----------------------------------------------------------------------------
TEST_CASE("unit::date_time basic creation and conversions", "[date_time]") {
    using namespace mino::core::datetime::unit;

    SECTION("from_datetime and getters (UTC)") {
        auto dt_opt = date_time::from_datetime(2025, 10, 17, 14, 30, 45, 123, time_zone::utc);
        REQUIRE(dt_opt.has_value());

        const auto& dt = *dt_opt;
        CHECK(dt.is_utc());
        CHECK(dt.spec() == time_zone::utc);
        CHECK(dt.year() == 2025);
        CHECK(dt.month() == 10);
        CHECK(dt.day() == 17);
        CHECK(dt.hour() == 14);
        CHECK(dt.minute() == 30);
        CHECK(dt.second() == 45);
        CHECK(dt.msec() == 123);

        // 2025-10-17은 금요일 (tm_wday = 5)
        CHECK(dt.day_of_week() == 5);
        CHECK(dt.day_name(false) == "Friday");
        CHECK(dt.day_name(true) == "Fri");
    }

    SECTION("from_epoch_msecs and to_epoch_msecs") {
        long long epoch_ms = 1729175445123LL;
        auto dt_opt = date_time::from_epoch_msecs(epoch_ms, time_zone::utc);
        REQUIRE(dt_opt.has_value());
        CHECK(dt_opt->to_epoch_msecs() == epoch_ms);
        CHECK(dt_opt->to_epoch_msecs() / 1000LL == dt_opt->to_time_t());
    }

    SECTION("from_tm null check") {
        CHECK_FALSE(date_time::from_tm(nullptr, time_zone::utc).has_value());
    }

    SECTION("Timezone conversions: to_utc and to_local_time") {
        auto dt_utc_opt = date_time::from_datetime(2025, 1, 1, 12, 0, 0, 0, time_zone::utc);
        REQUIRE(dt_utc_opt.has_value());

        auto dt_local = dt_utc_opt->to_local_time();
        CHECK_FALSE(dt_local.is_utc());
        CHECK(dt_local.spec() == time_zone::local_time);
        CHECK(dt_local.to_epoch_msecs() == dt_utc_opt->to_epoch_msecs());

        auto dt_utc_back = dt_local.to_utc();
        CHECK(dt_utc_back.is_utc());
        CHECK(dt_utc_back == *dt_utc_opt);
    }
}

// ----------------------------------------------------------------------------
// 3. unit::date_time 날짜 연산 및 기간 연산자 테스트
// ----------------------------------------------------------------------------
TEST_CASE("unit::date_time arithmetic and differences", "[date_time]") {
    using namespace mino::core::datetime::unit;

    auto dt_opt = date_time::from_datetime(2025, 1, 1, 0, 0, 0, 0, time_zone::utc);
    REQUIRE(dt_opt.has_value());
    const auto dt = *dt_opt;

    SECTION("add_msecs, add_secs, add_minutes, add_hours, add_days") {
        auto dt_ms = dt.add_msecs(500);
        CHECK(dt_ms.msec() == 500);
        CHECK(dt_ms.to_epoch_msecs() - dt.to_epoch_msecs() == 500);

        auto dt_sec = dt.add_secs(10);
        CHECK(dt_sec.second() == 10);

        auto dt_min = dt.add_minutes(15);
        CHECK(dt_min.minute() == 15);

        auto dt_hr = dt.add_hours(3);
        CHECK(dt_hr.hour() == 3);

        auto dt_day = dt.add_days(2);
        CHECK(dt_day.day() == 3);
    }

    SECTION("add_months and add_years (calendar adjustments)") {
        auto dt_m = dt.add_months(2);
        CHECK(dt_m.year() == 2025);
        CHECK(dt_m.month() == 3);
        CHECK(dt_m.day() == 1);

        auto dt_y = dt.add_years(1);
        CHECK(dt_y.year() == 2026);
        CHECK(dt_y.month() == 1);
        CHECK(dt_y.day() == 1);
    }

    SECTION("Difference and span operators") {
        auto dt2 = dt.add_hours(2);
        time_span span = dt2 - dt;
        CHECK(span.total_secs() == 7200);

        time_span span2 = dt.secs_to(dt2);
        CHECK(span2.total_secs() == 7200);

        auto dt_restored = dt + span;
        CHECK(dt_restored == dt2);

        auto dt_subtracted = dt2 - span;
        CHECK(dt_subtracted == dt);
    }
}

// ----------------------------------------------------------------------------
// 4. unit::date_time 문자열 변환 및 UTC 오프셋 테스트
// ----------------------------------------------------------------------------
TEST_CASE("unit::date_time to_string and UTC offsets", "[date_time]") {
    using namespace mino::core::datetime::unit;

    SECTION("to_string with standard format") {
        auto dt_opt = date_time::from_datetime(2025, 12, 25, 8, 30, 0, 0, time_zone::utc);
        REQUIRE(dt_opt.has_value());
        CHECK(dt_opt->to_string("%Y-%m-%d %H:%M:%S") == "2025-12-25 08:30:00");
    }

    SECTION("UTC offset methods consistency") {
        long long off_sec = date_time::utc_offset_seconds();
        long long off_min = date_time::utc_offset_minutes();
        int off_hr = date_time::utc_offset_hours();

        CHECK(off_sec / 60LL == off_min);
        CHECK(off_min / 60LL == off_hr);

        std::string str_short = date_time::utc_offset_string(true);
        std::string str_long = date_time::utc_offset_string(false);
        CHECK_FALSE(str_short.empty());
        CHECK_FALSE(str_long.empty());
    }
}

// ----------------------------------------------------------------------------
// 5. util::datetime_convert 함수군 테스트
// ----------------------------------------------------------------------------
TEST_CASE("util::datetime_convert functions", "[util][convert]") {
    using mino::core::datetime::util::time_zone_mode;
    using mino::core::datetime::util::to_time_t;
    using mino::core::datetime::util::to_tm;
    using mino::core::datetime::util::to_timepoint;
    using mino::core::datetime::util::to_timepoint_opt;

    SECTION("to_time_t and to_tm round trip (UTC)") {
        std::tm tm_in{};
        tm_in.tm_year = 2025 - 1900;
        tm_in.tm_mon = 5 - 1; // 5월
        tm_in.tm_mday = 10;
        tm_in.tm_hour = 11;
        tm_in.tm_min = 22;
        tm_in.tm_sec = 33;

        auto t_opt = to_time_t(tm_in, time_zone_mode::utc);
        REQUIRE(t_opt.has_value());

        auto tm_out = to_tm(*t_opt, time_zone_mode::utc);
        REQUIRE(tm_out.has_value());
        CHECK(tm_out->tm_year == tm_in.tm_year);
        CHECK(tm_out->tm_mon == tm_in.tm_mon);
        CHECK(tm_out->tm_mday == tm_in.tm_mday);
        CHECK(tm_out->tm_hour == tm_in.tm_hour);
        CHECK(tm_out->tm_min == tm_in.tm_min);
        CHECK(tm_out->tm_sec == tm_in.tm_sec);
    }

    SECTION("to_timepoint_opt and to_timepoint") {
        std::tm tm_in{};
        tm_in.tm_year = 2025 - 1900;
        tm_in.tm_mon = 0;
        tm_in.tm_mday = 1;
        tm_in.tm_hour = 0;
        tm_in.tm_min = 0;
        tm_in.tm_sec = 0;

        auto tp_opt = to_timepoint_opt(tm_in, time_zone_mode::utc);
        REQUIRE(tp_opt.has_value());

        auto tp = to_timepoint(tm_in, time_zone_mode::utc);
        CHECK(tp == *tp_opt);

        std::tm out_tm{};
        bool ok = to_tm(tp, time_zone_mode::utc, out_tm);
        CHECK(ok);
        CHECK(out_tm.tm_year == tm_in.tm_year);
        CHECK(out_tm.tm_mday == 1);
    }
}

// ----------------------------------------------------------------------------
// 6. util::datetime_util 유틸리티 헬퍼 테스트
// ----------------------------------------------------------------------------
TEST_CASE("util::datetime_util helper functions", "[util][helper]") {
    using namespace mino::core::datetime::util;

    SECTION("is_leap") {
        CHECK(is_leap(2000));
        CHECK(is_leap(2024));
        CHECK_FALSE(is_leap(1900));
        CHECK_FALSE(is_leap(2023));
        CHECK_FALSE(is_leap(2025));
    }

    SECTION("valid_ymd") {
        CHECK(valid_ymd(2024, 2, 29));
        CHECK_FALSE(valid_ymd(2023, 2, 29));
        CHECK(valid_ymd(2025, 1, 31));
        CHECK_FALSE(valid_ymd(2025, 4, 31));
        CHECK_FALSE(valid_ymd(2025, 13, 1));
        CHECK_FALSE(valid_ymd(2025, 0, 1));
        CHECK_FALSE(valid_ymd(2025, 1, 0));
        CHECK_FALSE(valid_ymd(2025, 1, 32));
    }

    SECTION("read_ndigits and match_token") {
        std::string s = "2025-10-17";
        size_t pos = 0;
        int val = 0;
        CHECK(read_ndigits(s, pos, 4, val));
        CHECK(val == 2025);
        CHECK(pos == 4);

        CHECK(match_token("YYYY-MM-DD", 0, "YYYY"));
        CHECK(match_token("YYYY-MM-DD", 5, "MM"));
        CHECK_FALSE(match_token("YYYY-MM-DD", 0, "MM"));
    }

    SECTION("get_weekday") {
        // 2025-10-17: 금요일
        CHECK(get_weekday(2025, 10, 17, time_zone_mode::utc) == weekday::friday);
        // 2025-10-19: 일요일
        CHECK(get_weekday(2025, 10, 19, time_zone_mode::utc) == weekday::sunday);
        // 1900년 이전 지원 불가
        CHECK(get_weekday(1900, 1, 1, time_zone_mode::utc) == weekday::no_statement);
        // 유효하지 않은 일자
        CHECK(get_weekday(2025, 2, 29, time_zone_mode::utc) == weekday::no_statement);
    }

    SECTION("format_datetime with format tokens") {
        std::tm tmv{};
        tmv.tm_year = 2025 - 1900;
        tmv.tm_mon = 9;  // 10월
        tmv.tm_mday = 17;
        tmv.tm_hour = 9;
        tmv.tm_min = 5;
        tmv.tm_sec = 7;

        std::string formatted = format_datetime(tmv, "YYYY/MM/DD hh:mm:ss.SSS");
        CHECK(formatted == "2025/10/17 09:05:07.000");

        // epoch_ms 포맷
        uint64_t epoch_ms = 1000ULL * 1729155907ULL + 456ULL;
        std::string fmt_epoch = format_datetime(epoch_ms, "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::utc);
        CHECK(fmt_epoch.rfind(".456") != std::string::npos);
    }
}

// ----------------------------------------------------------------------------
// 7. util::datetime_string 파서 테스트 (Strict, ISO-8601, RFC-3339, Auto)
// ----------------------------------------------------------------------------
TEST_CASE("util::datetime_string strict parser", "[util][parser][strict]") {
    using namespace mino::core::datetime::util;

    SECTION("Strict parsing exact format") {
        auto res = parse_strict_datetime("2025-10-17 15:30:45.123", "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::utc);
        REQUIRE(res.ok);
        CHECK(res.millisecond == 123);
        CHECK(res.present.has_year);
        CHECK(res.present.has_month);
        CHECK(res.present.has_day);
        CHECK(res.present.has_hour);
        CHECK(res.present.has_minute);
        CHECK(res.present.has_second);
        CHECK(res.present.has_millisecond);

        auto tm_opt = to_tm(res.epoch, time_zone_mode::utc);
        REQUIRE(tm_opt.has_value());
        CHECK(tm_opt->tm_year + 1900 == 2025);
        CHECK(tm_opt->tm_mon + 1 == 10);
        CHECK(tm_opt->tm_mday == 17);
        CHECK(tm_opt->tm_hour == 15);
        CHECK(tm_opt->tm_min == 30);
        CHECK(tm_opt->tm_sec == 45);
    }

    SECTION("Strict parsing with base tm") {
        std::tm base{};
        base.tm_year = 2025 - 1900;
        base.tm_mon = 5;
        base.tm_mday = 20;

        auto res = parse_strict_datetime_with_base("10:20:30", "hh:mm:ss", time_zone_mode::utc, base);
        REQUIRE(res.ok);
        REQUIRE(res.broken.has_value());
        CHECK(res.broken->tm_year + 1900 == 2025);
        CHECK(res.broken->tm_mon == 5);
        CHECK(res.broken->tm_mday == 20);
        CHECK(res.broken->tm_hour == 10);
        CHECK(res.broken->tm_min == 20);
        CHECK(res.broken->tm_sec == 30);
    }

    SECTION("Strict parsing error cases") {
        auto res1 = parse_strict_datetime("2025-13-01", "YYYY-MM-DD", time_zone_mode::utc);
        CHECK_FALSE(res1.ok);

        auto res2 = parse_strict_datetime("2025-10-17 Extra", "YYYY-MM-DD", time_zone_mode::utc);
        CHECK_FALSE(res2.ok);

        auto res3 = parse_strict_datetime("Mismatch", "YYYY-MM-DD", time_zone_mode::utc);
        CHECK_FALSE(res3.ok);
    }

    SECTION("parse_strict_datetime_epoch_ms") {
        auto epoch_ms = parse_strict_datetime_epoch_ms("2025-01-01 00:00:00.500", "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::utc);
        REQUIRE(epoch_ms.has_value());
        CHECK(*epoch_ms % 1000 == 500);
    }
}

TEST_CASE("util::datetime_string ISO-8601 parser", "[util][parser][iso8601]") {
    using namespace mino::core::datetime::util;

    SECTION("Full ISO-8601 string with UTC indicator 'Z'") {
        auto res = parse_iso8601_datetime("2025-10-17T09:30:05.123Z");
        REQUIRE(res.ok);
        CHECK(res.millisecond == 123);

        auto tm_opt = to_tm(res.epoch, time_zone_mode::utc);
        REQUIRE(tm_opt.has_value());
        CHECK(tm_opt->tm_year + 1900 == 2025);
        CHECK(tm_opt->tm_mon + 1 == 10);
        CHECK(tm_opt->tm_mday == 17);
        CHECK(tm_opt->tm_hour == 9);
        CHECK(tm_opt->tm_min == 30);
        CHECK(tm_opt->tm_sec == 5);
    }

    SECTION("ISO-8601 string with timezone offset") {
        // 2025-10-17 09:30:00 +09:00 -> UTC 00:30:00
        auto res = parse_iso8601_datetime("2025-10-17T09:30:00+09:00");
        REQUIRE(res.ok);

        auto tm_opt = to_tm(res.epoch, time_zone_mode::utc);
        REQUIRE(tm_opt.has_value());
        CHECK(tm_opt->tm_hour == 0);
        CHECK(tm_opt->tm_min == 30);
        CHECK(tm_opt->tm_sec == 0);
    }

    SECTION("Invalid ISO-8601 input") {
        auto res = parse_iso8601_datetime("invalid-string");
        CHECK_FALSE(res.ok);
    }
}

TEST_CASE("util::datetime_string RFC-3339 parser", "[util][parser][rfc3339]") {
    using namespace mino::core::datetime::util;

    SECTION("Valid RFC-3339 with 'T' and timezone offset") {
        auto res = parse_rfc3339_datetime("2025-10-17T12:00:00-05:00");
        REQUIRE(res.ok);

        // UTC 기준 17:00:00
        auto tm_opt = to_tm(res.epoch, time_zone_mode::utc);
        REQUIRE(tm_opt.has_value());
        CHECK(tm_opt->tm_hour == 17);
        CHECK(tm_opt->tm_min == 0);
        CHECK(tm_opt->tm_sec == 0);
    }

    SECTION("Valid RFC-3339 with space separator and 'Z'") {
        auto res = parse_rfc3339_datetime("2025-10-17 12:00:00.250Z");
        REQUIRE(res.ok);
        CHECK(res.millisecond == 250);

        auto tm_opt = to_tm(res.epoch, time_zone_mode::utc);
        REQUIRE(tm_opt.has_value());
        CHECK(tm_opt->tm_hour == 12);
    }

    SECTION("RFC-3339 rejects invalid format") {
        // 콜론 없는 타임존 표기 (+0900은 ISO8601 허용이지만 RFC3339 불허)
        auto res = parse_rfc3339_datetime("2025-10-17T09:30:00+0900");
        CHECK_FALSE(res.ok);

        // 잘못된 날짜
        auto res_date = parse_rfc3339_datetime("2025-02-29T12:00:00Z");
        CHECK_FALSE(res_date.ok);
    }
}

TEST_CASE("util::datetime_string parse_datetime_auto and timepoint", "[util][parser][auto]") {
    using namespace mino::core::datetime::util;

    SECTION("Auto detection of ISO8601 literal") {
        auto res = parse_datetime_auto("2025-10-17T00:30:05.123Z", "ISO8601");
        CHECK(res.ok);
        CHECK(res.millisecond == 123);
    }

    SECTION("Auto detection of RFC3339 literal") {
        auto res = parse_datetime_auto("2025-10-17T00:30:05.123Z", "RFC-3339");
        CHECK(res.ok);
        CHECK(res.millisecond == 123);
    }

    SECTION("Auto fallback to strict format") {
        auto res = parse_datetime_auto("2025-10-17", "YYYY-MM-DD", time_zone_mode::utc);
        CHECK(res.ok);
    }

    SECTION("parse_datetime_timepoint") {
        auto tp_opt = parse_datetime_timepoint("2025-10-17T00:00:00Z", "ISO8601");
        REQUIRE(tp_opt.has_value());
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(tp_opt->time_since_epoch()).count();
        CHECK(sec > 0);
    }

    SECTION("current_time_string produces expected pattern") {
        std::string cur = current_time_string(time_zone_mode::utc, "YYYY-MM-DD hh:mm:ss.SSS");
        CHECK(cur.size() == 23);
        CHECK(cur[4] == '-');
        CHECK(cur[7] == '-');
        CHECK(cur[10] == ' ');
        CHECK(cur[13] == ':');
        CHECK(cur[16] == ':');
        CHECK(cur[19] == '.');
    }
}
