#include "mino/core/time/hierarchical_timer_wheel.hpp"

#include <algorithm>
#include <utility>

namespace mino::core::time {

    hierarchical_timer_wheel::hierarchical_timer_wheel(std::chrono::milliseconds tick_interval)
        : tick_interval_(tick_interval),
        running_(false),
        current_tick_(0),
        next_timer_id_(1) {

        wheels_[0].resize(TVR_SIZE);
        for (std::size_t i = 1; i < WHEEL_NUM; ++i) {
            wheels_[i].resize(TVN_SIZE);
        }
        start();
    }

    hierarchical_timer_wheel::~hierarchical_timer_wheel() {
        stop();
    }

    timer_id hierarchical_timer_wheel::schedule_after(std::chrono::milliseconds delay, std::function<void()> callback) {
        return add_timer_internal(delay, std::chrono::milliseconds(0), false, std::move(callback));
    }

    timer_id hierarchical_timer_wheel::schedule_every(std::chrono::milliseconds interval, std::function<void()> callback) {
        return add_timer_internal(interval, interval, true, std::move(callback));
    }

    bool hierarchical_timer_wheel::cancel(timer_id id) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = active_timers_.find(id);
        if (it != active_timers_.end()) {
            it->second->cancelled.store(true, std::memory_order_release);
            active_timers_.erase(it);
            return true;
        }
        return false;
    }

    void hierarchical_timer_wheel::start() {
        if (!running_.exchange(true, std::memory_order_acq_rel)) {
            worker_thread_ = std::thread(&hierarchical_timer_wheel::worker_loop, this);
        }
    }

    void hierarchical_timer_wheel::stop() {
        if (running_.exchange(false, std::memory_order_acq_rel)) {
            cv_.notify_all();
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
    }

    std::size_t hierarchical_timer_wheel::active_timer_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return active_timers_.size();
    }

    timer_id hierarchical_timer_wheel::add_timer_internal(std::chrono::milliseconds delay,
        std::chrono::milliseconds interval,
        bool periodic,
        std::function<void()> callback) {
        std::uint64_t delay_ticks = std::max<std::uint64_t>(1, delay.count() / tick_interval_.count());
        std::uint64_t interval_ticks = interval.count() / tick_interval_.count();

        auto node = std::make_shared<timer_node>();
        node->interval_ticks = interval_ticks;
        node->periodic = periodic;
        node->callback = std::move(callback);

        std::lock_guard<std::mutex> lock(mtx_);
        node->id = next_timer_id_++;
        node->expires_tick = current_tick_ + delay_ticks;

        active_timers_[node->id] = node;
        insert_into_wheel(node);

        return node->id;
    }

    void hierarchical_timer_wheel::insert_into_wheel(const std::shared_ptr<timer_node>& node) {
        std::uint64_t delta = node->expires_tick - current_tick_;
        std::size_t wheel_idx = 0;
        std::size_t slot_idx = 0;

        if (delta < TVR_SIZE) {
            wheel_idx = 0;
            slot_idx = node->expires_tick & TVR_MASK;
        }
        else if (delta < (1ULL << (TVR_BITS + TVN_BITS))) {
            wheel_idx = 1;
            slot_idx = (node->expires_tick >> TVR_BITS) & TVN_MASK;
        }
        else if (delta < (1ULL << (TVR_BITS + 2 * TVN_BITS))) {
            wheel_idx = 2;
            slot_idx = (node->expires_tick >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
        }
        else if (delta < (1ULL << (TVR_BITS + 3 * TVN_BITS))) {
            wheel_idx = 3;
            slot_idx = (node->expires_tick >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
        }
        else {
            wheel_idx = 4;
            slot_idx = (node->expires_tick >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
        }

        wheels_[wheel_idx][slot_idx].push_back(node);
    }

    void hierarchical_timer_wheel::cascade(std::size_t wheel_idx, std::size_t slot_idx) {
        auto& slot = wheels_[wheel_idx][slot_idx];
        if (slot.empty()) {
            return;
        }

        std::vector<std::shared_ptr<timer_node>> items = std::move(slot);
        slot.clear();

        for (auto& item : items) {
            if (!item->cancelled.load(std::memory_order_relaxed)) {
                insert_into_wheel(item);
            }
        }
    }

    void hierarchical_timer_wheel::tick() {
        std::vector<std::shared_ptr<timer_node>> to_execute;

        {
            std::lock_guard<std::mutex> lock(mtx_);

            std::size_t idx0 = current_tick_ & TVR_MASK;

            // Level 0 휠이 회전할 때 상위 휠에서 하위 휠로 연쇄 강하(Cascade)
            if (idx0 == 0 && current_tick_ > 0) {
                std::size_t idx1 = (current_tick_ >> TVR_BITS) & TVN_MASK;
                cascade(1, idx1);
                if (idx1 == 0) {
                    std::size_t idx2 = (current_tick_ >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
                    cascade(2, idx2);
                    if (idx2 == 0) {
                        std::size_t idx3 = (current_tick_ >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
                        cascade(3, idx3);
                        if (idx3 == 0) {
                            std::size_t idx4 = (current_tick_ >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
                            cascade(4, idx4);
                        }
                    }
                }
            }

            auto& slot = wheels_[0][idx0];
            for (auto& node : slot) {
                if (!node->cancelled.load(std::memory_order_relaxed)) {
                    to_execute.push_back(node);
                }
            }
            slot.clear();

            ++current_tick_;
        }

        // 락 해제 후 콜백 실행 (콜백 내부에서 schedule/cancel 호출 시의 데드락 방지)
        for (auto& node : to_execute) {
            if (node->cancelled.load(std::memory_order_acquire)) {
                continue;
            }

            if (node->callback) {
                node->callback();
            }

            // 반복 타이머 재배치
            if (node->periodic && !node->cancelled.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!node->cancelled.load(std::memory_order_relaxed)) {
                    node->expires_tick = current_tick_ + node->interval_ticks;
                    insert_into_wheel(node);
                }
            }
            else {
                std::lock_guard<std::mutex> lock(mtx_);
                active_timers_.erase(node->id);
            }
        }
    }

    void hierarchical_timer_wheel::worker_loop() {
        auto next_tick_time = std::chrono::steady_clock::now();

        while (running_.load(std::memory_order_acquire)) {
            next_tick_time += tick_interval_;

            std::unique_lock<std::mutex> lock(cv_mtx_);
            cv_.wait_until(lock, next_tick_time, [this]() {
                return !running_.load(std::memory_order_acquire);
                });

            if (!running_.load(std::memory_order_acquire)) {
                break;
            }

            tick();
        }
    }

} // namespace mino::core::time
