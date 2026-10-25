#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "mino/core/schedule/task/task_scheduler.hpp"
#include "mino/core/schedule/task/period_strategy.hpp"
#include "mino/core/json/value.hpp"

namespace mino::core::schedule::task {

    struct task_config {
        uint64_t task_id;
        std::optional<std::string> description;
        std::string strategy_type;
        mino::core::json::value strategy_params;
        std::string strategy_params_json;
        time_base time_basis;
    };

    class task_scheduler_json {
    public:
        // JSON 파일로 내보내기
        static bool to_json_file(
            const task_scheduler& scheduler,
            const std::string& file_path,
            int indent = 2
        );

        // JSON 문자열로 직렬화
        static std::string to_json_string(
            const task_scheduler& scheduler,
            int indent = 2
        );

        // JSON 파일에서 읽기
        static std::vector<task_config> from_json_file(const std::string& file_path);

        // JSON 문자열에서 읽기
        static std::vector<task_config> from_json_string(const std::string& json_str);

        // task_config로부터 strategy 복원
        static std::unique_ptr<period_strategy> strategy_from_config(
            const task_config& config
        );

        // task_scheduler에 복원하기
        static bool restore_tasks_to_scheduler(
            task_scheduler& scheduler,
            const std::string& file_path,
            std::function<std::function<void()>(uint64_t, const std::string&)> work_factory
        );

        // ID별로 task_config 복원
        static std::unique_ptr<task_config> restore_task_by_id(
            const std::string& file_path,
            uint64_t task_id
        );
    };

}
