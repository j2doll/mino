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

// circuit_breaker는 외부 시스템의 장애가 발생했을 때
// 불필요한 호출을 즉시 차단(Fail-Fast)하여
// 장애가 전체 시스템으로 전파(Cascading Failure)되는 것을 막고,
// 대상 시스템이 회복할 시간을 확보해 주는
// 서킷 브레이커(Circuit Breaker) 패턴 구현체입니다. 

// -----------------------------------------------------------------------------
// 3. 써킷 브레이커 (circuit_breaker) 테스트
// -----------------------------------------------------------------------------
namespace {
    // 써킷 브레이커 상태 출력 헬퍼 함수
    const char* state_to_string(mino::core::resilience::circuit_breaker::state s) {
        namespace mcs = mino::core::resilience;
        using state = mcs::circuit_breaker::state;
        switch (s) {
        case state::closed: return "CLOSED";
        case state::open: return "OPEN";
        case state::half_open: return "HALF_OPEN";
        }
        return "UNKNOWN";
    }
}

void test_circuit_breaker() {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    using namespace std::chrono_literals;

    namespace mcs = mino::core::resilience;
    using config_t = mcs::circuit_breaker::config_t;
    using circuit_breaker = mcs::circuit_breaker;

    print(endl, tce("========================================"));
    print(tce(" [3] Circuit Breaker Test"));
    print(tce("========================================"));

    // NOTE: 써킷 브레이커(Circuit Breaker) 상태
    //  Open 상태: 써킷 브레이커가 열려서 모든 호출이 차단됨.
    //  Closed 상태: 써킷 브레이커가 닫혀서 모든 호출이 정상적으로 전달됨.
    //  Half-Open 상태: 써킷 브레이커가 반쯤 열려서 제한된 호출만 허용되고, 결과에 따라 상태가 전환됨.

    // 빠른 테스트를 위한 설정
    config_t config;
    config.failure_threshold = 3; // 3회 연속 실패 시 Open(차단) 상태로 전환
    config.reset_timeout = 500ms; // 0.5초 후 Half-Open(부분 허용) 상태로 전환
    config.half_open_success_threshold = 2; // Half-Open에서 2회 연속 성공 시 Closed(호출) 상태로 전환

    circuit_breaker cb(config); // 써킷 브레이커 객체 생성

    print(tce("초기 써킷 상태: "), state_to_string(cb.current_state()), endl);
    // 초기 써킷 상태: CLOSED(써킷 브레이커가 닫혀서 모든 호출이 정상적으로 전달됨)

    // 1. 의도적 실패를 유도하여 Open 상태로 만들기
    print(tce("--- [1단계] 연속 3회 실패 발생 시키기 ---"));

    for (int i = 1; i <= 3; ++i) {

        // execute 호출 시, 의도적으로 예외를 발생시켜 실패를 유도
        cb.execute([i, &tce]() {
            auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
            print(tce("  호출 "), i, tce(" 시도 중... 예외 발생!"));

            throw std::runtime_error("DB 연결 실패");
        });

        print(tce("  -> 현재 상태: "), state_to_string(cb.current_state()));
        // i가 1이면, 현재 상태: CLOSED
        // i가 2이면, 현재 상태: CLOSED
        // i가 3이면, 현재 상태: OPEN (써킷 브레이커가 열려서 모든 호출이 차단됨)
    }
    //  호출 1 시도 중... 예외 발생!
    //  -> 현재 상태: CLOSED
    // 
    //  호출 2 시도 중... 예외 발생!
    //  -> 현재 상태: CLOSED
    // 
    //  호출 3 시도 중... 예외 발생!
    //  -> 현재 상태: OPEN

    // 2. Open 상태에서 차단되는지 확인
    print(endl, tce("--- [2단계] Open 상태에서 차단 여부 확인 ---"), endl);

    // Open 상태에서 execute 호출 시, 예외가 발생하여 차단됨. 
    cb.execute([&tce]() {
        std::cerr << tce("  이 문장은 출력되지 않아야 합니다.\n");
        });

    print(tce("  차단 후 상태: "), state_to_string(cb.current_state()), endl);
    //  차단 후 상태: OPEN (써킷 브레이커가 열려서 모든 호출이 차단됨)

    // 3. reset_timeout 대기 후 Half-Open 진입 확인
    print(endl, tce("--- [3단계] Reset Timeout (600ms) 대기 ---"), endl);
    std::this_thread::sleep_for(600ms); // config.reset_timeout(0.5초) 보다 더 대기

    // 4. Half-Open 상태에서 성공을 쌓아 Closed로 복구
    print(tce("--- [4단계] Half-Open 복구 시도 (연속 2회 성공 필요) ---"), endl);
    // 첫 번째 성공
    cb.execute([&tce]() {
        std::cout << tce("  Half-Open 첫 번째 성공 호출\n");
        });
    print(tce("  -> 상태: "), state_to_string(cb.current_state()), endl);
    //
    //  Half-Open 첫 번째 성공 호출
    //  -> 상태: HALF_OPEN

    // 두 번째 성공
    cb.execute([&tce]() {
        std::cout << tce("  Half-Open 두 번째 성공 호출\n");
        });
    print(tce("  -> 최종 복구 상태: "), state_to_string(cb.current_state()), endl);
    //
    //  Half-Open 두 번째 성공 호출
    //  -> 최종 복구 상태: CLOSED

}


