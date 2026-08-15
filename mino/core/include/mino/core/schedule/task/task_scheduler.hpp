#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <optional>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "mino/core/log/tinylog/tinylog_fwd.hpp"

#include "mino/core/schedule/task/period_strategy.hpp"

namespace mino::core::schedule::task {

    struct  scheduled_task {
        uint64_t task_id;
        std::optional<std::string> description;
        std::unique_ptr<period_strategy> strategy;
        std::function<void()> work;
        std::chrono::system_clock::time_point next_run_time;
    };

    class  task_scheduler {
    public:
        task_scheduler();
        ~task_scheduler();

        void set_logger(std::shared_ptr< mino::core::log::tinylog::logger > logger);

        uint64_t add_task(std::unique_ptr<period_strategy> strategy,
            std::function<void()> work,
            std::optional<std::string> description = std::nullopt);

        bool remove_task(uint64_t task_id);
        void start();
        void stop();

        // 신규: task 목록을 읽기 전용으로 안전하게 반환
        struct task_info {
            uint64_t task_id;
            std::optional<std::string> description;
            std::chrono::system_clock::time_point next_run_time;
        };

        std::vector<task_info> list_tasks();

    private:
        void run_loop();
        void worker_loop();
        void log_info(const std::string& msg);
        void log_error(const std::string& msg);

        std::vector<scheduled_task> tasks_;
        uint64_t next_id_;
        std::thread scheduler_thread_;
        bool is_running_;
        std::mutex scheduler_mutex_;

        std::thread worker_thread_;
        std::queue<std::function<void()>> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;

        std::shared_ptr< mino::core::log::tinylog::logger > logger_;
    };

}  
