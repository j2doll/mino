#include <chrono>
#include <utility>

#include "mino/core/resilience/bulkhead.hpp"

namespace mino::core::resilience {

    namespace {

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

    // ---- permit ----

    bulkhead::permit::permit(permit&& other) noexcept
        : owner_(other.owner_)
    {
        other.owner_ = nullptr;
    }

    bulkhead::permit& bulkhead::permit::operator=(permit&& other) noexcept
    {
        if (this != &other)
        {
            release();
            owner_ = other.owner_;
            other.owner_ = nullptr;
        }
        return *this;
    }

    bulkhead::permit::~permit()
    {
        release();
    }

    void bulkhead::permit::release() noexcept
    {
        if (owner_ != nullptr)
        {
            owner_->release_slot();
            owner_ = nullptr;
        }
    }

    // ---- bulkhead ----

    bulkhead::bulkhead(config_t cfg)
        : config_(cfg)
        , active_(0)
        , closed_(false)
        , waiters_(0)
    {
        if (config_.max_concurrent == 0)
        {
            config_.max_concurrent = 1;
        }
    }

    bulkhead::~bulkhead()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
        cv_.wait(lock, [this]() { return waiters_ == 0; });
    }

    bulkhead::permit bulkhead::try_acquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || active_ >= config_.max_concurrent)
        {
            return permit();
        }

        ++active_;
        return permit(this);
    }

    bulkhead::permit bulkhead::acquire_for(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (closed_)
        {
            return permit();
        }

        if (active_ < config_.max_concurrent)
        {
            ++active_;
            return permit(this);
        }

        if (timeout <= std::chrono::milliseconds::zero())
        {
            return permit();
        }

        waiter_scope scope(waiters_, cv_);
        cv_.wait_for(lock, timeout, [this]() { return closed_ || active_ < config_.max_concurrent; });

        if (closed_ || active_ >= config_.max_concurrent)
        {
            return permit();
        }

        ++active_;
        return permit(this);
    }

    std::size_t bulkhead::active() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return active_;
    }

    std::size_t bulkhead::available() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.max_concurrent - active_;
    }

    bulkhead::config_t bulkhead::config() const noexcept
    {
        return config_;
    }

    void bulkhead::close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }

    bool bulkhead::is_closed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    void bulkhead::release_slot() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_ > 0)
        {
            --active_;
        }
        cv_.notify_all();
    }

}
