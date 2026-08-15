#include <algorithm>

#include "mino/core/log/tinylog/tinylog.hpp"

#include "mino/core/schedule/task/task_scheduler.hpp"

namespace mino::core::schedule::task {

    task_scheduler::task_scheduler()
        : next_id_(1), is_running_(false), logger_(nullptr) {
    }

    task_scheduler::~task_scheduler() {
        stop();
    }

    void task_scheduler::set_logger(std::shared_ptr< mino::core::log::tinylog::logger > logger) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        logger_ = logger;
    }

    uint64_t task_scheduler::add_task(std::unique_ptr<period_strategy> strategy,
        std::function<void()> work,
        std::optional<std::string> description) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        auto now = std::chrono::system_clock::now();
        auto next_run = strategy->get_next_run_time(now);

        uint64_t current_id = next_id_++;
        tasks_.push_back({ current_id, description, std::move(strategy), work, next_run });

        log_info("Task registered successfully. ID: " + std::to_string(current_id));
        return current_id;
    }

    bool task_scheduler::remove_task(uint64_t task_id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        auto it = std::remove_if(tasks_.begin(), tasks_.end(), [task_id](const scheduled_task& task) {
            return task.task_id == task_id;
            });

        if (it != tasks_.end()) {
            tasks_.erase(it, tasks_.end());
            log_info("Task removed successfully. ID: " + std::to_string(task_id));
            return true;
        }

        log_error("Failed to remove task. ID not found: " + std::to_string(task_id));
        return false;
    }

    void task_scheduler::start() {
        if (is_running_) return;
        is_running_ = true;

        scheduler_thread_ = std::thread(&task_scheduler::run_loop, this);
        worker_thread_ = std::thread(&task_scheduler::worker_loop, this);

        log_info("Scheduler and Worker thread started.");
    }

    void task_scheduler::stop() {
        if (!is_running_) return;

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            is_running_ = false;
        }

        queue_cv_.notify_all();

        if (scheduler_thread_.joinable()) {
            scheduler_thread_.join();
        }
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        log_info("Scheduler stopped safely.");
    }

    // 신규: 현재 등록된 task 목록을 안전하게 복사하여 반환
    std::vector<task_scheduler::task_info> task_scheduler::list_tasks() {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        std::vector<task_info> result;
        result.reserve(tasks_.size());
        for (const auto& t : tasks_) {
            result.push_back({ t.task_id, t.description, t.next_run_time });
        }
        return result;
    }

    void task_scheduler::run_loop() {
        while (is_running_) {
            auto now = std::chrono::system_clock::now();

            {
                std::lock_guard<std::mutex> lock(scheduler_mutex_);
                for (auto& task : tasks_) {
                    if (now >= task.next_run_time) {
                        {
                            std::lock_guard<std::mutex> q_lock(queue_mutex_);
                            task_queue_.push(task.work);
                        }
                        queue_cv_.notify_one();

                        task.next_run_time = task.strategy->get_next_run_time(now);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    void task_scheduler::worker_loop() {
        while (true) {
            std::function<void()> current_work;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this]() {
                    return !is_running_ || !task_queue_.empty();
                    });

                if (!is_running_ && task_queue_.empty()) {
                    break;
                }

                current_work = std::move(task_queue_.front());
                task_queue_.pop();
            }

            if (current_work) {
                try {
                    current_work();
                }
                catch (...) {
                    log_error("Exception occurred during task execution.");
                }
            }
        }
    }

    void task_scheduler::log_info(const std::string& msg) {
        if (logger_) {
            logger_->info(msg);
        }
    }

    void task_scheduler::log_error(const std::string& msg) {
        if (logger_) {
            logger_->error(msg);
        }
    }

}  
