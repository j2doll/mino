#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>

#include "mino/core/resilience/resilience.hpp"

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// -----------------------------------------------------------------------------
// 7. 조합 테스트: bulkhead -> token_bucket -> circuit_breaker
// -----------------------------------------------------------------------------
void test_combined() {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    using namespace std::chrono_literals;

    namespace mcs = mino::core::resilience;

    print(endl, tce("========================================"));
    print(tce(" [7] Combined Test (bulkhead -> rate limiter -> circuit breaker)"));
    print(tce("========================================"));

    // NOTE: 호출 순서
    //  리미터가 거부한 요청이 써킷 브레이커의 '실패'로 집계되지 않도록,
    //  bulkhead -> rate limiter -> circuit_breaker -> 실제 호출 순서로 구성함.
    mcs::bulkhead gate({ 2 }); // 동시 실행 수 제한 2

    mcs::token_bucket::config_t tb_config;

    tb_config.capacity = 2; // 버스트 2회
    tb_config.refill_tokens = 1; // 1회씩 보충
    tb_config.refill_period = 10000ms; // 테스트 중에는 보충되지 않도록 길게 설정

    mcs::token_bucket limiter(tb_config);

    mcs::circuit_breaker::config_t cb_config;
    cb_config.failure_threshold = 3; // 3회 연속 실패 시 Open
    mcs::circuit_breaker cb(cb_config);

    auto call_service = [&](int id, bool fail) {
        auto permit = gate.try_acquire(); // 1) 동시 실행 수 제한
        if (!permit) {
            print(tce("  요청 "), id, tce(": 벌크헤드 거부"));
            return;
        }
        if (!limiter.try_acquire()) { // 2) 호출 속도 제한
            print(tce("  요청 "), id, tce(": 속도 제한 거부 (재시도까지 "),
                limiter.retry_after().count(), tce(" ms)"));
            return;
        }
        cb.execute([id, fail]() { // 3) 써킷 브레이커 + 실제 호출
            if (fail) {
                throw std::runtime_error("외부 서비스 오류");
            }
            });
        print(tce("  요청 "), id, fail ? tce(": 호출 실패") : tce(": 호출 성공"));
    };

    call_service(1, true); // 실패 1회 집계
    call_service(2, true); // 실패 2회 집계
    call_service(3, true); // 토큰 소진: 속도 제한 거부 (실패로 집계되지 않음)
    call_service(4, true); // 토큰 소진: 속도 제한 거부 (실패로 집계되지 않음)

    const auto s = cb.current_state();
    print(endl, tce("  써킷 상태: "), s == mcs::circuit_breaker::state::closed ? "CLOSED" : "NOT CLOSED");
    // 써킷 상태: CLOSED
    //  NOTE: 거부된 요청 2건이 실패로 집계되었다면 연속 실패가 4회가 되어 OPEN 이 되었을 것임.
}


