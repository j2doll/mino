#include <any>
#include <chrono>
#include <functional>
#include <thread>

#include "mino/core/resilience/circuit_breaker.hpp"

namespace mino::core::resilience {

    circuit_breaker::circuit_breaker(config_t cfg) noexcept
        : config_(std::move(cfg))
        , state_(state::closed)
        , consecutive_failures_(0)
        , consecutive_successes_(0)
        , last_failure_time_()
    {
    }

    circuit_breaker::~circuit_breaker() = default;

    void circuit_breaker::execute_void(std::function<void()> op)
    {
        pre_execute_check();

        try
        {
            op();
            on_success();
        }
        catch (...)
        {
            on_failure();
            throw;
        }
    }

    std::any circuit_breaker::execute_any(std::function<std::any()> op)
    {
        pre_execute_check();

        try
        {
            std::any res = op();
            on_success();
            return res;
        }
        catch (...)
        {
            on_failure();
            throw;
        }
    }

    std::future<std::any> circuit_breaker::execute_async_any(std::function<std::any()> op)
    {
        // 비동기 실행: 내부에서 동기 execute_any 호출 -> 상태 관리 일관성 보장
        return std::async(std::launch::async, [this, op = std::move(op)]() mutable -> std::any {
            return this->execute_any(std::move(op));
        });
    }

    void circuit_breaker::execute_with_callback_any(std::function<std::any()> op,
                                                    std::function<void(std::optional<std::any>, std::exception_ptr)> cb)
    {
        std::thread([this, op = std::move(op), cb = std::move(cb)]() mutable {
            try
            {
                std::any r = this->execute_any(std::move(op));
                cb(std::optional<std::any>{std::move(r)}, std::exception_ptr{});
            }
            catch (...)
            {
                cb(std::optional<std::any>{}, std::current_exception());
            }
        })
            .detach();
    }

    circuit_breaker::state circuit_breaker::current_state() const noexcept
    {
        return state_.load(std::memory_order_acquire);
    }

    circuit_breaker::config_t circuit_breaker::config() const noexcept
    {
        return config_;
    }

    void circuit_breaker::reset() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.store(state::closed, std::memory_order_release);
        consecutive_failures_.store(0, std::memory_order_release);
        consecutive_successes_.store(0, std::memory_order_release);
        last_failure_time_ = std::chrono::steady_clock::time_point{};
    }

    void circuit_breaker::pre_execute_check()
    {
        state s = state_.load(std::memory_order_acquire);
        if (s == state::open)
        {
            // 만약 reset_timeout이 비어있다면(수동 모드) 자동 전환 없음
            if (!config_.reset_timeout.has_value())
            {
                throw circuit_open_exception();
            }

            auto now = std::chrono::steady_clock::now();

            std::lock_guard<std::mutex> lock(mutex_);
            if (state_.load(std::memory_order_acquire) == state::open)
            {
                // reset_timeout == 0ms 이면 즉시 half_open 허용
                if (now - last_failure_time_ >= config_.reset_timeout.value())
                {
                    state_.store(state::half_open, std::memory_order_release);
                    consecutive_successes_.store(0, std::memory_order_release);
                }
                else
                {
                    throw circuit_open_exception();
                }
            }
        }
    }

    void circuit_breaker::on_success() noexcept
    {
        state s = state_.load(std::memory_order_acquire);
        if (s == state::half_open)
        {
            auto succ = consecutive_successes_.fetch_add(1, std::memory_order_acq_rel) + 1;
            if (succ >= config_.half_open_success_threshold)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.store(state::closed, std::memory_order_release);
                consecutive_failures_.store(0, std::memory_order_release);
                consecutive_successes_.store(0, std::memory_order_release);
            }
        }
        else
        {
            consecutive_failures_.store(0, std::memory_order_release);
        }
    }

    void circuit_breaker::on_failure() noexcept
    {
        auto fails = consecutive_failures_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (fails >= config_.failure_threshold)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.store(state::open, std::memory_order_release);
            last_failure_time_ = std::chrono::steady_clock::now();
            consecutive_successes_.store(0, std::memory_order_release);
        }
        else
        {
            state s = state_.load(std::memory_order_acquire);
            if (s == state::half_open)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.store(state::open, std::memory_order_release);
                last_failure_time_ = std::chrono::steady_clock::now();
                consecutive_failures_.store(fails, std::memory_order_release);
                consecutive_successes_.store(0, std::memory_order_release);
            }
        }
    }

}
