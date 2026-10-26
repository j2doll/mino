#include <cassert>
#include <thread>
#include <chrono>
#include <memory>
#include <cstdio>
#include <filesystem>

#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/core/schedule/task/task.hpp"

using namespace mino::core::schedule::task;
using namespace mino::core::log::tinylog;

// 3-1. 공휴일 캘린더 단독 기능 검증 (주말, 반복, 1회성, 제외일 우선순위)
void test_holiday_calendar_rules(std::shared_ptr<logger> log) {
    log->info("<bold><cyan>[Test 3-1] Testing Holiday Calendar Rules...</cyan></bold>");

    holiday_calendar cal; // 공휴일 캘린더 생성 (기본: localtime 기준, skip(공휴일이면 수행 안함) 정책)

    // (1) 주말 설정: 토, 일은 기본적으로 휴무
    assert(cal.is_weekend_holiday(weekday::saturday) == true);
    assert(cal.is_weekend_holiday(weekday::sunday) == true);

    // 토요일을 휴무에서 해제 테스트
    cal.set_weekend_holiday(weekday::saturday, false); // 토요일을 휴무에서 해제
    assert(cal.is_weekend_holiday(weekday::saturday) == false);

    // (2) 반복 공휴일: 12월 25일 크리스마스
    cal.add_recurring_holiday(12, 25);

    // (3) 1회성 공휴일: 2026년 5월 6일 임시공휴일
    cal.add_specific_holiday(2026, 5, 6);

    // 2026-12-25 (금요일) -> 반복 공휴일이므로 true
    assert(cal.is_holiday(2026, 12, 25, weekday::friday) == true);

    // 2027-12-25 (토요일) -> 반복 공휴일이므로 true
    assert(cal.is_holiday(2027, 12, 25, weekday::saturday) == true);

    // 2026-05-06 (수요일) -> 1회성 공휴일이므로 true
    assert(cal.is_holiday(2026, 5, 6, weekday::wednesday) == true);

    // 2027-05-06 (목요일) -> 공휴일이 아니므로 false
    assert(cal.is_holiday(2027, 5, 6, weekday::thursday) == false);

    // (4) 1회성 제외: 2026-12-25를 특별 근무일로 지정
    cal.add_specific_exclusion(2026, 12, 25);

    assert(cal.is_holiday(2026, 12, 25, weekday::friday) == false); // 공휴일 아님: 1회성 제외 적용 확인
    assert(cal.is_holiday(2027, 12, 25, weekday::saturday) == true); // 2027년은 여전히 공휴일

    // (5) 반복 제외: 매년 5월 1일 근로자의 날이 휴무여도 제외일로 등록하면 평일로 판정
    cal.add_recurring_holiday(5, 1); // 매년 5월 1일을 공휴일로 등록
    cal.add_recurring_exclusion(5, 1); // 매년 5월 1일을 제외일로 등록
    assert(cal.is_holiday(2026, 5, 1, weekday::friday) == false); // 5월 1일은 최종 공휴일이 아님

    log->info("<green>[Test 1] Passed!</green>");
}

// 3-2. 공휴일 JSON 파일/문자열 Import/Export 검증
void test_holiday_json_import_export(std::shared_ptr<logger> log) {
    log->info("<bold><cyan>[Test 3-2] Testing Holiday JSON Import/Export...</cyan></bold>");

    holiday_calendar cal(time_base::localtime, holiday_policy::shift_to_next); // (A) 시간 기준: localtime, 정책: shift_to_next(공휴일이면 이후 평일에 수행)

    cal.set_weekend_holiday(weekday::saturday, true); // (B) 토요일을 휴무로 설정
    cal.set_weekend_holiday(weekday::sunday, true); // (C) 일요일을 휴무로 설정

    cal.add_recurring_holiday(1, 1); // (D) 매년 1월 1일을 공휴일로 등록

    cal.add_specific_holiday(2026, 8, 17); // (E) 2026년 8월 17일을 임시 공휴일로 등록

    cal.add_specific_exclusion(2026, 1, 1); // (F) 2026년 1월 1일을 제외일로 등록 (공휴일 아님)

    // JSON 문자열 export
    std::string json_str = cal.to_json_string(2);
    assert(!json_str.empty());
    log->debug("Exported Holiday JSON string:\n{}", json_str);

    // JSON 문자열 import
    auto cal_from_str = holiday_calendar::from_json_string(json_str);
    assert(cal_from_str.has_value());
    assert(cal_from_str->get_policy() == holiday_policy::shift_to_next); // 정책 확인: (A) shift_to_next
    assert(cal_from_str->get_time_base() == time_base::localtime); // 시간 기준 확인: (A) localtime 
    assert(cal_from_str->is_holiday(2026, 8, 17, weekday::monday) == true); // 임시 공휴일 확인: (E) 2026-08-17
    assert(cal_from_str->is_holiday(2026, 1, 1, weekday::thursday) == false); // 제외일 적용 확인: (F) 2026-01-01

    // 파일 export 및 import
    const std::string test_file = "test_holiday.json";
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path test_file_path = current_path / test_file;
    int json_indent = 2;
    assert(cal.to_json_file(test_file_path.string(), json_indent) == true); // JSON 파일로 export

    auto cal_from_file = holiday_calendar::from_json_file(test_file); // JSON 파일로 import
    assert(cal_from_file.has_value());
    assert(cal_from_file->is_holiday(2026, 8, 17, weekday::monday) == true);

    std::remove(test_file_path.string().c_str());
    log->info("<green>[Test 2] Passed!</green>");
}

// 3-3. 스케줄러 연동 및 중간 Import 시 실시간 반영 검증 (멀티스레드)
void test_scheduler_dynamic_holiday_application(std::shared_ptr<logger> log) {
    log->info("<bold><cyan>[Test 3-3] Testing Scheduler Runtime Holiday Import & Multi-threading...</cyan></bold>");

    task_scheduler scheduler;
    scheduler.set_logger(log);
    scheduler.start();

    // 1. 공휴일이 없는 상태에서 "매월 말일 18:00" 실행 태스크 등록
    int executed_count = 0;
    auto task_id = scheduler.add_task(
        std::make_unique<monthly_strategy>(-1, 18, 0),
        [&executed_count, log]() {
            executed_count++;
            log->info("Monthly task executed! Count: {}", executed_count);
        },
        "Monthly End Task"
    );

    auto tasks = scheduler.list_tasks();
    assert(tasks.size() == 1);
    log->info("Task registered with ID: <bright_green>{}</bright_green>, Strategy: <bright_blue>{}</bright_blue>", task_id, tasks[0].strategy_type);

    // 2. 다른 스레드에서 공휴일 JSON 적용 (멀티스레드 검증)
    std::thread importer_thread([&scheduler, log]() {
        log->info("Importer thread: Starting runtime holiday import...");
        std::string holiday_json = R"({
            "time_basis": "localtime",
            "policy": "shift_to_previous",
            "weekend_holidays": ["saturday", "sunday"],
            "recurring_holidays": [
                { "month": 12, "day": 31 }
            ],
            "specific_holidays": [],
            "recurring_exclusions": [],
            "specific_exclusions": []
        })";
        bool loaded = scheduler.load_holiday_json(holiday_json); // 공휴일 JSON 적용
        assert(loaded == true);
        log->info("Importer thread: Holiday JSON applied successfully.");
        });

    importer_thread.join(); // 쓰레드 종료 시까지 대기

    // 3. 중간 import 후 태스크의 next_run_time이 즉시 shift_to_previous(비휴일)로 갱신되었는지 확인
    tasks = scheduler.list_tasks();
    assert(tasks.size() == 1);

    const auto& task = tasks[0];

    // next_run_time을 포맷팅하기 위한 date_time_parts 변환
    auto run_parts = date_time_parts::from_time_point(task.next_run_time, scheduler.get_time_basis());

    std::ostringstream time_ss;
    time_ss << run_parts.year << "-"
        << std::setfill('0') << std::setw(2) << run_parts.month << "-"
        << std::setfill('0') << std::setw(2) << run_parts.day << " "
        << std::setfill('0') << std::setw(2) << run_parts.hour << ":"
        << std::setfill('0') << std::setw(2) << run_parts.minute << ":"
        << std::setfill('0') << std::setw(2) << run_parts.second
        << " (" << to_string(run_parts.weekday) << ")";

    log->info("<bold><yellow>--- Task[0] Info ---</yellow></bold>");
    log->info("  Task ID        : <green>{}</green>", task.task_id);
    log->info("  Description    : <yellow>{}</yellow>", task.description.value_or("N/A"));
    log->info("  Strategy Type  : <cyan>{}</cyan>", task.strategy_type);
    log->info("  Strategy Params: <bright_blue>{}</bright_blue>", task.strategy_params_json);
    log->info("  Next Run Time  : <magenta>{}</magenta>", time_ss.str());
    log->info("<bold><yellow>-----------------</yellow></bold>");

    auto parts = date_time_parts::from_time_point(tasks[0].next_run_time, time_base::localtime);
    log->info("Adjusted next run date: <bright_green>{}-{}-{}</bright_green>", parts.year, parts.month, parts.day);

    // 매달 말일에 수행하는 task 이다. 
    // 12월 31일이 공휴일로 등록되었다. 
    // 정책이 shift_to_previous 이다. (공휴일인 경우 이전 평일에 수행)
    // 따라서, 12월 말일 실행일은 31일이 아니어야 함. (12월 30일(평일인 경우)에 수행되어야 함)
    if (parts.month == 12) {
        assert(parts.day != 31);
    }

    scheduler.stop();
    log->info("<green>[Test 3] Passed!</green>");
}

// 3-4. task_scheduler 에 holiday_calendar 정보를 설정하는 다양한 방법 검증
void test_scheduler_holiday_configuration_methods(std::shared_ptr<logger> log) {
    log->info("<bold><cyan>[Test 3-4] Testing Various Ways to Set Holiday Calendar in Task Scheduler...</cyan></bold>");

    task_scheduler scheduler;
    scheduler.set_logger(log);

    std::cout << std::endl;

    // -------------------------------------------------------------------------
    // 방법 1: holiday_calendar 객체를 생성하여 통째로 주입 (set_holiday_calendar)
    // -------------------------------------------------------------------------
    {
        log->info("  Method 1: Injecting holiday_calendar object via <yellow>set_holiday_calendar()</yellow>");

        holiday_calendar cal(time_base::localtime, holiday_policy::shift_to_next);
        cal.set_weekend_holiday(weekday::saturday, true);
        cal.set_weekend_holiday(weekday::sunday, true);
        cal.add_recurring_holiday(1, 1);           // 매년 1월 1일 신정
        cal.add_specific_holiday(2026, 5, 6);      // 2026년 5월 6일 임시공휴일

        scheduler.set_holiday_calendar(std::move(cal)); // holiday_calendar 객체를 통째로 주입

        assert(scheduler.has_holiday_calendar() == true);
        log->info("    -> Holiday calendar successfully set via object injection.");
    }

    std::cout << std::endl;

    // -------------------------------------------------------------------------
    // 방법 2: JSON 파일 경로로 직접 로드 (load_holiday_file) 및 Export (export_holiday_file)
    // -------------------------------------------------------------------------
    {
        log->info("  Method 2: Loading from file path via <yellow>load_holiday_file()</yellow>");
        std::filesystem::path current_path = std::filesystem::current_path();
        std::filesystem::path tmp_file_path = current_path / "scheduler_holiday_conf_test.json";

        // 테스트용 공휴일 파일 생성
        holiday_calendar file_cal(time_base::localtime, holiday_policy::skip);

        file_cal.add_recurring_holiday(12, 25);
        int json_indent = 2;
        file_cal.to_json_file(tmp_file_path.string(), json_indent);
        log->info("   -> Temporary holiday calendar file created at:\n <bright_blue>{}</bright_blue>", tmp_file_path.string());

        bool loaded = scheduler.load_holiday_file(tmp_file_path.string()); // 스케줄러에서 파일 경로로 직접 로드
        log->info(" scheduler is loading holiday calendar from file:\n <green>{}</green>", tmp_file_path.string());

        assert(loaded == true);
        assert(scheduler.has_holiday_calendar() == true);

        // 파일로 다시 내보내기 검증
        std::filesystem::path export_path = current_path / "scheduler_holiday_export_test.json";
        assert(scheduler.export_holiday_file(export_path.string(), json_indent) == true);
        log->info(" scheduler is exporting holiday calendar to file:\n <magenta>{}</magenta>", export_path.string());

        std::remove(tmp_file_path.string().c_str());
        std::remove(export_path.string().c_str());
        log->info("    -> Holiday calendar successfully loaded and exported via file path.");
    }

    std::cout << std::endl;

    // -------------------------------------------------------------------------
    // 방법 3: 스케줄러 Direct Fluent API 호출 (스레드 안전한 개별 추가/제외)
    // -------------------------------------------------------------------------
    {
        log->info("  Method 3: Direct modification via scheduler fluent APIs");

        // 정책 및 주말 설정
        scheduler.set_holiday_policy(holiday_policy::shift_to_previous);
        scheduler.set_weekend_holiday(weekday::saturday, true);
        scheduler.set_weekend_holiday(weekday::sunday, true);

        // 공휴일 추가
        scheduler.add_recurring_holiday(3, 1);             // 3.1절
        scheduler.add_specific_holiday(2026, 10, 1);       // 2026년 10월 1일 국군의 날

        // 제외일 추가 (공휴일이라도 근무일로 지정)
        scheduler.add_recurring_exclusion(5, 1);           // 매년 5월 1일은 근무일
        scheduler.add_specific_exclusion(2026, 3, 1);      // 2026년 3월 1일만 특별 근무일

        // 규칙 동적 제거
        scheduler.remove_specific_holiday(2026, 10, 1);
        scheduler.remove_specific_exclusion(2026, 3, 1);

        // JSON 문자열로 내보내어 설정값 확인
        int json_indent = 2;
        std::string exported_json = scheduler.export_holiday_json(json_indent);
        assert(!exported_json.empty());

        log->debug("    Current Scheduler Holiday Config:\n{}", exported_json);
        log->info("    -> Direct holiday modifications applied successfully.");
    }

    std::cout << std::endl;

    // -------------------------------------------------------------------------
    // 방법 4: 공휴일 설정 완전 해제 (clear_holiday_calendar)
    // -------------------------------------------------------------------------
    {
        log->info("  Method 4: Clearing holiday calendar via <yellow>clear_holiday_calendar()</yellow>");
        scheduler.clear_holiday_calendar();
        assert(scheduler.has_holiday_calendar() == false);
        log->info("    -> Holiday calendar cleared. Scheduler restored to standard period mode.");
    }

    std::cout << std::endl;

    log->info("<green>[Test 3-4] Passed!</green>");
}
