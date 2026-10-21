#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/dispatch/dispatch.hpp"

/**
 * [Case 4] collecting_event_dispatcher 테스트용 이벤트
 * - 다중 보안 검증 파이프라인에서 핸들러들의 개별 검증 결과(bool)를 취합하기 위한 요청 데이터입니다.
 */
struct permission_check_event {
    std::string user_name;
    std::string action;
    int clear_level;
};

void test_collecting_event_dispatcher() {
    namespace mcd = mino::core::dispatch;

    // -------------------------------------------------------------------------
    // 4. collecting_event_dispatcher 테스트
    // 특징: 등록된 핸들러들의 반환값을 std::vector<return_t>로 취합하여 반환
    // -------------------------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "4. collecting_event_dispatcher (Result Aggregation)\n";
    std::cout << "========================================================\n";

    // 핸들러가 bool 타입을 반환하는 검증용 디스패처 생성
    mcd::collecting_event_dispatcher<bool> auth_disp;

    // 검증 규칙 1: 시스템 종료 명령어의 권한 등급 확인
    auth_disp.subscribe<permission_check_event>([](const auto& ev) {
        bool allowed = !(ev.action == "SERVER_SHUTDOWN" && ev.clear_level < 5);
        auto ret = mino::core::datetime::util::current_time_string() +
            "  [Check: Action]        " + ev.action +
            " | Required Level: " + std::to_string(ev.clear_level) +
            " | Result: " + (allowed ? "PASS" : "FAIL") + "\n";
        std::cout << ret << std::flush;
        return allowed;
        });

    // 검증 규칙 2: 게스트 계정 제한 확인
    auth_disp.subscribe<permission_check_event>([](const auto& ev) {
        bool allowed = (ev.user_name != "guest");
        auto ret = mino::core::datetime::util::current_time_string() +
            "  [Check: User]          " + ev.user_name +
            " | Result: " + (allowed ? "PASS" : "FAIL") + "\n";
        std::cout << ret << std::flush;
        return allowed;
        });

    // 이벤트 디스패치 실행 후 각 핸들러의 실행 결과가 담긴 vector<bool> 수집
    permission_check_event req{ "guest", "SERVER_SHUTDOWN", 3 };
    std::cout << "  -- Executing Permission Validation for User: " << req.user_name << " --\n";
    std::vector<bool> inspection_results = auth_disp.dispatch(req);

    // 수집된 모든 검증 결과의 참/거짓 종합 평가
    bool all_passed = true;
    for (std::size_t i = 0; i < inspection_results.size(); ++i) {
        if (!inspection_results[i]) {
            all_passed = false;
        }
    }

    std::cout << "  [Final Verdict] Execution: " << (all_passed ? "ALLOWED" : "DENIED") << "\n";
    std::cout << "========================================================\n";
}
