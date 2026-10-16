#include <algorithm>
#include <chrono>
#include <utility>

#include "mino/core/resilience/sliding_window_limiter.hpp"

namespace mino::core::resilience {

    namespace {

        sliding_window_limiter::config_t normalize_config(sliding_window_limiter::config_t cfg)
        {
            if (cfg.max_requests == 0)
            {
                cfg.max_requests = 1;
            }
            if (cfg.window <= std::chrono::milliseconds::zero())
            {
                cfg.window = std::chrono::milliseconds{ 1 };
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

        limiter_clock_t::duration to_window(const sliding_window_limiter::config_t& cfg)
        {
            return std::chrono::duration_cast<limiter_clock_t::duration>(cfg.window);
        }

    }

    sliding_window_limiter::sliding_window_limiter(config_t cfg, limiter_clock_fn_t clock_fn)
        : config_(normalize_config(std::move(cfg)))
        , clock_fn_(normalize_clock(std::move(clock_fn)))
        , timestamps_()
        , current_count_(0)
        , previous_count_(0)
        , window_start_(clock_fn_())
    {
    }

    sliding_window_limiter::sliding_window_limiter(config_t cfg)
        : sliding_window_limiter(std::move(cfg), limiter_clock_fn_t{})
    {
    }

    void sliding_window_limiter::purge_locked(limiter_time_point_t now) const
    {
        const auto threshold = now - to_window(config_);
        while (!timestamps_.empty() && timestamps_.front() <= threshold)
        {
            timestamps_.pop_front();
        }
    }

    void sliding_window_limiter::advance_window_locked(limiter_time_point_t now) const
    {
        if (now <= window_start_)
        {
            return;
        }

        const auto window = to_window(config_);
        const auto elapsed = now - window_start_;
        if (elapsed < window)
        {
            return;
        }

        const auto passed = elapsed / window; // 지나간 윈도우 수
        previous_count_ = (passed == 1) ? current_count_ : 0; // 2개 이상 지났다면 이전 윈도우는 비어 있음
        current_count_ = 0;
        window_start_ += window * passed;
    }

    double sliding_window_limiter::estimate_locked(limiter_time_point_t now) const
    {
        const auto window = to_window(config_);
        auto elapsed = now - window_start_;
        if (elapsed < limiter_clock_t::duration::zero())
        {
            elapsed = limiter_clock_t::duration::zero();
        }

        const double fraction = static_cast<double>(elapsed.count()) / static_cast<double>(window.count());
        return static_cast<double>(previous_count_) * (1.0 - fraction) + static_cast<double>(current_count_);
    }

    bool sliding_window_limiter::try_acquire(std::size_t n)
    {
        if (n == 0)
        {
            return true;
        }
        if (n > config_.max_requests)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = clock_fn_();

        if (config_.algorithm == algorithm_t::exact)
        {
            purge_locked(now);
            if (timestamps_.size() + n > config_.max_requests)
            {
                return false;
            }
            for (std::size_t i = 0; i < n; ++i)
            {
                timestamps_.push_back(now);
            }
            return true;
        }

        advance_window_locked(now);
        if (estimate_locked(now) + static_cast<double>(n) > static_cast<double>(config_.max_requests))
        {
            return false;
        }
        current_count_ += n;
        return true;
    }

    std::chrono::milliseconds sliding_window_limiter::retry_after(std::size_t n) const
    {
        if (n == 0)
        {
            return std::chrono::milliseconds::zero();
        }
        if (n > config_.max_requests)
        {
            return (std::chrono::milliseconds::max)();
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = clock_fn_();

        if (config_.algorithm == algorithm_t::exact)
        {
            purge_locked(now);
            if (timestamps_.size() + n <= config_.max_requests)
            {
                return std::chrono::milliseconds::zero();
            }

            // 필요한 만큼의 오래된 기록이 윈도우를 벗어날 때까지 기다린다.
            const std::size_t to_free = timestamps_.size() + n - config_.max_requests;
            const auto free_at = timestamps_[to_free - 1] + to_window(config_);
            const auto wait = free_at - now;
            return std::chrono::ceil<std::chrono::milliseconds>(
                (wait > limiter_clock_t::duration::zero()) ? wait : limiter_clock_t::duration::zero());
        }

        advance_window_locked(now);
        if (estimate_locked(now) + static_cast<double>(n) <= static_cast<double>(config_.max_requests))
        {
            return std::chrono::milliseconds::zero();
        }

        const auto remaining = window_start_ + to_window(config_) - now;
        return std::chrono::ceil<std::chrono::milliseconds>(
            (remaining > limiter_clock_t::duration::zero()) ? remaining : limiter_clock_t::duration::zero());
    }

    std::size_t sliding_window_limiter::used() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = clock_fn_();

        if (config_.algorithm == algorithm_t::exact)
        {
            purge_locked(now);
            return timestamps_.size();
        }

        advance_window_locked(now);
        return static_cast<std::size_t>(estimate_locked(now));
    }

    sliding_window_limiter::config_t sliding_window_limiter::config() const noexcept
    {
        return config_;
    }

    void sliding_window_limiter::reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timestamps_.clear();
        current_count_ = 0;
        previous_count_ = 0;
        window_start_ = clock_fn_();
    }

}
