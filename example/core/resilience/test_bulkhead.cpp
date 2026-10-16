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

//---------------------------------------------------------------------------
// * bulkhead는 "동시에 몇 개까지"만 제한하는 입구이고,
// '속도 제한'이나 '장애 차단'은 하지 않습니다.
// 그래서 다른 클래스와 겹겹이 쌓아 쓰는 것이 기본입니다.
//
// if (auto permit = gate.acquire_for(500ms)) { // 슬롯 획득 (최대 0.5초 대기)
//     do_work();
// }                                           // permit 소멸 시 슬롯 자동 반환
// else {
//     // 거부: 503 응답, 큐에 보관, 폴백 등
// }
// 
//---------------------------------------------------------------------------
// * 권장 조합 순서
// 요청 → bulkhead → rate limiter → circuit_breaker → 실제 호출
//       (동시 수)   (호출 속도)     (장애 차단)
//
//  - bulkhead가 맨 앞인 이유:
//    이미 몰려든 요청이 뒤의 자원(스레드, 커넥션)을 점유하기 전에 입구에서 막습니다.
//  - 리미터가 breaker 앞인 이유:
//     리미터가 거부한 요청이 circuit_breaker의 실패로 집계되지 않게 하기 위해서입니다.
//     circuit_breaker는 차단 시 예외 없이 기본값을 돌려주므로,
//     거부와 정상 결과를 구분하려면 앞단에서 bool로 걸러야 합니다.
//---------------------------------------------------------------------------
// * 예제 코드 (circuit_breaker + token_bucket)
// bulkhead::config_t config;
// config.max_concurrent = 2; // 동시에 최대 2개까지만 사용 가능
// bulkhead gate(config);
// 
// bool call_service(bulkhead& gate, token_bucket& limiter, circuit_breaker& cb)
// {
//     auto permit = gate.try_acquire(); // 사전에 미리 정해두 최대 갯수만큼만 동시 사용 가능
//     if (!permit)
//         return false; // 동시 실행 갯수가 초과
// 
//     if (!limiter.try_acquire()) // limiter는 속도 제한 용도
//         return false; // 속도 초과
// 
//     bool ok = false;
//     cb.execute([&] { ok = http_call(); }); // 실패 시 breaker가 집계.
//     // breaker는 실패 시 예외를 던지지 않고 기본값을 반환하도록 설정되어 있음
//     return ok;
// }
//---------------------------------------------------------------------------
// * retry_helper 와 함께 사용 (슬롯 점유 주의)
//   - 재시도 대기(sleep) 중에 슬롯을 쥐고 있으면 낭비됩니다.
//   - retry가 바깥, bulkhead가 안쪽이어야 시도마다 슬롯을 얻고 반납합니다.
// 
// bool ok = retry_helper::retry(
//     5, 100ms, true, logger,
//     [&]() -> bool {
//         auto permit = gate.acquire_for(200ms); // 시도마다 획득
//         if (!permit) return false;             // 실패로 간주되어 재시도
//         return call_service(...);
//     });  // 시도가 끝나면 슬롯 반납 후 백오프 대기
// 
//---------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// 6. 벌크헤드 (bulkhead) 테스트
// -----------------------------------------------------------------------------
void test_bulkhead() {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    using namespace std::chrono_literals;

    namespace mcs = mino::core::resilience;

    print(endl, tce("========================================"));
    print(tce(" [6] Bulkhead Test"));
    print(tce("========================================"));

    // NOTE: 벌크헤드(Bulkhead)
    //  동시에 실행되는 작업 수를 제한하여 자원 고갈을 막음.
    mcs::bulkhead::config_t config;
    config.max_concurrent = 2; // 동시에 최대 2개

    mcs::bulkhead gate(config);

    print(tce("--- [1단계] 슬롯 즉시 획득 (RAII permit) ---"));
    {
        auto p1 = gate.try_acquire(); // 슬롯 1개 획득 (+1)
        auto p2 = gate.try_acquire(); // 슬롯 1개 획득 (+2)
        auto p3 = gate.try_acquire(); // 슬롯이 없으므로 실패

        print(tce("  p1: "), p1 ? tce("획득") : tce("실패")); // 획득
        print(tce("  p2: "), p2 ? tce("획득") : tce("실패")); // 획득
        print(tce("  p3: "), p3 ? tce("획득") : tce("실패")); // 실패
        print(tce("  실행 중: "), gate.active(), tce(", 남은 슬롯: "), gate.available()); // 실행 중: 2, 남은 슬롯: 0

    } // permit scope 소멸 시 슬롯 자동 반환됨.

    print(tce("  범위 종료 후 실행 중: "), gate.active());
    //  범위 종료 후 실행 중: 0

    print(endl, tce("--- [2단계] 4개 스레드 동시 실행 (동시에 2개까지만) ---"));

    std::mutex print_mutex; // 콘솔 출력 보호
    std::vector<std::thread> workers;

    for (int i = 1; i <= 4; ++i) {
        workers.emplace_back([&gate, &print_mutex, i, tce]() {
            auto permit = gate.acquire_for(2000ms); // 슬롯이 날 때까지 최대 2초 대기

            if (!permit) {
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cout << tce("  작업 ") << i << tce(": 거부\n");
                return;
            }
            {
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cout
                    << tce("  작업 ") << i
                    << tce(" 시작 (실행 중: ") << gate.active() << tce(")\n");
            }

            std::this_thread::sleep_for(150ms); // 작업 수행을 하는 부분을 테스트를 위해 150ms sleep 으로 대체
        }); // permit scope 소멸 시 슬롯 반환
    }
    // 작업 2 시작(실행 중 : 2) 
    // 작업 1 시작(실행 중 : 2)
    // 작업 4 시작(실행 중 : 2)  
    // 작업 3 시작(실행 중 : 2)
    // 
    // 스레드 1, 2, 3, 4가 모두 실행 큐에 있지만,
    //     OS 스케줄러가** 어느 순서로 실행할지 결정**
    // 
    //     (X) 생성 순서대로 실행되는 것이 보장되지 않음
    //     (X) 실행 시간도 예측 불가능
    // 
    //     그 결과 :
    // -작업2가 먼저 acquire()를 호출할 수도 있고
    //     - 작업1이 먼저 호출할 수도 있음

// 시간 →
// 작업1:  [acquire] → 슬롯획득 → 출력(실행중:2) → sleep(150ms) → 슬롯반환 |
// 작업2: [acquire] → 슬롯획득 → 출력(실행중:2) → sleep(150ms) → 슬롯반환 | 
// 작업3:   [acquire] → 대기(슬롯없음)... 대기... →                        | → 슬롯획득 → 출력(실행중:2) → sleep(150ms) → 슬롯반환 |
// 작업4:   [acquire] → 대기(슬롯없음)... 대기... →                       | → 슬롯획득 → 출력(실행중:2) → sleep(150ms) → 슬롯반환 |
//        ├─────────────────────── 150ms ─────────────────────────────────┤

    // 모든 쓰레드가 종료될 때까지 대기      
    for (auto& worker : workers) {
        worker.join();
    }
    // 작업 1, 2가 먼저 시작되고, 150ms 후 작업 3, 4가 시작됨. (실행 중 수는 항상 2 이하)
}


