#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>
#include <atomic>

#include "mino/core/resilience/resilience.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

using namespace std::chrono_literals;
using namespace mino::core::resilience;

//---------------------------------------------------
// "레이트 리미터(rate limiter)"는 요청/작업의 속도나 빈도를 제어하는
// 다양한 알고리즘·정책·구성요소를 통칭하는 용어입니다.
// 즉 하위 기능(알고리즘, 파라미터, 동작 모드 등)을 포함하는 상위 개념입니다.
//---------------------------------------------------

// ===================================================
// Part 1. 사용 예제 (Examples)
// ===================================================

// 토큰 버킷: 초당 5개 보충, 최대 버스트 3
void example_token_bucket()
{
    std::cout << "\n[Example: token_bucket]" << std::endl;

    token_bucket::config_t cfg;
    cfg.capacity = 3;
    cfg.refill_tokens = 5;
    cfg.refill_period = 1000ms;
    token_bucket limiter(cfg);

    for (int i = 1; i <= 5; ++i)
    {
        if (limiter.try_acquire())
        {
            std::cout << "  request " << i << ": allowed" << std::endl;
        }
        else
        {
            std::cout << "  request " << i << ": rejected (retry after "
                << limiter.retry_after().count() << " ms)" << std::endl;
        }
    }

    // 토큰이 보충될 때까지 최대 2초 대기
    const bool acquired = limiter.acquire_for(1, 2000ms);
    std::cout << "  acquire_for: " << (acquired ? "allowed" : "timeout") << std::endl;
}

// 슬라이딩 윈도우: 1초 안에 최대 3회
void example_sliding_window()
{
    std::cout << "\n[Example: sliding_window_limiter]" << std::endl;

    sliding_window_limiter::config_t cfg;
    cfg.max_requests = 3;
    cfg.window = 1000ms;
    cfg.algorithm = sliding_window_limiter::algorithm_t::exact;
    sliding_window_limiter limiter(cfg);

    for (int i = 1; i <= 4; ++i)
    {
        std::cout << "  request " << i << ": "
            << (limiter.try_acquire() ? "allowed" : "rejected") << std::endl;
    }
    std::cout << "  used=" << limiter.used()
        << ", retry after " << limiter.retry_after().count() << " ms" << std::endl;
}

// 벌크헤드: 동시에 2개 작업까지만 실행
void example_bulkhead()
{
    std::cout << "\n[Example: bulkhead]" << std::endl;

    bulkhead::config_t cfg;
    cfg.max_concurrent = 2;
    bulkhead gate(cfg);

    std::vector<std::thread> workers;
    for (int i = 1; i <= 4; ++i)
    {
        workers.emplace_back([&gate, i]() {
            auto permit = gate.acquire_for(2000ms); // 슬롯 대기
            if (!permit)
            {
                std::cout << "  worker " << i << ": rejected" << std::endl;
                return;
            }
            std::cout << "  worker " << i << ": running (active=" << gate.active() << ")" << std::endl;
            std::this_thread::sleep_for(100ms);
            }); // permit 소멸 시 자동 반환
    }

    for (auto& worker : workers)
    {
        worker.join();
    }
}

// ===================================================
// Part 2. 단위 테스트 (Unit Tests)
// ===================================================

namespace {
    static int g_failures = 0; // 테스트 실패 횟수

    // 가짜 시계: advance() 로만 시간이 흐른다.
    struct fake_clock
    {
        std::shared_ptr<limiter_time_point_t> now = std::make_shared<limiter_time_point_t>();

        limiter_clock_fn_t fn() const
        {
            auto p = now;
            return [p]() { return *p; };
        }

        void advance(std::chrono::milliseconds ms)
        {
            *now += std::chrono::duration_cast<limiter_clock_t::duration>(ms);
        }
    };
} // namespace

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl;         \
            ++g_failures;                                                             \
        }                                                                             \
    } while (0)
// CHECK()는 조건이 false일 때 실패 메시지를 출력하고 g_failures를 증가시킵니다.

void test_token_bucket_basic()
{
    fake_clock clk;
    token_bucket::config_t cfg;
    cfg.capacity = 3;
    cfg.refill_tokens = 1;
    cfg.refill_period = 1000ms;
    token_bucket tb(cfg, clk.fn());

    CHECK(tb.available() == 3);
    CHECK(tb.try_acquire());
    CHECK(tb.try_acquire());
    CHECK(tb.try_acquire());
    CHECK(!tb.try_acquire());
    CHECK(tb.retry_after() == 1000ms);

    clk.advance(400ms);
    CHECK(tb.retry_after() == 600ms);
    CHECK(!tb.try_acquire());

    clk.advance(600ms);
    CHECK(tb.try_acquire());
    CHECK(!tb.try_acquire());

    clk.advance(10000ms); // 오래 지나도 capacity 를 넘지 않는다.
    CHECK(tb.available() == 3);

    CHECK(!tb.try_acquire(4)); // capacity 초과 요청은 영원히 불가
    CHECK(tb.retry_after(4) == (std::chrono::milliseconds::max)());
    CHECK(tb.try_acquire(0));
    CHECK(tb.available() == 3); // 실패한 요청은 토큰을 소모하지 않는다.
}

void test_token_bucket_partial_period()
{
    fake_clock clk;
    token_bucket::config_t cfg;
    cfg.capacity = 5;
    cfg.refill_tokens = 2;
    cfg.refill_period = 500ms;
    token_bucket tb(cfg, clk.fn());

    CHECK(tb.try_acquire(5));
    CHECK(tb.available() == 0);

    clk.advance(500ms);
    CHECK(tb.available() == 2);
    clk.advance(250ms); // 부분 주기는 이월된다.
    CHECK(tb.available() == 2);
    clk.advance(250ms);
    CHECK(tb.available() == 4);
}

void test_token_bucket_config_normalize()
{
    token_bucket::config_t cfg;
    cfg.capacity = 0;
    cfg.refill_tokens = 0;
    cfg.refill_period = 0ms;
    token_bucket tb(cfg);
    CHECK(tb.config().capacity == 1);
    CHECK(tb.config().refill_tokens == 1);
    CHECK(tb.config().refill_period == 1ms);
}

void test_token_bucket_blocking()
{
    token_bucket::config_t cfg;
    cfg.capacity = 1;
    cfg.refill_tokens = 1;
    cfg.refill_period = 100ms;
    token_bucket tb(cfg);

    CHECK(tb.try_acquire());
    CHECK(tb.acquire_for(1, 1000ms)); // 약 100ms 후 보충되어 성공
    CHECK(!tb.acquire_for(1, 10ms));  // 시간 내에 보충되지 않아 실패
}

void test_token_bucket_close_wakes_waiter()
{
    token_bucket::config_t cfg;
    cfg.capacity = 1;
    cfg.refill_tokens = 1;
    cfg.refill_period = 60000ms;
    token_bucket tb(cfg);
    CHECK(tb.try_acquire());

    std::atomic<bool> result{ true };
    const auto begin = std::chrono::steady_clock::now();
    std::thread t([&]() { result = tb.acquire_for(1, 10000ms); });
    std::this_thread::sleep_for(100ms);
    tb.close();
    t.join();
    const auto elapsed = std::chrono::steady_clock::now() - begin;

    CHECK(!result.load());
    CHECK(elapsed < 3s);
    CHECK(tb.is_closed());
    CHECK(!tb.try_acquire());
}

void test_sliding_window_exact()
{
    fake_clock clk;
    sliding_window_limiter::config_t cfg;
    cfg.max_requests = 3;
    cfg.window = 1000ms;
    cfg.algorithm = sliding_window_limiter::algorithm_t::exact;
    sliding_window_limiter lim(cfg, clk.fn());

    CHECK(lim.try_acquire());
    CHECK(lim.try_acquire());
    CHECK(lim.try_acquire());
    CHECK(!lim.try_acquire());
    CHECK(lim.used() == 3);
    CHECK(lim.retry_after() == 1000ms);

    clk.advance(400ms);
    CHECK(!lim.try_acquire());
    CHECK(lim.retry_after() == 600ms);

    clk.advance(600ms); // t = 1000ms: 처음 3건이 윈도우를 벗어남
    CHECK(lim.used() == 0);
    CHECK(lim.try_acquire(3));
    CHECK(!lim.try_acquire());

    CHECK(!lim.try_acquire(4));
    CHECK(lim.retry_after(4) == (std::chrono::milliseconds::max)());

    lim.reset();
    CHECK(lim.used() == 0);
}

void test_sliding_window_exact_staggered()
{
    fake_clock clk;
    sliding_window_limiter::config_t cfg;
    cfg.max_requests = 2;
    cfg.window = 1000ms;
    sliding_window_limiter lim(cfg, clk.fn());

    CHECK(lim.try_acquire());   // t = 0
    clk.advance(300ms);
    CHECK(lim.try_acquire());   // t = 300
    CHECK(!lim.try_acquire());
    CHECK(lim.retry_after() == 700ms);  // t = 1000 에 첫 기록이 만료

    clk.advance(700ms);         // t = 1000
    CHECK(lim.try_acquire());
    CHECK(!lim.try_acquire());
    CHECK(lim.retry_after() == 300ms);  // t = 1300 에 두 번째 기록이 만료
}

void test_sliding_window_approximate()
{
    fake_clock clk;
    sliding_window_limiter::config_t cfg;
    cfg.max_requests = 10;
    cfg.window = 1000ms;
    cfg.algorithm = sliding_window_limiter::algorithm_t::approximate;
    sliding_window_limiter lim(cfg, clk.fn());

    for (int i = 0; i < 10; ++i)
    {
        CHECK(lim.try_acquire());
    }
    CHECK(!lim.try_acquire());
    CHECK(lim.retry_after() == 1000ms);

    clk.advance(1000ms); // 새 윈도우: 이전 10건이 100% 가중
    CHECK(!lim.try_acquire());

    clk.advance(500ms);  // 이전 윈도우 가중치 50% => 추정 5건
    int granted = 0;
    for (int i = 0; i < 10; ++i)
    {
        if (lim.try_acquire())
        {
            ++granted;
        }
    }
    CHECK(granted == 5);

    clk.advance(5000ms); // 여러 윈도우가 지나면 완전히 초기화
    CHECK(lim.used() == 0);
    CHECK(lim.try_acquire(10));
}

void test_bulkhead_basic()
{
    bulkhead::config_t cfg;
    cfg.max_concurrent = 2;
    bulkhead bh(cfg);

    auto p1 = bh.try_acquire();
    auto p2 = bh.try_acquire();
    CHECK(p1);
    CHECK(p2);
    CHECK(bh.active() == 2);
    CHECK(bh.available() == 0);

    auto p3 = bh.try_acquire();
    CHECK(!p3);

    p1.release();
    p1.release(); // 중복 호출 안전
    CHECK(bh.active() == 1);

    {
        auto p4 = bh.try_acquire();
        CHECK(p4);
        CHECK(bh.active() == 2);
    } // RAII 반환
    CHECK(bh.active() == 1);

    bulkhead::permit moved = std::move(p2); // 이동
    CHECK(!p2);
    CHECK(moved);
    CHECK(bh.active() == 1);
    moved.release();
    CHECK(bh.active() == 0);

    CHECK(bh.acquire_for(0ms)); // 슬롯이 비어 있으면 즉시 성공
}

void test_bulkhead_blocking()
{
    bulkhead::config_t cfg;
    cfg.max_concurrent = 1;
    bulkhead bh(cfg);

    auto held = bh.try_acquire();
    CHECK(held);

    CHECK(!bh.acquire_for(20ms)); // 대기 시간 초과

    std::atomic<bool> acquired{ false };
    std::thread t([&]() {
        auto p = bh.acquire_for(3000ms);
        acquired = static_cast<bool>(p);
        });
    std::this_thread::sleep_for(100ms);
    held.release();
    t.join();
    CHECK(acquired.load());
    CHECK(bh.active() == 0);
}

void test_bulkhead_close_wakes_waiter()
{
    bulkhead::config_t cfg;
    cfg.max_concurrent = 1;
    bulkhead bh(cfg);

    auto held = bh.try_acquire();
    std::atomic<bool> acquired{ true };
    const auto begin = std::chrono::steady_clock::now();
    std::thread t([&]() { acquired = static_cast<bool>(bh.acquire_for(10000ms)); });
    std::this_thread::sleep_for(100ms);
    bh.close();
    t.join();

    CHECK(!acquired.load());
    CHECK(std::chrono::steady_clock::now() - begin < 3s);
    CHECK(!bh.try_acquire());
}

void test_bulkhead_concurrency_limit()
{
    bulkhead::config_t cfg;
    cfg.max_concurrent = 3;
    bulkhead bh(cfg);

    std::atomic<int> running{ 0 };
    std::atomic<int> peak{ 0 };
    std::atomic<int> done{ 0 };

    std::vector<std::thread> threads;
    for (int i = 0; i < 12; ++i)
    {
        threads.emplace_back([&]() {
            auto p = bh.acquire_for(5000ms);
            if (!p)
            {
                return;
            }
            const int now_running = ++running;
            int prev = peak.load();
            while (now_running > prev && !peak.compare_exchange_weak(prev, now_running)) {}
            std::this_thread::sleep_for(20ms);
            --running;
            ++done;
            });
    }
    for (auto& t : threads)
    {
        t.join();
    }

    CHECK(done.load() == 12);
    CHECK(peak.load() <= 3);
}

// ===================================================
// Part 3. Main 실행 진입점
// ===================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " Running Examples" << std::endl;
    std::cout << "========================================" << std::endl;
    example_token_bucket();
    example_sliding_window();
    example_bulkhead();

    std::cout << "\n========================================" << std::endl;
    std::cout << " Running Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
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

    if (g_failures == 0)
    {
        std::cout << "\n[PASS] All tests passed successfully!" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "\n[FAIL] Total failures: " << g_failures << std::endl;
        return 1;
    }
}
