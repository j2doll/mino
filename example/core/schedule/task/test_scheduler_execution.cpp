#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <filesystem>
#include <atomic>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/schedule/schedule.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/enum/enum.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

namespace tlog = mino::core::log::tinylog;
namespace mdtu = mino::core::datetime::util;

using namespace mino::core::schedule::task;

void print_task_info(const std::shared_ptr<tlog::logger>& logger, const task_scheduler::task_info& task) {
    if (!logger) return;
    logger->info("  작업 ID: <bright_green>{}</bright_green>", task.task_id);
    logger->info("  설명: <bright_blue>{}</bright_blue>", task.description ? *task.description : "(없음)");
    logger->info("  다음 실행: <gray>{}</gray>", mdtu::format_datetime(task.next_run_time, mdtu::time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss"));
    logger->info("  전략 유형: <bright_cyan>{}</bright_cyan>", task.strategy_type);
    logger->info("  전략 파라미터: <bright_white>{}</bright_white>\n", task.strategy_params_json);
}

// [Test 2] Task Scheduler 실시간 동작 테스트
void test_scheduler_execution() {
    auto logger = tlog::logger::get("task_test");
    if (!logger)
        return;

    logger->info("========================================");
    logger->info("[Test 2] Task Scheduler 실시간 동작 테스트");
    logger->info("========================================");

    logger->info("=== 작업 스케줄러 테스트 (UTC/Localtime 설정) ===");
    logger->info("[INFO] Current working directory: <gray>{}</gray>", std::filesystem::current_path().string());

    std::cout << std::endl;

    // --- Test 1: Localtime 기준 스케줄러 생성
    logger->info("<bright_yellow>*** Test 1: Localtime 기준 스케줄러 ***</bright_yellow> <blue>scheduler_local</blue>");
    task_scheduler scheduler_local(time_base::localtime);
    scheduler_local.set_logger(logger);

    logger->info("생성된 스케줄러 시간 기준: <bright_cyan>{}</bright_cyan>",
        scheduler_local.get_time_basis() == time_base::utc ? "UTC" : "Localtime");

    // 작업 추가
    int exec_count_1 = 0;
    int target_sec_1 = 29;
    scheduler_local.add_task(
        std::make_unique<minutely_strategy>(target_sec_1),
        [&exec_count_1]() { ++exec_count_1; },
        "Task 1: 매 분 29초"
    );

    int exec_count_2 = 0;
    scheduler_local.add_task(
        std::make_unique<hourly_strategy>(15, 45),
        [&exec_count_2]() { ++exec_count_2; },
        "Task 2: 매 시간 15분 45초"
    );

    int exec_count_3 = 0;
    scheduler_local.add_task(
        std::make_unique<daily_strategy>(10, 30, 0),
        [&exec_count_3]() { ++exec_count_3; },
        "Task 3: 매일 10시 30분"
    );

    int exec_count_4 = 0;
    scheduler_local.add_task(
        std::make_unique<weekly_strategy>(weekday::friday, 14, 0, 0),
        [&exec_count_4]() { ++exec_count_4; },
        "Task 4: 매주 금요일 14시"
    );

    int exec_count_5 = 0;
    scheduler_local.add_task(
        std::make_unique<monthly_strategy>(1, 9, 0),
        [&exec_count_5]() { ++exec_count_5; },
        "Task 5: 매월 1일 9시"
    );

    logger->info("추가된 작업 목록:");
    for (const auto& task : scheduler_local.list_tasks()) {
        print_task_info(logger, task);
    }

    std::cout << std::endl;

    // --- Test 2: UTC 기준으로 변경
    logger->info("<bright_yellow>*** Test 2: UTC로 시간 기준 변경 ***</bright_yellow> <blue>scheduler_local</blue>");
    scheduler_local.set_time_basis(time_base::utc);

    logger->info("변경된 시간 기준: <bright_cyan>{}</bright_cyan>",
        scheduler_local.get_time_basis() == time_base::utc ? "UTC" : "Localtime");

    std::cout << std::endl;

    // --- Test 3: UTC 기준 스케줄러로 생성
    logger->info("<bright_yellow>*** Test 3: UTC 기준 스케줄러 직접 생성 ***</bright_yellow> <blue>scheduler_utc</blue>");
    task_scheduler scheduler_utc(time_base::utc);
    scheduler_utc.set_logger(logger);

    int utc_exec_count = 0;
    scheduler_utc.add_task(
        std::make_unique<daily_strategy>(12, 0, 0),
        [&utc_exec_count]() { ++utc_exec_count; },
        "UTC Task: 매일 12시 (UTC)"
    );

    logger->info("UTC 스케줄러 시간 기준: <bright_cyan>{}</bright_cyan>",
        scheduler_utc.get_time_basis() == time_base::utc ? "UTC" : "Localtime");
    logger->info("추가된 작업:");
    for (const auto& task : scheduler_utc.list_tasks()) {
        print_task_info(logger, task);
    }

    std::cout << std::endl;

    // --- Test 4: JSON 직렬화 (시간 기준 포함)
    logger->info("<bright_yellow>*** Test 4: JSON 직렬화 (시간 기준 포함) ***</bright_yellow> <blue>scheduler_local</blue>");
    std::string json_output = task_scheduler_json::to_json_string(scheduler_local, 2);
    logger->info("<bright_blue>scheduler_local</bright_blue> JSON 출력:\n{}", json_output);

    std::filesystem::path json_file_path = std::filesystem::current_path() / "scheduler_config.json";
    std::string json_file = json_file_path.string();

    logger->info("JSON 파일 저장 시도: <gray>{}</gray>", json_file);

    if (task_scheduler_json::to_json_file(scheduler_local, json_file, 2)) {
        logger->info("JSON 파일에 저장됨: <gray>{}</gray>", json_file);

        if (std::filesystem::exists(json_file_path)) {
            logger->info("파일 생성 확인 완료 (크기: <gray>{}</gray> bytes)", std::filesystem::file_size(json_file_path));
        }
        else {
            logger->error("<red>파일 저장 후 확인 실패</red>: 파일이 존재하지 않습니다.");
        }
    }
    else {
        logger->error("<red>JSON 파일 저장 실패</red>!");
        return;
    }

    std::cout << std::endl;

    // --- Test 5: JSON에서 복원 (시간 기준 포함)
    logger->info("<bright_yellow>*** Test 5: JSON에서 스케줄러 복원 ***</bright_yellow> <blue>restored_scheduler</blue>");

    if (!std::filesystem::exists(json_file_path)) {
        logger->error("<red>복원 전 파일 미발견</red>: <gray>{}</gray>", json_file);
        return;
    }

    task_scheduler restored_scheduler(time_base::localtime);
    restored_scheduler.set_logger(logger);

    auto work_factory = [logger](uint64_t id, const std::string& strategy_type) {
        return [logger, id, strategy_type]() {
            if (logger) {
                logger->info("  작업 실행 (ID: {}, 전략: {})", id, strategy_type);
            }
            };
        };

    logger->info("<gray>restore_tasks_to_scheduler</gray> 호출 중: <magenta>restored_scheduler</magenta> 에 JSON 정보로 복원.");

    if (task_scheduler_json::restore_tasks_to_scheduler(
        restored_scheduler, json_file, work_factory)) {

        logger->info("복원된 스케줄러 시간 기준: <bright_cyan>{}</bright_cyan>",
            restored_scheduler.get_time_basis() == time_base::utc ? "UTC" : "Localtime");
        logger->info("복원된 작업 목록:");
        for (const auto& task : restored_scheduler.list_tasks()) {
            print_task_info(logger, task);
        }
    }
    else {
        logger->error("<red>restore_tasks_to_scheduler 복원 실패</red>");
    }

    std::cout << std::endl;

    // --- Test 6: 실시간 스케줄러 실행 (start / stop 동작 검증)
    logger->info("<bright_yellow>*** Test 6: 스케줄러 실시간 실행 테스트 (start / stop) ***</bright_yellow> <blue>live_scheduler</blue>");

    task_scheduler live_scheduler(time_base::localtime);
    live_scheduler.set_logger(logger);

    // 현재 시각 파악 후 약 2초 뒤에 실행되도록 초 설정
    auto now_tp = std::chrono::system_clock::now(); // 현재 시각
    auto now_parts = date_time_parts::from_time_point(now_tp, time_base::localtime); // 로컬 시간 기준으로 분리
    int trigger_sec = (now_parts.second + 2) % 60; // 현재 시간 기준으로 2초 뒤에 실행되도록 설정

    std::atomic<int> live_trigger_count{ 0 };

    uint64_t live_task_id = live_scheduler.add_task(
        std::make_unique<minutely_strategy>(trigger_sec),
        [logger, &live_trigger_count]() {
            ++live_trigger_count;
            if (logger) {
                logger->info("<bright_green>-> [실시간 작업 실행 성공] Work callback triggered! (count: {})</bright_green>",
                    live_trigger_count.load());
            }
        },
        "실시간 트리거 태스크 (2초 뒤 실행)"
    );

    logger->info("태스크 등록됨: ID <bright_cyan>{}</bright_cyan> (매 분 <bright_red>{}</bright_red>초에 실행)",
        live_task_id, trigger_sec);

    logger->info("스케줄러 시작 (<gray>live_scheduler.start()</gray>) 호출...");
    live_scheduler.start();

    logger->info("스케줄러 동작 대기 중 (약 5초 동안 대기)...");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    logger->info("스케줄러 종료 (<gray>live_scheduler.stop()</gray>) 호출...");
    live_scheduler.stop();

    logger->info("실행 결과: 총 트리거 횟수 = <bright_cyan>{}</bright_cyan>", live_trigger_count.load());

    std::cout << std::endl;
    logger->info("=== 테스트 완료 ===");
}

