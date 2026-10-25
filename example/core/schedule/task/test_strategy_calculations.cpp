#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

#include "mino/core/schedule/schedule.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/enum/enum.hpp"

namespace {
    namespace mdtu = mino::core::datetime::util;
    using time_zone_mode = mdtu::time_zone_mode;

    // 공통 날짜/시간 포맷 상수 (초 단위)
    constexpr const char* DATETIME_FORMAT = "YYYY-MM-DD hh:mm:ss";

    // 자주 쓰는 타임존(local_time)과 포맷을 기본값으로 바인딩한 auto 람다
    auto fdt = [](const auto& tp,
        time_zone_mode tz = time_zone_mode::local_time, // 기본은 로컬타임의 문자열로 표시
        const std::string& fmt = DATETIME_FORMAT) // 년-월-일 시:분:초 포맷
    { return mdtu::format_datetime(tp, tz, fmt); };
}

namespace {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;
    auto tcev = [](std::string_view sv) { return mino::core::string::to_console_encoding(std::string(sv)); };
}

// 1. 주기 전략 계산 로직 단위 테스트
void test_strategy_calculations() {
    namespace mcst = mino::core::schedule::task;
    namespace mdtu = mino::core::datetime::util;
    // namespace mcsw = mino::core::schedule::weekly;

    using weekday = mcst::weekday;
    using date_time_parts = mcst::date_time_parts;
    using time_base = mcst::time_base;

    using monthly_strategy = mcst::monthly_strategy;
    using weekly_strategy = mcst::weekly_strategy;
    using daily_strategy = mcst::daily_strategy;
    using hourly_strategy = mcst::hourly_strategy;
    using minutely_strategy = mcst::minutely_strategy;

    using time_zone_mode = mdtu::time_zone_mode;


    print(tce("========================================"));
    print(tce("[Test 1] 주기 전략 다음 실행 시간 계산 검증"));
    print(tce("========================================"));

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    date_time_parts now_parts = date_time_parts::from_time_point(now, time_base::localtime); // 로컬타임 기준의 현재 시각 정보

    // format_datetime 단축 람다(fdt)를 사용하여 Local 및 UTC 시간 출력
    print(tce("현재 시각(Local): "), tce(fdt(now))); // 로컬타임 기준의 현재 시각 문자열
    print(tce("현재 시각(UTC)  : "), tce(fdt(now, time_zone_mode::utc))); // UTC 기준의 현재 시각 문자열

    print(tce("요일: "), tcev(mcst::to_string(now_parts.weekday)), // 로컬타임 기준의 요일 문자열 
        tce(" ["), tcev(mcst::to_short_string(now_parts.weekday)), tce("]"), // 요일 약어 문자열
        tce(" / 코드: "), static_cast<int>(now_parts.weekday), endl); // 요일 코드 (0:일요일 ~ 6:토요일)

    // 1-1. Minutely 전략 (매 분 30초)
    minutely_strategy min_strat(30); // 매 분 30초에 실행
    auto next_min = min_strat.get_next_run_time(now); // 다음 실행 시간 계산
    print(tce("[Minutely (매분 30초)] -> "), tce(fdt(next_min)));

    // 1-2. Hourly 전략 (매 시 15분 00초)
    hourly_strategy hour_strat(15, 0); // 매 시 15분 00초에 실행
    auto next_hour = hour_strat.get_next_run_time(now); // 다음 실행 시간 계산
    print(tce("[Hourly   (매시 15분)] -> "), tce(fdt(next_hour)));

    // 1-3. Daily 전략 (매일 09시 00분 00초)
    daily_strategy day_strat(9, 0, 0); // 매일 09시 00분 00초에 실행
    auto next_day = day_strat.get_next_run_time(now); // 다음 실행 시간 계산
    print(tce("[Daily    (매일 09시)] -> "), tce(fdt(next_day)));

    // 1-4. Weekly 전략 (매주 일요일(weekday::sunday) 00시 00분 00초)
    weekly_strategy week_strat(weekday::sunday, 0, 0, 0); // 매주 일요일 00시 00분 00초에 실행
    auto next_week = week_strat.get_next_run_time(now); // 다음 실행 시간 계산
    print(tce("[Weekly   (매주 일요일)] -> "), tce(fdt(next_week)));

    // 1-5. Monthly 전략 (매월 1일 00시 00분)
    monthly_strategy month_strat(1, 0, 0); // 매월 1일 00시 00분에 실행
    auto next_month = month_strat.get_next_run_time(now); // 다음 실행 시간 계산
    print(tce("[Monthly  (매월 1일)]   -> "), tce(fdt(next_month)));

    monthly_strategy month_last_day(-1, 0, 0); // 매월 말일 00시 00분에 실행.
    // 1월:31일, 2월:28(평년)/29일(윤년), 3월:31일, 4월:30일, 5월:31일, 6월:30일,
    // 7월:31일, 8월:31일, 9월:30일, 10월:31일, 11월:30일, 12월:31일
    auto next_month_last = month_last_day.get_next_run_time(now); // 다음 실행 시간 (time_point)
    print(tce("[Monthly  (매월 말일)(Local)] -> "), tce(fdt(next_month_last)));
    print(tce("[Monthly  (매월 말일)(UTC)] -> "), tce(fdt(next_month_last, time_zone_mode::utc)));

    // 1-6. 요일 문자열 파싱 검증 (대소문자/전체 이름/약어)
    print(endl, tce("--- 요일 문자열 파싱 검증 ---"));
    for (const char* name_candidate : { "Sunday", "mon", "WEDNESDAY", "fri", "unknown" }) {
        auto parsed = mcst::weekday_from_string(name_candidate); // 요일 문자열 파싱 시도
        if (parsed.has_value()) {
            print(tce("파싱 성공: '"), name_candidate,
                tce("' -> "), tcev(mcst::to_string(*parsed)),
                tce(" (약어: "), tcev(mcst::to_short_string(*parsed)),
                tce(", 코드: "), static_cast<int>(*parsed), tce(")"));
        }
        else {
            print(tce("파싱 실패: '"), name_candidate, tce("'"));
        }
    }
    print(endl);
}
