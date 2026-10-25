#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

#include "mino/core/schedule/task/task.hpp"
#include "mino/core/datetime/util/util.hpp" 
#include "mino/core/string/string.hpp"
#include "mino/core/enum/enum.hpp"

extern void test_strategy_calculations();
extern void test_scheduler_execution();

int main() {
    test_strategy_calculations(); // [Test 1] 주기 전략 다음 실행 시간 계산 검증
    test_scheduler_execution(); // [Test 2] Task Scheduler 실시간 동작 테스트

    return 0;
}
