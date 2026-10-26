#include "mino/core/schedule/task/task_scheduler.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"
#include <chrono>
#include <iomanip>
#include <algorithm>

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
            std::string(mino::core::enums::enum_name(basis)));
    }

    time_base task_scheduler::get_time_basis() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        return time_basis_;
    }

    // --- 공휴일 관리 구현 ---

    bool task_scheduler::load_holiday_file(const std::string& file_path) {
        auto cal_opt = holiday_calendar::from_json_file(file_path);
        if (!cal_opt) {
            log_error("Failed to load holiday JSON from file: " + file_path);
            return false;
        }
        set_holiday_calendar(std::move(*cal_opt));
        return true;
    }

    bool task_scheduler::load_holiday_json(std::string_view json_str) {
        auto cal_opt = holiday_calendar::from_json_string(json_str);
        if (!cal_opt) {
            log_error("Failed to parse holiday JSON string");
            return false;
        }
        set_holiday_calendar(std::move(*cal_opt));
        return true;
    }

    void task_scheduler::set_holiday_calendar(holiday_calendar cal) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        holiday_calendar_ = std::move(cal);
        refresh_all_tasks_next_run_time_locked();
        log_info("Holiday calendar applied to scheduler");
    }

    void task_scheduler::clear_holiday_calendar() {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        holiday_calendar_.reset();
        refresh_all_tasks_next_run_time_locked();
        log_info("Holiday calendar cleared");
    }

    bool task_scheduler::has_holiday_calendar() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        return holiday_calendar_.has_value();
    }

    bool task_scheduler::export_holiday_file(const std::string& file_path, int indent) const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) return false;
        return holiday_calendar_->to_json_file(file_path, indent);
    }

    std::string task_scheduler::export_holiday_json(int indent) const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) return "{}";
        return holiday_calendar_->to_json_string(indent);
    }

    void task_scheduler::add_recurring_holiday(int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->add_recurring_holiday(month, day);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::remove_recurring_holiday(int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (holiday_calendar_) {
            holiday_calendar_->remove_recurring_holiday(month, day);
            refresh_all_tasks_next_run_time_locked();
        }
    }

    void task_scheduler::add_specific_holiday(int year, int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->add_specific_holiday(year, month, day);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::remove_specific_holiday(int year, int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (holiday_calendar_) {
            holiday_calendar_->remove_specific_holiday(year, month, day);
            refresh_all_tasks_next_run_time_locked();
        }
    }

    void task_scheduler::add_recurring_exclusion(int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->add_recurring_exclusion(month, day);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::remove_recurring_exclusion(int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (holiday_calendar_) {
            holiday_calendar_->remove_recurring_exclusion(month, day);
            refresh_all_tasks_next_run_time_locked();
        }
    }

    void task_scheduler::add_specific_exclusion(int year, int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->add_specific_exclusion(year, month, day);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::remove_specific_exclusion(int year, int month, int day) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (holiday_calendar_) {
            holiday_calendar_->remove_specific_exclusion(year, month, day);
            refresh_all_tasks_next_run_time_locked();
        }
    }

    void task_scheduler::set_weekend_holiday(weekday wd, bool is_holiday) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->set_weekend_holiday(wd, is_holiday);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::set_holiday_policy(holiday_policy policy) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        if (!holiday_calendar_) holiday_calendar_.emplace(time_basis_);
        holiday_calendar_->set_policy(policy);
        refresh_all_tasks_next_run_time_locked();
    }

    void task_scheduler::refresh_all_tasks_next_run_time_locked() {
        auto now = std::chrono::system_clock::now();
        for (auto& task : tasks_) {
            task.next_run_time = compute_next_run_time_with_holidays(task.strategy.get(), now);
        }
    }

    std::chrono::system_clock::time_point task_scheduler::compute_next_run_time_with_holidays(
        const period_strategy* strategy,
        std::chrono::system_clock::time_point current_time) const {

        if (!strategy) return current_time;

        if (!holiday_calendar_) {
            return const_cast<period_strategy*>(strategy)->get_next_run_time(current_time);
        }

        const auto& cal = *holiday_calendar_;
        bool is_date_based = (dynamic_cast<const monthly_strategy*>(strategy) != nullptr ||
            dynamic_cast<const weekly_strategy*>(strategy) != nullptr);

        if (!is_date_based) {
            // (15-1) 시:분:초 기반 전략 (daily, hourly, minutely) -> 공휴일 당일 실행 없이 Skip
            auto candidate = const_cast<period_strategy*>(strategy)->get_next_run_time(current_time);
            int loop_guard = 0;
            while (cal.is_holiday(candidate) && loop_guard++ < 366 * 24) {
                candidate = const_cast<period_strategy*>(strategy)->get_next_run_time(candidate);
            }
            return candidate;
        }

        // (15-2) 날짜 기반 전략 (monthly, weekly) -> 6번 옵션 정책 적용
        auto base_current = current_time;
        int max_cycles = 120; // 10년 주기 방어

        while (max_cycles-- > 0) {
            auto target = const_cast<period_strategy*>(strategy)->get_next_run_time(base_current);

            if (!cal.is_holiday(target)) {
                return target;
            }

            switch (cal.get_policy()) {
            case holiday_policy::skip: {
                base_current = target;
                break;
            }
            case holiday_policy::shift_to_previous: {
                auto shifted = target;
                int shift_guard = 0;
                while (cal.is_holiday(shifted) && shift_guard++ < 365) {
                    shifted -= std::chrono::hours(24);
                }
                if (shifted > current_time) {
                    return shifted;
                }
                base_current = target;
                break;
            }
            case holiday_policy::shift_to_next: {
                auto shifted = target;
                int shift_guard = 0;
                while (cal.is_holiday(shifted) && shift_guard++ < 365) {
                    shifted += std::chrono::hours(24);
                }
                return shifted;
            }
            }
        }

        return const_cast<period_strategy*>(strategy)->get_next_run_time(current_time);
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
        auto next_run_time = compute_next_run_time_with_holidays(strategy.get(), current_time);

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
                        task.next_run_time = compute_next_run_time_with_holidays(task.strategy.get(), current_time);
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
        if (dynamic_cast<const minutely_strategy*>(strategy)) return "minutely";
        if (dynamic_cast<const hourly_strategy*>(strategy)) return "hourly";
        if (dynamic_cast<const daily_strategy*>(strategy)) return "daily";
        if (dynamic_cast<const weekly_strategy*>(strategy)) return "weekly";
        if (dynamic_cast<const monthly_strategy*>(strategy)) return "monthly";
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

} // namespace mino::core::schedule::task
