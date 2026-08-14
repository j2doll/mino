#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>

// 모듈 헤더 포함
#include "mino/core/resilience/circuit_breaker.hpp"
#include "mino/core/resilience/paginated_list.hpp"
#include "mino/core/resilience/retry_helper.hpp"
#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/log/tinylog/logger.hpp"

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

    // 1페이지 조회
    auto page1 = mcs::to_paginated_list(numbers, 1, page_size);

    // 페이지네이션 정보 출력
    std::cout << to_enc("Total items : ") << page1.total_items << "\n";
    std::cout << to_enc("Total pages : ") << page1.total_pages << "\n";

    // 1페이지 데이터 출력
    std::cout << to_enc("Page 1 data : ");
    for (int val : page1.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    // 3페이지 조회 (마지막 페이지)
    auto page3 = mcs::to_paginated_list(numbers, 3, page_size);
    std::cout << to_enc("Page 3 data : ");
    for (int val : page3.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    // 범위를 벗어난 페이지 요청 시 방어 코드 동작 확인
    auto page_overflow = mcs::to_paginated_list(numbers, 99, page_size);
    std::cout
        << to_enc("Page 99 요청 -> 실제 적용 페이지: ")
        << page_overflow.current_page << "\n";
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
        param_success_on_attempt // flaky_func의 인자 success_on_attempt
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
    // 최종 결과 : 성공(SUCCESS)

    std::cout
        << to_enc("최종 결과: ")
        << (result ? to_enc("성공 (SUCCESS)") : to_enc("실패 (FAILED)"))
        << std::endl;
}

// -----------------------------------------------------------------------------
// 3. 써킷 브레이커 (circuit_breaker) 테스트
// -----------------------------------------------------------------------------
void test_circuit_breaker() {
    namespace mcs = mino::core::resilience;
    auto to_enc = mino::core::string::to_console_encoding;
    using namespace std::chrono_literals;

    std::cout << to_enc("\n========================================\n");
    std::cout << to_enc(" [3] Circuit Breaker Test\n");
    std::cout << to_enc("========================================\n");

    // 빠른 테스트를 위한 설정
    mcs::circuit_breaker::config_t config;
    config.failure_threshold = 3; // 3회 연속 실패 시 Open
    config.reset_timeout = 500ms; // 0.5초 후 Half-Open으로 전환
    config.half_open_success_threshold = 2; // Half-Open에서 2회 연속 성공 시 Closed로 복구

    mcs::circuit_breaker cb(config); // 써킷 브레이커 객체 생성

    std::cout
        << to_enc("초기 써킷 상태: ")
        << state_to_string(cb.current_state())
        << std::endl << std::endl;

    // 1. 의도적 실패를 유도하여 Open 상태로 만들기
    std::cout << to_enc("--- [1단계] 연속 3회 실패 발생 시키기 ---\n");
    for (int i = 1; i <= 3; ++i) {
        cb.execute([i, &to_enc]() {
            std::cout << to_enc("  호출 ") << i << to_enc(" 시도 중... 예외 발생!\n");
            throw std::runtime_error("DB 연결 실패");
        });
        std::cout << to_enc("  -> 현재 상태: ") << state_to_string(cb.current_state()) << "\n";
    }

    // 2. Open 상태에서 차단되는지 확인
    std::cout << to_enc("\n--- [2단계] Open 상태에서 차단 여부 확인 ---\n");
    cb.execute([&to_enc]() {
        std::cout << to_enc("  이 문장은 출력되지 않아야 합니다.\n");
    });
    std::cout << to_enc("  차단 후 상태: ") << state_to_string(cb.current_state()) << "\n";

    // 3. reset_timeout 대기 후 Half-Open 진입 확인
    std::cout << to_enc("\n--- [3단계] Reset Timeout (600ms) 대기 ---\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // 4. Half-Open 상태에서 성공을 쌓아 Closed로 복구
    std::cout
        << to_enc("--- [4단계] Half-Open 복구 시도 (연속 2회 성공 필요) ---")
        << std::endl;

    // 첫 번째 성공
    cb.execute([&to_enc]() {
        std::cout << to_enc("  Half-Open 첫 번째 성공 호출\n");
    });
    std::cout
        << to_enc("  -> 상태: ")
        << state_to_string(cb.current_state())
        << std::endl;

    // 두 번째 성공
    cb.execute([&to_enc]() {
        std::cout << to_enc("  Half-Open 두 번째 성공 호출\n");
    });
    std::cout
        << to_enc("  -> 최종 복구 상태: ")
        << state_to_string(cb.current_state())
        << std::endl;
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
