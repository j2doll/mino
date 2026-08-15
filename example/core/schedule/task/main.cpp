#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <thread>
#include <sstream>

#include "mino/core/schedule/task/task.hpp"
#include "mino/core/string/to_console_encoding.hpp"

namespace { 
    // 시간을 읽기 쉬운 문자열로 변환하는 유틸리티 함수
    std::string format_time_point(std::chrono::system_clock::time_point tp) {
        namespace mcst = mino::core::schedule::task;
        using date_time_parts = mcst::date_time_parts;
        auto parts = date_time_parts::from_time_point(tp);

        std::ostringstream oss;
        oss << parts.year << "-"
            << std::setfill('0') << std::setw(2) << parts.month << "-"
            << std::setfill('0') << std::setw(2) << parts.day << " "
            << std::setfill('0') << std::setw(2) << parts.hour << ":"
            << std::setfill('0') << std::setw(2) << parts.minute << ":"
            << std::setfill('0') << std::setw(2) << parts.second;
        return oss.str(); // YYYY-MM-DD HH:MM:SS 형식의 문자열 반환
    }

    std::string time_point_to_string(const std::chrono::system_clock::time_point& tp) {
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        std::tm tm;
#if defined(_WIN32)
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}

// 1. 주기 전략 계산 로직 단위 테스트
void test_strategy_calculations() {
    namespace mcst = mino::core::schedule::task;
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

    auto now = std::chrono::system_clock::now(); // 현재 시각
    auto now_parts = date_time_parts::from_time_point(now); 
    print(tce("현재 시각: "), tce(format_time_point(now)),
        tce(" (요일: "), now_parts.weekday, tce(")"), endl); // 현재 시각과 요일 출력
    // 0: 일요일, 1: 월요일, 2: 화요일, 3: 수요일, 4: 목요일, 5: 금요일, 6: 토요일

    // 1-1. Minutely 전략 (매 분 30초)
    minutely_strategy min_strat(30);
    auto next_min = min_strat.get_next_run_time(now); // 현재 시간에서 가장 가까운 ●●분:30초
    print(tce("[Minutely (매분 30초)] -> "), tce(format_time_point(next_min)));

    // 1-2. Hourly 전략 (매 시 15분 00초)
    hourly_strategy hour_strat(15, 0);
    auto next_hour = hour_strat.get_next_run_time(now); // 현재 시간에서 가장 가까운 ●●시:15분:00초
    print(tce("[Hourly   (매시 15분)] -> "), tce(format_time_point(next_hour)));

    // 1-3. Daily 전략 (매일 09시 00분 00초)
    daily_strategy day_strat(9, 0, 0);
    auto next_day = day_strat.get_next_run_time(now); // 현재 시간에서 가장 가까운 ●●일 09시:00분:00초
    print(tce("[Daily    (매일 09시)] -> "), tce(format_time_point(next_day)));

    // 1-4. Weekly 전략 (매주 일요일(0) 00시 00분 00초)
    weekly_strategy week_strat(0, 0, 0, 0);
    auto next_week = week_strat.get_next_run_time(now); // 현재 시간에서 가장 가까운 일요일 00시:00분:00초
    print(tce("[Weekly   (매주 일요일)] -> "), tce(format_time_point(next_week)));

    // 1-5. Monthly 전략 (매월 1일 00시 00분)
    monthly_strategy month_strat(1, 0, 0);
    auto next_month = month_strat.get_next_run_time(now); // 현재 시간에서 가장 가까운 매월 1일 00시:00분:00초
    print(tce("[Monthly  (매월 1일)]   -> "), tce(format_time_point(next_month)), endl);
}

// 2. 실시간 스케줄러 및 워커 스레드 동작 테스트
void test_scheduler_execution() {
    namespace mcst = mino::core::schedule::task;
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

    task_scheduler scheduler; // 스케줄러

    auto now = std::chrono::system_clock::now(); // 현재 시각
    auto parts = date_time_parts::from_time_point(now);

    // 현재 초 기준 2초 뒤, 4초 뒤에 실행되도록 전략 생성
    int target_sec_1 = (parts.second + 2) % 60; // 현재 초 기준 2초 뒤
    int target_sec_2 = (parts.second + 4) % 60; // 현재 초 기준 4초 뒤

    print(tce("현재 초: "), parts.second, tce("초"));
    print(tce("Task 1 예정: "), target_sec_1, tce("초 (매 분마다 반복)"));
    print(tce("Task 2 예정: "), target_sec_2, tce("초 (중간에 취소될 예정)"), endl);

    // Task 1 등록
    uint64_t task1_id = scheduler.add_task(
        std::make_unique<minutely_strategy>(target_sec_1), // [전략] 매 분마다 target_sec_1 초에 실행
        []() { // 작업 함수
            auto tce = mino::core::string::to_console_encoding;
            auto current = std::chrono::system_clock::now();
            std::cout << tce(">>> [실행] Task 1 작업 완료! (")
                << tce(format_time_point(current)) << tce(")") << std::endl;
        },
        "태스크 1 (반복 실행)" // 설명 (생략 가능)
    );

    // Task 2 등록 (삭제 테스트용)
    uint64_t task2_id = scheduler.add_task(
        std::make_unique<minutely_strategy>(target_sec_2), // [전략] 매 분마다 target_sec_2 초에 실행
        []() { // 작업 함수
            auto tce = mino::core::string::to_console_encoding;
            auto current = std::chrono::system_clock::now();
            std::cout << tce(">>> [실행] Task 2 작업 완료! (")
                << tce(format_time_point(current)) << tce(")") << std::endl;
        },
        "태스크 2 (취소 대상)" // 설명
    );

    // 스케줄러 시작
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
                << time_point_to_string(t.next_run_time) << "\n";
        }
        std::cout << std::endl;
    }

    // Task 2 삭제 테스트 (실행되기 전 바로 제거)
    print(tce(">> Task 2(ID: "), task2_id, tce(")를 제거합니다."));
    scheduler.remove_task(task2_id);

    // 목록 조회 (Task2 제거 후)
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
                << time_point_to_string(t.next_run_time) << "\n";
        }
        std::cout << std::endl;
    }

    // Task 1이 실행될 때까지 약 5초간 대기
    print(tce(">> 약 5초 동안 스케줄러를 관찰합니다..."));
    std::this_thread::sleep_for(std::chrono::seconds(5));

    print(endl, tce(">> 스케줄러를 종료합니다."));
    scheduler.stop(); // 스케줄러 종료
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
