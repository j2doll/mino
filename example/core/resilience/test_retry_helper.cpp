#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>

#include "mino/core/datetime/datetime.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/core/resilience/resilience.hpp"

// -----------------------------------------------------------------------------
// retry_helper 는 일시적인 네트워크 순단, 원격 서비스의 일시적 오류(Transient Fault),
// 데이터베이스 락 충돌 등으로 인해 실패할 가능성이 있는 작업을 자동으로 재시도
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 2. 재시도 헬퍼 (retry_helper) 테스트
// -----------------------------------------------------------------------------
void test_retry_helper() {
    namespace mcs = mino::core::resilience;
    namespace mlog = mino::core::log::tinylog;
    using namespace std::chrono_literals;
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

    int attempt_count = 0;

    // [테스트 용도 함수] 지정된 시도 횟수(success_on_attempt)에서 성공하는 테스트 람다
    auto flaky_func = [&attempt_count](int success_on_attempt) -> bool {
        namespace dtutil = mino::core::datetime::util;
        using time_zone_mode = mino::core::datetime::util::time_zone_mode;
        std::string current_local
            = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");

        attempt_count++;
        auto to_enc = mino::core::string::to_console_encoding;
        auto msg = // current_local
            to_enc(" [작업 실행] 시도 횟수: ") + std::to_string(attempt_count) + "\n";
        std::cout << msg;

        if (attempt_count < success_on_attempt) { // 지정된 시도 횟수(success_on_attempt) 이전에는 실패
            // auto err_string = current_local + std::string(" 네트워크 일시적 오류");
            auto err_string = std::string(" 네트워크 일시적 오류");
            throw std::runtime_error(err_string);
        }
        return true; // 성공 반환. (성공이 되면 retry_helper는 더 이상 재시도하지 않음.)
    };

    // -------------------------------------------------------------
    // 시나리오 1: 사용자 정의 배수(backoff_factor) 및 max_delay 테스트
    // -------------------------------------------------------------
    attempt_count = 0;

    int max_attempts = 5; // 최대 5회 시도
    std::chrono::milliseconds base_delay(100); // 100ms 기본 대기
    bool use_backoff = true; // 지수 백오프 활성화
    double backoff_factor = 1.5; // <-- 1.5배씩 증가 (100ms -> 150ms -> 225ms ...)
    std::chrono::milliseconds max_delay = 500ms; // <-- 최대 대기 상한선 500ms
    int param_success_on_attempt = 3; // 3번째 시도에서 flaky_func 에서 성공하도록 설정 (테스트 용도)

    std::cout << to_enc("\n[테스트 1] 배수 1.5배 적용 재시도:\n");

    bool result1 = mcs::retry_helper::retry(
        max_attempts,              // 최대 5회 재시도를 함 (0이면 무한 재시도)
        base_delay,                // 최초 재시도 시간은 100ms
        use_backoff,               // 지수 백오프 활성화
        logger,                    // 로거
        backoff_factor,            // 백오프 배수(1.5). 재시도 시간: 100ms -> 150ms -> ...
        max_delay,                 // 최대 지연 상한선(500ms). 재시도 시간: ... -> 500ms(max) -> 500ms ...
        flaky_func,                // 실행할 함수
        param_success_on_attempt   // 함수 인자 (테스트 용도)
    );
    // 100ms -> 150ms -> 225ms -> 337ms -> 500ms(최대) -> 500ms -> ... 순으로 대기하며 재시도
    // [1]   -> [2]   -> [3] 성공 (성공 시 더 이상 재시도하지 않음)

    std::cout
        << to_enc("테스트 1 결과: ")
        << (result1 ? to_enc("성공 (SUCCESS)") : to_enc("실패 (FAILED)"))
        << std::endl;

    // -------------------------------------------------------------
    // 시나리오 2: max_attempts = 0 (무한 재시도 모드) 테스트
    // -------------------------------------------------------------
    attempt_count = 0;
    int infinite_mode = 0; // <-- 0을 주어 무한 재시도 활성화
    int succeed_late = 6;  // 6번째 시도에 성공하도록 설정 (유한 5회였으면 실패했을 작업)

    std::cout << to_enc("\n[테스트 2] 무한 재시도 모드 (0 지정):\n");

    bool result2 = mcs::retry_helper::retry(
        infinite_mode,             // max_attempts = 0 (무한 재시도)
        50ms,                      // 50ms 기본 간격
        true,                      // 백오프 활성화
        logger,
        2.0,                       // 2.0배 증가 (50ms -> 100ms -> 200ms -> ... -> 1000ms(max) )
        1000ms,                    // 최대 1초까지만 대기
        flaky_func,
        succeed_late               // (함수 인자) [테스트 용도] 6번째에 flaky_func 에서 성공
    );

    std::cout
        << to_enc("테스트 2 결과: ")
        << (result2 ? to_enc("성공 (SUCCESS)") : to_enc("실패 (FAILED)"))
        << std::endl;
}



