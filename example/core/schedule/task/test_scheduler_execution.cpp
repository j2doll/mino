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
    auto fdt = [](const auto& tp, time_zone_mode tz = time_zone_mode::local_time, const std::string& fmt = DATETIME_FORMAT) {
        return mdtu::format_datetime(tp, tz, fmt);
        };
}

namespace {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;
    auto tcev = [](std::string_view sv) { return mino::core::string::to_console_encoding(std::string(sv)); };
}


// 2. 실시간 스케줄러 및 워커 스레드 동작 테스트
void test_scheduler_execution() {
    namespace mcst = mino::core::schedule::task;
    namespace mdtu = mino::core::datetime::util;

    using task_scheduler = mcst::task_scheduler;
    using date_time_parts = mcst::date_time_parts;

    using monthly_strategy = mcst::monthly_strategy; // 매월 특정 일/시/분 전략
    using daily_strategy = mcst::daily_strategy; // 매일 특정 시/분/초 전략
    using weekly_strategy = mcst::weekly_strategy; // 매주 특정 요일/시/분/초 전략
    using hourly_strategy = mcst::hourly_strategy; // 매 시간 특정 분/초 전략
    using minutely_strategy = mcst::minutely_strategy; // 매 분 특정 초 전략

    print(tce("========================================"));
    print(tce("[Test 2] Task Scheduler 실시간 동작 테스트"));
    print(tce("========================================"));

    task_scheduler scheduler;

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    date_time_parts parts = date_time_parts::from_time_point(now);

    int target_sec_1 = (parts.second + 2) % 60;
    int target_sec_2 = (parts.second + 4) % 60;

    print(tce("현재 초: "), parts.second, tce("초"));
    print(tce("Task 1 예정: "), target_sec_1, tce("초 (매 분마다 반복)"));
    print(tce("Task 2 예정: "), target_sec_2, tce("초 (중간에 취소될 예정)"), endl);

    // Task 1 등록
    uint64_t task1_id = scheduler.add_task(
        std::make_unique<minutely_strategy>(target_sec_1), // 매 분마다 target_sec_1 초에 실행
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
        std::make_unique<minutely_strategy>(target_sec_2), // 매 분마다 target_sec_2 초에 실행
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

    scheduler.start(); // 스케줄러 시작
    print(tce(">> 스케줄러가 시작되었습니다."), endl);

    // 목록 조회
    {
        auto tasks = scheduler.list_tasks(); // 현재 등록된 task 목록 조회
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
    scheduler.remove_task(task2_id); // task2_id 삭제

    // 목록 조회 (Task 2 제거 후)
    {
        auto tasks = scheduler.list_tasks(); // 현재 등록된 task 목록 조회
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
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Task 1이 여러 번 실행되는 것을 관찰

    print(endl, tce(">> 스케줄러를 종료합니다."));
    scheduler.stop(); // 스케줄러 종료
    print(tce("========================================"));
    print(tce("테스트가 성공적으로 완료되었습니다."));
}


