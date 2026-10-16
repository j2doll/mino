#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>

#include "mino/core/resilience/resilience.hpp"

#include "mino/core/datetime/datetime.hpp" 
#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// token_bucket은 빈도를 조절할 때 널리 쓰이는 토큰 버킷(Token Bucket) 알고리즘 기반의
// 속도 제한기(Rate Limiter) 입니다.
//  네트워크 트래픽 제어나 API 호출 제한 등에서 사용되며,
//  평균 속도를 제한하면서도 순간적인 버스트(Burst)를 허용합니다.

// -----------------------------------------------------------------------------
// 4. 토큰 버킷 (token_bucket) 테스트
// -----------------------------------------------------------------------------

using namespace std::chrono_literals;

void test_token_bucket() {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    namespace mcs = mino::core::resilience;
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;

    print(endl, tce("========================================"));
    print(tce(" [4] Token Bucket Test"));
    print(tce("========================================"));

    // NOTE: 토큰 버킷(Token Bucket)
    //  평균 속도는 제한하면서, capacity 만큼의 순간 버스트는 허용함.

    mcs::token_bucket::config_t config;
    config.capacity = 3; // 최대 3개 토큰 (최대 버스트 크기)
    config.refill_tokens = 1; // 주기마다 1개 보충
    config.refill_period = 500ms; // 0.5초마다 보충

    mcs::token_bucket limiter(config);

    print(tce("--- [1단계] 연속 5회 요청 (버스트 3회까지 허용) ---"));

    for (int i = 1; i <= 5; ++i) {
        if (limiter.try_acquire()) {
            std::string current_local
                = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
            print(
                current_local,
                tce("  요청 "), i,
                tce(": 허용 (남은 토큰: "), limiter.available(), tce(")"));
        }
        else {
            auto retry_ms = limiter.retry_after().count(); // 토큰이 보충될 때까지 남은 시간 (밀리초)

            std::string current_local
                = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
            print(
                current_local,
                tce("  요청 "), i,
                tce(": 거부 (재시도까지 "), retry_ms, tce(" ms)"));

            std::this_thread::sleep_for(std::chrono::milliseconds(retry_ms)); // 토큰이 보충될 때까지 대기
        }
    }
    // -- - [1단계] 연속 5회 요청(버스트 3회까지 허용) -- -
    //     2026-10-17 01:13:02.167  요청 1 : 허용(남은 토큰 : 2)
    //     2026-10-17 01:13:02.167  요청 2 : 허용(남은 토큰 : 1)
    //     2026-10-17 01:13:02.168  요청 3 : 허용(남은 토큰 : 0)
    //     2026-10-17 01:13:02.168  요청 4 : 거부(재시도까지 500 ms)
    //     2026-10-17 01:13:02.669  요청 5 : 허용(남은 토큰 : 0)

    print(endl, tce("--- [2단계] 600ms 대기 후 재요청 ---"));

    std::this_thread::sleep_for(600ms);

    std::string current_local
        = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
    print(
        current_local,
        tce("  요청: "), limiter.try_acquire() ? tce("허용") : tce("거부"));
    // --- [2단계] 600ms 대기 후 재요청-- -
    //     2026-10-17 01:13:03.281  요청 : 허용

    print(endl, tce("--- [3단계] 토큰이 보충될 때까지 대기하며 획득 (최대 2초) ---"));

    const auto start = std::chrono::steady_clock::now(); // 현재 시간 기록
    const bool acquired = limiter.acquire_for(1, 2000ms); // 토큰이 보충될 때까지 대기하며 획득 (최대 2초)
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start); // 실제 대기 시간 계산

    current_local = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
    print(
        current_local,
        tce("  결과: "), acquired ? tce("획득 성공") : tce("시간 초과"),
        tce(" (실제 대기: "), elapsed.count(), tce(" ms)"));
    // --- [3단계] 토큰이 보충될 때까지 대기하며 획득(최대 2초) -- -
    //     2026-10-17 01:13:03.672  결과 : 획득 성공(실제 대기 : 389 ms)

}
