#include <algorithm>
#include <chrono>
#include <utility>

#include "mino/core/resilience/token_bucket.hpp"

namespace mino::core::resilience {

    namespace {

        token_bucket::config_t normalize_config(token_bucket::config_t cfg)
        {
            if (cfg.capacity == 0)
            {
                cfg.capacity = 1;
            }
            if (cfg.refill_tokens == 0)
            {
                cfg.refill_tokens = 1;
            }
            if (cfg.refill_period <= std::chrono::milliseconds::zero())
            {
                cfg.refill_period = std::chrono::milliseconds{ 1 };
            }
            return cfg;
        }

        limiter_clock_fn_t normalize_clock(limiter_clock_fn_t clock_fn)
        {
            if (clock_fn)
            {
                return clock_fn;
            }
            return []() { return limiter_clock_t::now(); };
        }

        // 대기자 수를 관리하는 RAII (mutex 를 잡은 상태에서 생성/소멸되어야 한다)
        struct waiter_scope
        {
            std::size_t& count;
            std::condition_variable& cv;

            waiter_scope(std::size_t& c, std::condition_variable& v) : count(c), cv(v) { ++count; }
            ~waiter_scope()
            {
                --count;
                cv.notify_all(); // 소멸자에서 대기 중인 스레드에게 알림
            }
            waiter_scope(const waiter_scope&) = delete;
            waiter_scope& operator=(const waiter_scope&) = delete;
        };

    }

    token_bucket::token_bucket(config_t cfg, limiter_clock_fn_t clock_fn)
        : config_(normalize_config(std::move(cfg)))
        , clock_fn_(normalize_clock(std::move(clock_fn)))
        , tokens_(config_.capacity)
        , last_refill_(clock_fn_())
        , closed_(false)
        , waiters_(0)
    {
    }

    token_bucket::token_bucket(config_t cfg)
        : token_bucket(std::move(cfg), limiter_clock_fn_t{})
    {
    }

    token_bucket::~token_bucket()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
        cv_.wait(lock, [this]() { return waiters_ == 0; });
    }


    void token_bucket::refill_locked(limiter_time_point_t now) const
    {
        if (now <= last_refill_)
        {
            return; // 시간이 흐르지 않았거나 시계가 거꾸로 감
        }

        // 이미 가득 찬 상태에서는 경과 시간을 누적하지 않는다.
        if (tokens_ >= config_.capacity)
        {
            last_refill_ = now;
            return;
        }

        const auto period = std::chrono::duration_cast<limiter_clock_t::duration>(config_.refill_period);
        const auto periods = (now - last_refill_) / period; // 경과한 완전한 주기 수
        if (periods <= 0)
        {
            return;
        }

        // 가득 차기까지 필요한 주기 수와 비교하여 오버플로 없이 보충한다.
        const std::size_t needed = config_.capacity - tokens_;
        const std::size_t periods_to_full = (needed + config_.refill_tokens - 1) / config_.refill_tokens;

        if (static_cast<std::size_t>(periods) >= periods_to_full)
        {
            tokens_ = config_.capacity;
            last_refill_ = now;
        }
        else
        {
            tokens_ += static_cast<std::size_t>(periods) * config_.refill_tokens;
            last_refill_ += period * periods; // 남은 부분 주기는 다음 계산에 이월
        }
    }



    limiter_clock_t::duration token_bucket::wait_time_locked(std::size_t n, limiter_time_point_t now) const
    {
        if (tokens_ >= n)
        {
            return limiter_clock_t::duration::zero();
        }

        const std::size_t needed = n - tokens_;
        const std::size_t periods = (needed + config_.refill_tokens - 1) / config_.refill_tokens;
        const auto period = std::chrono::duration_cast<limiter_clock_t::duration>(config_.refill_period);

        const auto wait = period * static_cast<limiter_clock_t::rep>(periods) - (now - last_refill_);
        return (wait > limiter_clock_t::duration::zero()) ? wait : limiter_clock_t::duration::zero();
    }

    bool token_bucket::try_acquire(std::size_t n)
    {
        if (n == 0)
        {
            return true;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || n > config_.capacity)
        {
            return false;
        }

        refill_locked(clock_fn_());
        if (tokens_ >= n)
        {
            tokens_ -= n;
            return true;
        }
        return false;
    }

    bool token_bucket::acquire_for(std::size_t n, std::chrono::milliseconds timeout)
    {
        if (n == 0)
        {
            return true;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        if (closed_ || n > config_.capacity)
        {
            return false;
        }

        waiter_scope scope(waiters_, cv_);
        const auto deadline = clock_fn_() + std::chrono::duration_cast<limiter_clock_t::duration>(timeout);

        for (;;)
        {
            if (closed_)
            {
                return false;
            }

            const auto now = clock_fn_();
            refill_locked(now);
            if (tokens_ >= n)
            {
                tokens_ -= n;
                return true;
            }

            if (now >= deadline)
            {
                return false;
            }

            auto wait = wait_time_locked(n, now);
            const auto remaining = deadline - now;
            if (wait <= limiter_clock_t::duration::zero())
            {
                wait = limiter_clock_t::duration{ 1 };
            }
            cv_.wait_for(lock, (std::min)(wait, remaining));
        }
    }


    std::chrono::milliseconds token_bucket::retry_after(std::size_t n) const
    {
        if (n == 0)
        {
            return std::chrono::milliseconds::zero();
        }
        if (n > config_.capacity)
        {
            return (std::chrono::milliseconds::max)();
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = clock_fn_();
        refill_locked(now);

        // 현재 토큰 수를 기반으로 필요한 추가 토큰 계산
        const std::size_t needed = (tokens_ >= n) ? 0 : n - tokens_;
        const std::size_t periods = (needed + config_.refill_tokens - 1) / config_.refill_tokens;
        const auto period = std::chrono::duration_cast<limiter_clock_t::duration>(config_.refill_period);

        // 마지막 보충 시점 이후 추가로 필요한 시간
        const auto wait = period * static_cast<limiter_clock_t::rep>(periods) - (now - last_refill_);
        const auto result = (wait > limiter_clock_t::duration::zero()) ? wait : limiter_clock_t::duration::zero();

        return std::chrono::ceil<std::chrono::milliseconds>(result);
    }


    std::size_t token_bucket::available() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        refill_locked(clock_fn_());
        return tokens_;
    }

    token_bucket::config_t token_bucket::config() const noexcept
    {
        return config_;
    }

    void token_bucket::reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tokens_ = config_.capacity;
        last_refill_ = clock_fn_();
        cv_.notify_all();
    }

    void token_bucket::close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }

    bool token_bucket::is_closed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

}
