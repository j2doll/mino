#include <memory>
#include "mino/core/log/tinylog/tinylog.hpp"

extern void test_strategy_calculations();
extern void test_scheduler_execution();

extern void test_holiday_calendar_rules(std::shared_ptr<mino::core::log::tinylog::logger> log);
extern void test_holiday_json_import_export(std::shared_ptr<mino::core::log::tinylog::logger> log);
extern void test_scheduler_dynamic_holiday_application(std::shared_ptr<mino::core::log::tinylog::logger> log);
extern void test_scheduler_holiday_configuration_methods(std::shared_ptr<mino::core::log::tinylog::logger> log);

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
    std::cout << std::endl << std::endl;

    test_holiday_calendar_rules(log); // 3-1. 공휴일 캘린더 단독 기능 검증 (주말, 반복, 1회성, 제외일 우선순위)
    std::cout << std::endl << std::endl;

    test_holiday_json_import_export(log); // 3-2. 공휴일 JSON 파일/문자열 Import/Export 검증
    std::cout << std::endl << std::endl;

    test_scheduler_dynamic_holiday_application(log); // 3-3. 스케줄러 연동 및 중간 Import 시 실시간 반영 검증 (멀티스레드)
    std::cout << std::endl << std::endl;

    test_scheduler_holiday_configuration_methods(log); // 3-4. task_scheduler 에 holiday_calendar 정보를 설정하는 다양한 방법 검증
    std::cout << std::endl << std::endl;

    return 0;
}
