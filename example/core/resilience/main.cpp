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

extern void test_paginated_list();
extern void test_retry_helper();
extern void test_circuit_breaker(); 
extern void test_token_bucket();
extern void test_sliding_window_limiter();
extern void test_bulkhead();
extern void test_combined();

extern void example_token_bucket();
extern void example_sliding_window();
extern void example_bulkhead();

extern void test_token_bucket_basic();
extern void test_token_bucket_partial_period();
extern void test_token_bucket_config_normalize();
extern void test_token_bucket_blocking();
extern void test_token_bucket_close_wakes_waiter();
extern void test_sliding_window_exact();
extern void test_sliding_window_exact_staggered();
extern void test_sliding_window_approximate();
extern void test_bulkhead_basic();
extern void test_bulkhead_blocking();
extern void test_bulkhead_close_wakes_waiter();
extern void test_bulkhead_concurrency_limit();

// -----------------------------------------------------------------------------
// main 함수
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    namespace mcstr = mino::core::string;
    namespace mcsp = mino::core::string::print;
    auto tcenc = mcstr::to_console_encoding;

    try {
        test_paginated_list();
        test_retry_helper();
        test_circuit_breaker();
        test_token_bucket();
        test_sliding_window_limiter();
        test_bulkhead();
        test_combined();

        {
            example_token_bucket();
            example_sliding_window();
            example_bulkhead();
        }

        {
            test_token_bucket_basic();
            test_token_bucket_partial_period();
            test_token_bucket_config_normalize();
            test_token_bucket_blocking();
            test_token_bucket_close_wakes_waiter();
            test_sliding_window_exact();
            test_sliding_window_exact_staggered();
            test_sliding_window_approximate();
            test_bulkhead_basic();
            test_bulkhead_blocking();
            test_bulkhead_close_wakes_waiter();
            test_bulkhead_concurrency_limit();
        }
    }
    catch (const std::exception& ex) {
        mcsp::eprintln("메인 실행 중 예외 발생: {}", ex.what());
        return 1;
    }

    mcsp::println("... 모든 테스트가 완료되었습니다.");
    return 0;
}
