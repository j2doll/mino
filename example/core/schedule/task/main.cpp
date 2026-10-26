#include <memory>
#include "mino/core/log/tinylog/tinylog.hpp"

extern void test_strategy_calculations();
extern void test_scheduler_execution();

int main() {
    using namespace mino::core::log::tinylog;

    // "task_test" 로거 생성 및 콘솔 싱크 연결
    auto log = std::make_shared<logger>("task_test");
    log->add_sink(std::make_shared<console_sink>("console"));
    log->set_level(log_level::trace);
    logger::register_logger(log);

    test_strategy_calculations(); // [Test 1] 주기 전략 다음 실행 시간 계산 검증
    std::cout << std::endl << std::endl;
    test_scheduler_execution();   // [Test 2] Task Scheduler 실시간 동작 테스트

    return 0;
}
