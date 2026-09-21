#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

#include "mino/core/schedule/task/task.hpp"
#include "mino/core/datetime/util/util.hpp" 
#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/enum/enum.hpp"

namespace {
    namespace mdtu = mino::core::datetime::util;

    // 공통 날짜/시간 포맷 상수 (초 단위)
    constexpr const char* DATETIME_FORMAT = "YYYY-MM-DD hh:mm:ss";

    // 자주 쓰는 타임존(local_time)과 포맷을 기본값으로 바인딩한 auto 람다
    auto fdt = [](
        const auto& tp,
        mdtu::time_zone_mode tz = mdtu::time_zone_mode::local_time,
        const std::string& fmt = DATETIME_FORMAT)
        {
            return mdtu::format_datetime(tp, tz, fmt);
        };
}

// 1. 주기 전략 계산 로직 단위 테스트
void test_strategy_calculations() {
    namespace mcst = mino::core::schedule::task;
    namespace mdtu = mino::core::datetime::util;

    using weekday = mcst::weekday;
    using date_time_parts = mcst::date_time_parts;
    using monthly_strategy = mcst::monthly_strategy;
    using weekly_strategy = mcst::weekly_strategy;
    using daily_strategy = mcst::daily_strategy;
    using hourly_strategy = mcst::hourly_strategy;
    using minutely_strategy = mcst::minutely_strategy;

    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce("[Test 1] 주기 전략 다음 실행 시간 계산 검증"));
    print(tce("========================================"));

    auto now = std::chrono::system_clock::now();
    auto now_parts = date_time_parts::from_time_point(now);

    // format_datetime 단축 람다(fdt)를 사용하여 Local 및 UTC 시간 출력
    print(tce("현재 시각(Local): "), tce(fdt(now)));
    print(tce("현재 시각(UTC)  : "), tce(fdt(now, mdtu::time_zone_mode::utc)));
    print(tce("요일: "), tce(std::string(mcst::to_string(now_parts.weekday))),
        tce(" ["), tce(std::string(mcst::to_short_string(now_parts.weekday))), tce("]"),
        tce(" / 코드: "), static_cast<int>(now_parts.weekday), endl);

    // 1-1. Minutely 전략 (매 분 30초)
    minutely_strategy min_strat(30);
    auto next_min = min_strat.get_next_run_time(now);
    print(tce("[Minutely (매분 30초)] -> "), tce(fdt(next_min)));

    // 1-2. Hourly 전략 (매 시 15분 00초)
    hourly_strategy hour_strat(15, 0);
    auto next_hour = hour_strat.get_next_run_time(now);
    print(tce("[Hourly   (매시 15분)] -> "), tce(fdt(next_hour)));

    // 1-3. Daily 전략 (매일 09시 00분 00초)
    daily_strategy day_strat(9, 0, 0);
    auto next_day = day_strat.get_next_run_time(now);
    print(tce("[Daily    (매일 09시)] -> "), tce(fdt(next_day)));

    // 1-4. Weekly 전략 (매주 일요일(weekday::sunday) 00시 00분 00초)
    weekly_strategy week_strat(weekday::sunday, 0, 0, 0);
    auto next_week = week_strat.get_next_run_time(now);
    print(tce("[Weekly   (매주 일요일)] -> "), tce(fdt(next_week)));

    // 1-5. Monthly 전략 (매월 1일 00시 00분)
    monthly_strategy month_strat(1, 0, 0);
    auto next_month = month_strat.get_next_run_time(now);
    print(tce("[Monthly  (매월 1일)]   -> "), tce(fdt(next_month)));

    // 1-6. 요일 문자열 파싱 검증 (대소문자/전체 이름/약어)
    print(endl, tce("--- 요일 문자열 파싱 검증 ---"));
    for (const char* name_candidate : { "Sunday", "mon", "WEDNESDAY", "fri", "unknown" }) {
        auto parsed = mcst::weekday_from_string(name_candidate);
        if (parsed.has_value()) {
            print(tce("파싱 성공: '"), name_candidate, tce("' -> "),
                tce(std::string(mcst::to_string(*parsed))),
                tce(" (약어: "), tce(std::string(mcst::to_short_string(*parsed))),
                tce(", 코드: "), static_cast<int>(*parsed), tce(")"));
        }
        else {
            print(tce("파싱 실패: '"), name_candidate, tce("'"));
        }
    }
    print(endl);
}

// 2. 실시간 스케줄러 및 워커 스레드 동작 테스트
void test_scheduler_execution() {
    namespace mcst = mino::core::schedule::task;
    namespace mdtu = mino::core::datetime::util;

    using task_scheduler = mcst::task_scheduler;
    using date_time_parts = mcst::date_time_parts;
    using minutely_strategy = mcst::minutely_strategy;

    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce("[Test 2] Task Scheduler 실시간 동작 테스트"));
    print(tce("========================================"));

    task_scheduler scheduler;

    auto now = std::chrono::system_clock::now();
    auto parts = date_time_parts::from_time_point(now);

    int target_sec_1 = (parts.second + 2) % 60;
    int target_sec_2 = (parts.second + 4) % 60;

    print(tce("현재 초: "), parts.second, tce("초"));
    print(tce("Task 1 예정: "), target_sec_1, tce("초 (매 분마다 반복)"));
    print(tce("Task 2 예정: "), target_sec_2, tce("초 (중간에 취소될 예정)"), endl);

    // Task 1 등록
    uint64_t task1_id = scheduler.add_task(
        std::make_unique<minutely_strategy>(target_sec_1),
        []() {
            auto tce = mino::core::string::to_console_encoding;
            std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
            std::cout
                << tce(">>> [실행] Task 1 작업 완료! (")
                << tce(fdt(current))
                << tce(")")
                << std::endl;
        },
        "태스크 1 (반복 실행)"
    );

    // Task 2 등록 (삭제 테스트용)
    uint64_t task2_id = scheduler.add_task(
        std::make_unique<minutely_strategy>(target_sec_2),
        []() {
            auto tce = mino::core::string::to_console_encoding;
            std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
            std::cout
                << tce(">>> [실행] Task 2 작업 완료! (")
                << tce(fdt(current))
                << tce(")")
                << std::endl;
        },
        "태스크 2 (취소 대상)"
    );

    scheduler.start();
    print(tce(">> 스케줄러가 시작되었습니다."), endl);

    // 목록 조회
    {
        auto tasks = scheduler.list_tasks();
        std::cout << "\nRegistered tasks: " << tasks.size() << "\n";
        for (const auto& t : tasks) {
            std::cout
                << "  ID: "
                << t.task_id
                << ", desc: "
                << (t.description ? tce(*t.description) : "(none)")
                << ", next_run: "
                << fdt(t.next_run_time)
                << "\n";
        }
        std::cout << std::endl;
    }

    // Task 2 삭제 테스트
    print(tce(">> Task 2(ID: "), task2_id, tce(")를 제거합니다."));
    scheduler.remove_task(task2_id);

    // 목록 조회 (Task 2 제거 후)
    {
        auto tasks = scheduler.list_tasks();
        std::cout << "\nRegistered tasks: " << tasks.size() << "\n";
        for (const auto& t : tasks) {
            std::cout
                << "  ID: "
                << t.task_id
                << ", desc: "
                << (t.description ? tce(*t.description) : "(none)")
                << ", next_run: "
                << fdt(t.next_run_time)
                << "\n";
        }
        std::cout << std::endl;
    }

    // Task 1 관찰 대기
    print(tce(">> 약 5초 동안 스케줄러를 관찰합니다..."));
    std::this_thread::sleep_for(std::chrono::seconds(5));

    print(endl, tce(">> 스케줄러를 종료합니다."));
    scheduler.stop();
    print(tce("========================================"));
    print(tce("테스트가 성공적으로 완료되었습니다."));
}

int main() {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    test_strategy_calculations();
    test_scheduler_execution();

    return 0;
}
