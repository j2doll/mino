#include <chrono>
#include <string>
#include <memory>

#include "mino/core/schedule/schedule.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/enum/enum.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

namespace {
    namespace mdtu = mino::core::datetime::util;
    using time_zone_mode = mdtu::time_zone_mode;

    constexpr const char* DATETIME_FORMAT = "YYYY-MM-DD hh:mm:ss";

    auto fdt = [](const auto& tp,
        time_zone_mode tz = time_zone_mode::local_time,
        const std::string& fmt = DATETIME_FORMAT)
        {
            return mdtu::format_datetime(tp, tz, fmt);
        };
}

// [Test 1] 주기 전략 다음 실행 시간 계산 검증
void test_strategy_calculations() {
    namespace mcst = mino::core::schedule::task;
    namespace mdtu = mino::core::datetime::util;
    namespace tlog = mino::core::log::tinylog;

    using weekday = mcst::weekday;
    using date_time_parts = mcst::date_time_parts;
    using time_base = mcst::time_base;

    using monthly_strategy = mcst::monthly_strategy;
    using weekly_strategy = mcst::weekly_strategy;
    using daily_strategy = mcst::daily_strategy;
    using hourly_strategy = mcst::hourly_strategy;
    using minutely_strategy = mcst::minutely_strategy;

    using time_zone_mode = mdtu::time_zone_mode;

    auto logger = tlog::logger::get("task_test");
    if (!logger) return;

    logger->info("========================================");
    logger->info("[Test 1] 주기 전략 다음 실행 시간 계산 검증");
    logger->info("========================================");

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    date_time_parts now_parts = date_time_parts::from_time_point(now, time_base::localtime);

    logger->info("현재 시각(Local): <bright_green>{}</bright_green>", fdt(now));
    logger->info("현재 시각(UTC)  : <bright_blue>{}</bright_blue>", fdt(now, time_zone_mode::utc));

    logger->info("요일: <bright_yellow>{}</bright_yellow> [<gray>{}</gray>] / 코드: <gray>{}</gray>",
        mcst::to_string(now_parts.weekday),
        mcst::to_short_string(now_parts.weekday),
        static_cast<int>(now_parts.weekday));

    // 1-1. Minutely 전략 (매 분 30초)
    minutely_strategy min_strat(30);
    auto next_min = min_strat.get_next_run_time(now);
    logger->info("[Minutely (매분 30초)] -> <green>{}</green>", fdt(next_min));

    // 1-2. Hourly 전략 (매 시 15분 00초)
    hourly_strategy hour_strat(15, 0);
    auto next_hour = hour_strat.get_next_run_time(now);
    logger->info("[Hourly   (매시 15분)] -> <green>{}</green>", fdt(next_hour));

    // 1-3. Daily 전략 (매일 09시 00분 00초)
    daily_strategy day_strat(9, 0, 0);
    auto next_day = day_strat.get_next_run_time(now);
    logger->info("[Daily    (매일 09시)] -> <green>{}</green>", fdt(next_day));

    // 1-4. Weekly 전략 (매주 일요일 00시 00분 00초)
    weekly_strategy week_strat(weekday::sunday, 0, 0, 0);
    auto next_week = week_strat.get_next_run_time(now);
    logger->info("[Weekly   (매주 일요일)] -> <green>{}</green>", fdt(next_week));

    // 1-5. Monthly 전략 (매월 1일 00시 00분)
    monthly_strategy month_strat(1, 0, 0);
    auto next_month = month_strat.get_next_run_time(now);
    logger->info("[Monthly  (매월 1일)]   -> <green>{}</green>", fdt(next_month));

    monthly_strategy month_last_day(-1, 0, 0);
    auto next_month_last = month_last_day.get_next_run_time(now);
    logger->info("[Monthly  (매월 말일)(Local)] -> <green>{}</green>", fdt(next_month_last));
    logger->info("[Monthly  (매월 말일)(UTC)] -> <green>{}</green>", fdt(next_month_last, time_zone_mode::utc));

    // 1-6. 요일 문자열 파싱 검증 (대소문자/전체 이름/약어)
    logger->info("--- 요일 문자열 파싱 검증 ---");
    for (const char* name_candidate : { "Sunday", "mon", "WEDNESDAY", "fri", "unknown" }) {
        auto parsed = mcst::weekday_from_string(name_candidate);
        if (parsed.has_value()) {
            logger->info("파싱 성공: <green>'{}'</green> -> <bright_cyan>{}</bright_cyan> (약어: <gray>{}</gray>, 코드: <gray>{}</gray>)",
                name_candidate,
                mcst::to_string(*parsed),
                mcst::to_short_string(*parsed),
                static_cast<int>(*parsed));
        }
        else {
            logger->warn("파싱 실패: <red>'{}'</red>", name_candidate);
        }
    }
}
