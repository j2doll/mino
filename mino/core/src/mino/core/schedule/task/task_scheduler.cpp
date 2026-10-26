#include "mino/core/schedule/task/task_scheduler.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"
#include <chrono>
#include <iomanip>

namespace mino::core::schedule::task {

    task_scheduler::task_scheduler()
        : time_basis_(time_base::localtime), next_id_(1), is_running_(false) {
    }

    task_scheduler::task_scheduler(time_base time_basis)
        : time_basis_(time_basis), next_id_(1), is_running_(false) {
    }

    task_scheduler::~task_scheduler() {
        stop();
    }

    void task_scheduler::set_logger(std::shared_ptr< mino::core::log::tinylog::logger > logger) {
        logger_ = logger;
    }

    void task_scheduler::set_time_basis(time_base basis) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        time_basis_ = basis;
        log_info(std::string("Time basis changed to: ") +
            (basis == time_base::utc ? "UTC" : "localtime"));
    }

    time_base task_scheduler::get_time_basis() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        return time_basis_;
    }

    uint64_t task_scheduler::add_task(std::unique_ptr<period_strategy> strategy,
        std::function<void()> work,
        std::optional<std::string> description) {

        if (!strategy || !work) {
            log_error("Invalid strategy or work function");
            return 0;
        }

        std::lock_guard<std::mutex> lock(scheduler_mutex_);

        auto current_time = std::chrono::system_clock::now();
        auto next_run_time = strategy->get_next_run_time(current_time);

        scheduled_task task{
            next_id_,
            description,
            std::move(strategy),
            work,
            next_run_time
        };

        tasks_.push_back(std::move(task));
        uint64_t task_id = next_id_;
        ++next_id_;

        log_info("Task added with ID: " + std::to_string(task_id));
        return task_id;
    }

    bool task_scheduler::remove_task(uint64_t task_id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);

        auto it = std::find_if(tasks_.begin(), tasks_.end(),
            [task_id](const scheduled_task& task) { return task.task_id == task_id; });

        if (it != tasks_.end()) {
            tasks_.erase(it);
            log_info("Task removed with ID: " + std::to_string(task_id));
            return true;
        }

        log_error("Task not found with ID: " + std::to_string(task_id));
        return false;
    }

    void task_scheduler::start() {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);

        if (is_running_) {
            log_error("Scheduler is already running");
            return;
        }

        is_running_ = true;
        scheduler_thread_ = std::thread(&task_scheduler::run_loop, this);
        worker_thread_ = std::thread(&task_scheduler::worker_loop, this);
        log_info("Scheduler started");
    }

    void task_scheduler::stop() {
        {
            std::lock_guard<std::mutex> lock(scheduler_mutex_);
            if (!is_running_) {
                return;
            }
            is_running_ = false;
        }

        queue_cv_.notify_one();

        if (scheduler_thread_.joinable()) {
            scheduler_thread_.join();
        }
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        log_info("Scheduler stopped");
    }

    std::vector<task_scheduler::task_info> task_scheduler::list_tasks() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        std::vector<task_info> result;

        for (const auto& task : tasks_) {
            result.push_back({
                task.task_id,
                task.description,
                task.next_run_time,
                get_strategy_type_name(task.strategy.get()),
                get_strategy_parameters_json(task.strategy.get())
                });
        }

        return result;
    }

    void task_scheduler::run_loop() {
        while (is_running_) {
            auto current_time = std::chrono::system_clock::now();

            {
                std::lock_guard<std::mutex> lock(scheduler_mutex_);

                for (auto& task : tasks_) {
                    if (current_time >= task.next_run_time) {
                        task_queue_.push(task.work);
                        task.next_run_time = task.strategy->get_next_run_time(current_time);
                    }
                }
            }

            queue_cv_.notify_one();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void task_scheduler::worker_loop() {
        while (is_running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || !is_running_; });

            while (!task_queue_.empty()) {
                auto work = task_queue_.front();
                task_queue_.pop();
                lock.unlock();

                try {
                    work();
                }
                catch (const std::exception& e) {
                    log_error(std::string("Task execution failed: ") + e.what());
                }

                lock.lock();
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

    std::string task_scheduler::get_strategy_type_name(const period_strategy* strategy) const {
        if (dynamic_cast<const minutely_strategy*>(strategy)) {
            return "minutely";
        }
        if (dynamic_cast<const hourly_strategy*>(strategy)) {
            return "hourly";
        }
        if (dynamic_cast<const daily_strategy*>(strategy)) {
            return "daily";
        }
        if (dynamic_cast<const weekly_strategy*>(strategy)) {
            return "weekly";
        }
        if (dynamic_cast<const monthly_strategy*>(strategy)) {
            return "monthly";
        }
        return "unknown";
    }

    std::string task_scheduler::get_strategy_parameters_json(const period_strategy* strategy) const {
        if (auto* s = dynamic_cast<const minutely_strategy*>(strategy)) {
            return std::string("{\"target_second\":") + std::to_string(s->get_target_second()) + "}";
        }
        if (auto* s = dynamic_cast<const hourly_strategy*>(strategy)) {
            return std::string("{\"target_minute\":") + std::to_string(s->get_target_minute()) +
                ",\"target_second\":" + std::to_string(s->get_target_second()) + "}";
        }
        if (auto* s = dynamic_cast<const daily_strategy*>(strategy)) {
            return std::string("{\"target_hour\":") + std::to_string(s->get_target_hour()) +
                ",\"target_minute\":" + std::to_string(s->get_target_minute()) +
                ",\"target_second\":" + std::to_string(s->get_target_second()) + "}";
        }
        if (auto* s = dynamic_cast<const weekly_strategy*>(strategy)) {
            return std::string("{\"target_weekday\":") + std::to_string(static_cast<int>(s->get_target_weekday())) +
                ",\"target_hour\":" + std::to_string(s->get_target_hour()) +
                ",\"target_minute\":" + std::to_string(s->get_target_minute()) +
                ",\"target_second\":" + std::to_string(s->get_target_second()) + "}";
        }
        if (auto* s = dynamic_cast<const monthly_strategy*>(strategy)) {
            return std::string("{\"target_day\":") + std::to_string(s->get_target_day()) +
                ",\"target_hour\":" + std::to_string(s->get_target_hour()) +
                ",\"target_minute\":" + std::to_string(s->get_target_minute()) + "}";
        }
        return "{}";
    }

}
