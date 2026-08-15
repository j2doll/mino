#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>

#include "mino/core/resilience/circuit_breaker.hpp"
#include "mino/core/resilience/paginated_list.hpp"
#include "mino/core/resilience/retry_helper.hpp"

#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// -----------------------------------------------------------------------------
// 1. 페이지네이션 (paginated_list) 테스트
// -----------------------------------------------------------------------------
void test_paginated_list() {
    namespace mcs = mino::core::resilience;
    auto to_enc = mino::core::string::to_console_encoding;

    std::cout << to_enc("\n========================================\n");
    std::cout << to_enc(" [1] Paginated List Test\n");
    std::cout << to_enc("========================================\n");

    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    int page_size = 5;

    // 1페이지를 조회
    //  NOTE: 페이지 크기가 5이므로, 1페이지에는 1~5, 2페이지에는 6~10,
    //   3페이지에는 11~13이 포함됨.
    auto page1 = mcs::to_paginated_list(numbers, 1, page_size);

    // 페이지네이션 정보 출력
    std::cout << to_enc("Total items : ") << page1.total_items << "\n"; // 13
    std::cout << to_enc("Total pages : ") << page1.total_pages << "\n"; // 3        

    // 1페이지 출력
    std::cout << to_enc("Page 1 data : ");
    for (int val : page1.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    // 출력값: 1, 2, 3, 4, 5

    // 3페이지(마지막 페이지) 출력 
    auto page3 = mcs::to_paginated_list(numbers, 3, page_size);
    std::cout << to_enc("Page 3 data : ");
    for (int val : page3.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    // 출력값: 11, 12, 13

    // 범위(1~3)를 벗어난 페이지(99) 요청 시, 방어 코드 동작 확인
    auto page_overflow = mcs::to_paginated_list(numbers, 99, page_size);
    std::cout
        << to_enc("Page 99 요청 -> 실제 적용 페이지: ")
        << page_overflow.current_page << "\n";
    // Page 99 요청 -> 실제 적용 페이지: 3
    //  NOTE: 요청 페이지가 총 페이지 수를 초과했으므로,
    //   실제 적용 페이지는 마지막 페이지(3)로 조정됨.
}

// -----------------------------------------------------------------------------
// 2. 재시도 헬퍼 (retry_helper) 테스트
// -----------------------------------------------------------------------------
void test_retry_helper() {
    namespace mcs = mino::core::resilience;
    namespace mlog = mino::core::log::tinylog;
    auto to_enc = mino::core::string::to_console_encoding;

    std::cout << to_enc("\n========================================\n");
    std::cout << to_enc(" [2] Retry Helper Test\n");
    std::cout << to_enc("========================================\n");

    // tinylog 로거 생성 및 콘솔 싱크 추가
    using console_sink_config = mlog::console_sink_config;
    console_sink_config ccfg;
#ifdef _WIN32
    ccfg.encoding = mlog::encoding_type::cp949;
    ccfg.eol = mlog::eol_type::crlf;
#else
    ccfg.encoding = mlog::encoding_type::utf8;
    ccfg.eol = mlog::eol_type::lf;
#endif
    auto logger = std::make_shared<mlog::logger>("retry_logger");
    logger->add_sink(std::make_shared<mlog::console_sink>("console", ccfg));

    int attempt_count = 0; // 시도 횟수

    // success_on_attempt 번째 시도에서 성공하는 임의의 람다 함수
    auto flaky_func = [&attempt_count](int success_on_attempt) -> bool
    {
        attempt_count++; // 시도 횟수 증가

        auto to_enc = mino::core::string::to_console_encoding;
        std::cout << to_enc("  [작업 실행] 시도 횟수: ") << attempt_count << "\n";

        if (attempt_count < success_on_attempt) { // success_on_attempt 번째 시도 전까지 실패
            throw std::runtime_error("네트워크 일시적 오류");
        }
        return true; // success_on_attempt 번째 시도에서 성공
    };

    attempt_count = 0; // 시도 횟수 초기화

    int max_attempts = 5; // 최대 시도 횟수
    std::chrono::milliseconds base_delay(100); // 100ms 기본 대기 시간
    bool use_backoff = true; // 지수 백오프 사용
    // NOTE: 지수 백오프를 사용하면, 첫 번째 실패 후 100ms, 두 번째 실패 후 200ms,
    //  세 번째 실패 후 400ms, 네 번째 실패 후 800ms 대기 후 재시도.
    int param_success_on_attempt = 3; // 3번째 시도에서 성공하도록 설정

    std::cout
        << to_enc("-> ") << param_success_on_attempt
        << to_enc("번째 시도 시 성공하는 작업 재시도 수행:\n");
    // -> 3번째 시도 시 성공하는 작업 재시도 수행:

    bool result = mcs::retry_helper::retry(
        max_attempts, // 최대 시도 횟수: 5
        base_delay,   // 기본 대기 시간: 100ms
        use_backoff,  // 지수 백오프(Exponential Backoff) 사용: true
        logger,       // tinylog 로거 객체 전달
        flaky_func,   // 재시도할 작업 람다 함수
        param_success_on_attempt // flaky_func의 인자(success_on_attempt)
    );
    // 
    // [작업 실행] 시도 횟수 : 1
    // [2026 - 08 - 15 01:45 : 44.255][retry_logger][WRN] Attempt 1 failed : 네트워크 일시적 오류
    // 
    // [작업 실행] 시도 횟수 : 2
    // [2026 - 08 - 15 01:45 : 44.365][retry_logger][WRN] Attempt 2 failed : 네트워크 일시적 오류
    // 
    // [작업 실행] 시도 횟수 : 3
    // 

    std::cout
        << to_enc("최종 결과: ")
        << (result ? to_enc("성공 (SUCCESS)") : to_enc("실패 (FAILED)"))
        << std::endl;
    // 최종 결과 : 성공(SUCCESS)
}

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
    print(endl, tce( "--- [3단계] Reset Timeout (600ms) 대기 ---"), endl); 
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

// -----------------------------------------------------------------------------
// main 함수
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    auto to_enc = mino::core::string::to_console_encoding;

    try {
        test_paginated_list();
        test_retry_helper();
        test_circuit_breaker();
    }
    catch (const std::exception& ex) {
        std::cerr << to_enc("메인 실행 중 예외 발생: ") << ex.what() << "\n";
        return 1;
    }

    std::cout << to_enc("\n모든 테스트가 완료되었습니다.\n");
    return 0;
}
