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

using namespace std::chrono_literals;
using namespace mino::core::resilience;

// 토큰 버킷: 초당 5개 보충, 최대 버스트 3
void example_token_bucket()
{
    std::cout << "[token_bucket]" << std::endl;

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
    std::cout << "[sliding_window_limiter]" << std::endl;

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
    std::cout << "[bulkhead]" << std::endl;

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

