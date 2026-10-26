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
#include "mino/core/schedule/task/holiday_calendar.hpp"

namespace mino::core::schedule::task {

    struct scheduled_task {
        uint64_t task_id;
        std::optional<std::string> description;
        std::unique_ptr<period_strategy> strategy;
        std::function<void()> work;
        std::chrono::system_clock::time_point next_run_time;
    };

    class task_scheduler {
    public:
        task_scheduler();
        explicit task_scheduler(time_base time_basis);
        ~task_scheduler();

        void set_logger(std::shared_ptr< mino::core::log::tinylog::logger > logger);

        // 시간 기준 설정/조회
        void set_time_basis(time_base basis);
        time_base get_time_basis() const;

        // --- 공휴일 관리 인터페이스 ---
        bool load_holiday_file(const std::string& file_path);
        bool load_holiday_json(std::string_view json_str);
        void set_holiday_calendar(holiday_calendar cal);
        void clear_holiday_calendar();
        bool has_holiday_calendar() const;

        bool export_holiday_file(const std::string& file_path, int indent = 2) const;
        std::string export_holiday_json(int indent = 2) const;

        // 멀티스레드 안전한 동적 공휴일 조작 API
        void add_recurring_holiday(int month, int day);
        void remove_recurring_holiday(int month, int day);
        void add_specific_holiday(int year, int month, int day);
        void remove_specific_holiday(int year, int month, int day);
        void add_recurring_exclusion(int month, int day);
        void remove_recurring_exclusion(int month, int day);
        void add_specific_exclusion(int year, int month, int day);
        void remove_specific_exclusion(int year, int month, int day);
        void set_weekend_holiday(weekday wd, bool is_holiday);
        void set_holiday_policy(holiday_policy policy);

        // 기존 task 관리 인터페이스 (원형 유지)
        uint64_t add_task(std::unique_ptr<period_strategy> strategy,
            std::function<void()> work,
            std::optional<std::string> description = std::nullopt);

        bool remove_task(uint64_t task_id);
        void start();
        void stop();

        struct task_info {
            uint64_t task_id;
            std::optional<std::string> description;
            std::chrono::system_clock::time_point next_run_time;
            std::string strategy_type;
            std::string strategy_params_json;
        };

        std::vector<task_info> list_tasks() const;

    private:
        void run_loop();
        void worker_loop();
        void log_info(const std::string& msg);
        void log_error(const std::string& msg);

        // 공휴일 규칙이 적용된 다음 실행 시각 계산
        std::chrono::system_clock::time_point compute_next_run_time_with_holidays(
            const period_strategy* strategy,
            std::chrono::system_clock::time_point current_time) const;

        void refresh_all_tasks_next_run_time_locked();

        // Helper methods for strategy serialization
        std::string get_strategy_type_name(const period_strategy* strategy) const;
        std::string get_strategy_parameters_json(const period_strategy* strategy) const;

        time_base time_basis_;
        std::optional<holiday_calendar> holiday_calendar_;
        std::vector<scheduled_task> tasks_;
        uint64_t next_id_;
        std::thread scheduler_thread_;
        bool is_running_;
        mutable std::mutex scheduler_mutex_;

        std::thread worker_thread_;
        std::queue<std::function<void()>> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;

        std::shared_ptr< mino::core::log::tinylog::logger > logger_;
    };

} // namespace mino::core::schedule::task
