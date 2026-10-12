#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mino::core::time {

    using timer_id = std::uint64_t;

    class hierarchical_timer_wheel {
    private:
        static constexpr std::size_t TVR_BITS = 8;
        static constexpr std::size_t TVN_BITS = 6;
        static constexpr std::size_t TVR_SIZE = 1ULL << TVR_BITS; // 256 slots
        static constexpr std::size_t TVN_SIZE = 1ULL << TVN_BITS; // 64 slots
        static constexpr std::size_t TVR_MASK = TVR_SIZE - 1;
        static constexpr std::size_t TVN_MASK = TVN_SIZE - 1;
        static constexpr std::size_t WHEEL_NUM = 5;

        struct timer_node {
            timer_id id{ 0 };
            std::uint64_t expires_tick{ 0 };
            std::uint64_t interval_ticks{ 0 };
            bool periodic{ false };
            std::atomic<bool> cancelled{ false };
            std::function<void()> callback;
        };

        using slot_type = std::vector<std::shared_ptr<timer_node>>;

    public:
        explicit hierarchical_timer_wheel(std::chrono::milliseconds tick_interval = std::chrono::milliseconds(10));
        ~hierarchical_timer_wheel();

        hierarchical_timer_wheel(const hierarchical_timer_wheel&) = delete;
        hierarchical_timer_wheel& operator=(const hierarchical_timer_wheel&) = delete;
        hierarchical_timer_wheel(hierarchical_timer_wheel&&) = delete;
        hierarchical_timer_wheel& operator=(hierarchical_timer_wheel&&) = delete;

        /**
         * @brief 1회성 지연 타이머 등록 (O(1))
         */
        timer_id schedule_after(std::chrono::milliseconds delay, std::function<void()> callback);

        /**
         * @brief 주기적 반복 타이머 등록 (O(1))
         */
        timer_id schedule_every(std::chrono::milliseconds interval, std::function<void()> callback);

        /**
         * @brief 등록된 타이머 취소 (O(1))
         */
        bool cancel(timer_id id);

        void start();
        void stop();
        std::size_t active_timer_count() const;

    private:
        timer_id add_timer_internal(std::chrono::milliseconds delay,
            std::chrono::milliseconds interval,
            bool periodic,
            std::function<void()> callback);

        void insert_into_wheel(const std::shared_ptr<timer_node>& node);
        void cascade(std::size_t wheel_idx, std::size_t slot_idx);
        void tick();
        void worker_loop();

        const std::chrono::milliseconds tick_interval_;
        std::atomic<bool> running_;
        std::uint64_t current_tick_;
        timer_id next_timer_id_;

        std::vector<slot_type> wheels_[WHEEL_NUM];
        std::unordered_map<timer_id, std::shared_ptr<timer_node>> active_timers_;

        mutable std::mutex mtx_;
        std::mutex cv_mtx_;
        std::condition_variable cv_;
        std::thread worker_thread_;
    };

} // namespace mino::core::time
