#include "mino/core/schedule/task/task_scheduler.hpp"
#include "mino/core/schedule/task/task_scheduler_json.hpp"
#include "mino/core/schedule/task/period_strategy.hpp"
#include "mino/core/string/to_console_encoding.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <ctime>
#include <iomanip>
#include <filesystem>

using namespace mino::core::schedule::task;

// 콘솔 인코딩 헬퍼
const auto& tce = mino::core::string::to_console_encoding;

// 한글 문자열을 콘솔에 출력하기 위한 헬퍼
void print_task_info(const task_scheduler::task_info& task) {
    std::cout << tce("  작업 ID: ") << task.task_id << "\n";
    std::cout << tce("  설명: ") << (task.description ? tce(*task.description) : tce("(없음)")) << "\n";
    
    auto time_t = std::chrono::system_clock::to_time_t(task.next_run_time);
    auto tm = *std::localtime(&time_t);
    std::cout << tce("  다음 실행: ") << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
    
    std::cout << tce("  전략 유형: ") << task.strategy_type << "\n";
    std::cout << tce("  전략 파라미터: ") << task.strategy_params_json << "\n";
}

// 외부 호출을 위한 함수 - main.cpp에서 호출됨
void test_scheduler_execution() {
    std::cout << tce("=== 작업 스케줄러 테스트 (UTC/Localtime 설정) ===") << "\n\n";

    // 작업 디렉토리 출력
    std::cerr << "[INFO] Current working directory: " << std::filesystem::current_path() << "\n";

    // --- Test 1: Localtime 기준 스케줄러 생성
    std::cout << tce("*** Test 1: Localtime 기준 스케줄러 ***") << "\n";
    task_scheduler scheduler_local(time_base::localtime);
    std::cout << tce("생성된 스케줄러 시간 기준: ");
    std::cout << (scheduler_local.get_time_basis() == time_base::utc ? "UTC" : "Localtime") << "\n\n";

    // 작업 추가
    int exec_count_1 = 0;
    int target_sec_1 = 29;
    std::string desc1 = "Task 1: 매 분 " + std::to_string(target_sec_1) + "초";
    
    scheduler_local.add_task(
        std::make_unique<minutely_strategy>(target_sec_1),
        [&exec_count_1]() { ++exec_count_1; },
        desc1
    );

    int exec_count_2 = 0;
    int target_min_2 = 15, target_sec_2 = 45;
    std::string desc2 = "Task 2: 매 시간 " + std::to_string(target_min_2) + "분 " + std::to_string(target_sec_2) + "초";
    
    scheduler_local.add_task(
        std::make_unique<hourly_strategy>(target_min_2, target_sec_2),
        [&exec_count_2]() { ++exec_count_2; },
        desc2
    );

    int exec_count_3 = 0;
    int target_hour_3 = 10, target_min_3 = 30, target_sec_3 = 0;
    std::string desc3 = "Task 3: 매일 " + std::to_string(target_hour_3) + "시 " + std::to_string(target_min_3) + "분";
    
    scheduler_local.add_task(
        std::make_unique<daily_strategy>(target_hour_3, target_min_3, target_sec_3),
        [&exec_count_3]() { ++exec_count_3; },
        desc3
    );

    int exec_count_4 = 0;
    int target_weekday_4 = static_cast<int>(weekday::friday);
    int target_hour_4 = 14, target_min_4 = 0, target_sec_4 = 0;
    std::string desc4 = "Task 4: 매주 금요일 " + std::to_string(target_hour_4) + "시";
    
    scheduler_local.add_task(
        std::make_unique<weekly_strategy>(
            static_cast<weekday>(target_weekday_4),
            target_hour_4, target_min_4, target_sec_4),
        [&exec_count_4]() { ++exec_count_4; },
        desc4
    );

    int exec_count_5 = 0;
    int target_day_5 = 1;
    int target_hour_5 = 9, target_min_5 = 0;
    std::string desc5 = "Task 5: 매월 " + std::to_string(target_day_5) + "일 " + std::to_string(target_hour_5) + "시";
    
    scheduler_local.add_task(
        std::make_unique<monthly_strategy>(target_day_5, target_hour_5, target_min_5),
        [&exec_count_5]() { ++exec_count_5; },
        desc5
    );

    std::cout << tce("추가된 작업 목록:") << "\n";
    for (const auto& task : scheduler_local.list_tasks()) {
        print_task_info(task);
        std::cout << "\n";
    }

    // --- Test 2: UTC 기준으로 변경
    std::cout << tce("*** Test 2: UTC로 시간 기준 변경 ***") << "\n";
    scheduler_local.set_time_basis(time_base::utc);
    std::cout << tce("변경된 시간 기준: ");
    std::cout << (scheduler_local.get_time_basis() == time_base::utc ? "UTC" : "Localtime") << "\n\n";

    // --- Test 3: UTC 기준 스케줄러로 생성
    std::cout << tce("*** Test 3: UTC 기준 스케줄러 직접 생성 ***") << "\n";
    task_scheduler scheduler_utc(time_base::utc);
    
    int utc_exec_count = 0;
    scheduler_utc.add_task(
        std::make_unique<daily_strategy>(12, 0, 0),
        [&utc_exec_count]() { ++utc_exec_count; },
        "UTC Task: 매일 12시 (UTC)"
    );

    std::cout << tce("UTC 스케줄러 시간 기준: ");
    std::cout << (scheduler_utc.get_time_basis() == time_base::utc ? "UTC" : "Localtime") << "\n";
    std::cout << tce("추가된 작업:") << "\n";
    for (const auto& task : scheduler_utc.list_tasks()) {
        print_task_info(task);
    }
    std::cout << "\n";

    // --- Test 4: JSON 직렬화 (시간 기준 포함)
    std::cout << tce("*** Test 4: JSON 직렬화 (시간 기준 포함) ***") << "\n";
    std::string json_output = task_scheduler_json::to_json_string(scheduler_local, 2);
    std::cout << tce("JSON 출력:") << "\n" << json_output << "\n\n";

    // JSON 파일에 저장 - 절대 경로 사용
    std::filesystem::path json_file_path = std::filesystem::current_path() / "scheduler_config.json";
    std::string json_file = json_file_path.string();
    
    std::cerr << "[INFO] Attempting to save JSON to: " << json_file << "\n";
    
    if (task_scheduler_json::to_json_file(scheduler_local, json_file, 2)) {
        std::cout << tce("JSON 파일에 저장됨: ") << json_file << "\n\n";
        
        // 파일 존재 확인
        if (std::filesystem::exists(json_file_path)) {
            std::cerr << "[INFO] File exists! Size: " << std::filesystem::file_size(json_file_path) << " bytes\n";
        } else {
            std::cerr << "[ERROR] File does not exist after saving!\n";
        }
    } else {
        std::cerr << "[ERROR] Failed to save JSON file!\n";
        std::cout << tce("JSON 파일 저장 실패!") << "\n\n";
        return;
    }

    // --- Test 5: JSON에서 복원 (시간 기준 포함)
    std::cout << tce("*** Test 5: JSON에서 스케줄러 복원 ***") << "\n";
    
    // 파일이 실제로 존재하는지 다시 확인
    if (!std::filesystem::exists(json_file_path)) {
        std::cerr << "[ERROR] JSON file not found before restore: " << json_file << "\n";
        std::cout << tce("JSON 파일을 찾을 수 없습니다!") << "\n";
        return;
    }
    
    std::cerr << "[INFO] JSON file confirmed to exist\n";
    
    task_scheduler restored_scheduler(time_base::localtime);
    
    auto work_factory = [](uint64_t id, const std::string& strategy_type) {
        return [id, strategy_type]() {
            std::cout << tce("  작업 실행 (ID: ") << id << tce(", 전략: ") << strategy_type << ")\n";
        };
    };

    std::cerr << "[INFO] Calling restore_tasks_to_scheduler...\n";
    std::cout.flush();
    std::cerr.flush();

    if (task_scheduler_json::restore_tasks_to_scheduler(
        restored_scheduler, json_file, work_factory)) {
        
        std::cout << tce("복원된 스케줄러 시간 기준: ");
        std::cout << (restored_scheduler.get_time_basis() == time_base::utc ? "UTC" : "Localtime") << "\n";
        std::cout << tce("복원된 작업 목록:") << "\n";
        for (const auto& task : restored_scheduler.list_tasks()) {
            print_task_info(task);
            std::cout << "\n";
        }
    } else {
        std::cerr << "[ERROR] restore_tasks_to_scheduler returned false\n";
        std::cout << tce("스케줄러 복원 실패!") << "\n";
    }

    std::cout << tce("=== 테스트 완료 ===") << "\n";
}


