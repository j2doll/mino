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
// sliding_window_limiter는
// "최근 일정 시간 구간(윈도우) 내 요청 횟수를 최대 N회로 제한"하기 위해
// 사용하는 슬라이딩 윈도우(Sliding Window) 알고리즘 기반 속도 제한기.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 5. 슬라이딩 윈도우 (sliding_window_limiter) 테스트
// -----------------------------------------------------------------------------
void test_sliding_window_limiter() {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    using namespace std::chrono_literals;

    namespace mcs = mino::core::resilience;

    print(endl, tce("========================================"));
    print(tce(" [5] Sliding Window Limiter Test"));
    print(tce("========================================"));

    // NOTE: 슬라이딩 윈도우(Sliding Window)
    //  "최근 window 시간 안에 최대 max_requests 회"를 엄격히 보장함.
    mcs::sliding_window_limiter::config_t config;

    config.max_requests = 3; // 윈도우당 최대 3회
    config.window = 1000ms; // 윈도우 길이 1초
    config.algorithm = mcs::sliding_window_limiter::algorithm_t::exact; // 정확한 판정

    mcs::sliding_window_limiter limiter(config);

    print(tce("--- [1단계] 연속 4회 요청 (1초에 3회까지 허용) ---"));

    for (int i = 1; i <= 4; ++i) {
        print(tce("  요청 "), i, tce(": "), limiter.try_acquire() ? tce("허용") : tce("거부"));
    }
    //  요청 1: 허용
    //  요청 2: 허용
    //  요청 3: 허용
    //  요청 4: 거부

    print(tce("  사용량: "), limiter.used(), tce("/"), config.max_requests);
    // 사용량: 3 / 3

    const auto wait = limiter.retry_after();
    print(tce("  다음 허용까지: 약 "), wait.count(), tce(" ms"));
    // 다음 허용까지 : 약 1000 ms

    print(endl, tce("--- [2단계] 안내된 시간만큼 대기 후 재요청 ---"));

    std::this_thread::sleep_for(wait + 50ms); // 1초 + 50ms(여유 시간) 대기

    print(tce("  요청: "), limiter.try_acquire() ? tce("허용") : tce("거부"));
    // 요청: 허용

}

