#include "mino/core/schedule/task/task_scheduler_json.hpp"
#include "mino/core/json/parser.hpp"
#include "mino/core/json/serializer.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace mino::core::schedule::task {

    namespace {
        std::string time_point_to_iso8601(const std::chrono::system_clock::time_point& tp) {
            auto time_t = std::chrono::system_clock::to_time_t(tp);
            auto tm = *std::gmtime(&time_t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            return oss.str();
        }

        std::chrono::system_clock::time_point iso8601_to_time_point(const std::string& iso_str) {
            struct tm tm = {};
            std::istringstream iss(iso_str);
            iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            auto time_t = std::mktime(&tm);
            return std::chrono::system_clock::from_time_t(time_t);
        }

        std::string time_base_to_string(time_base base) {
            return (base == time_base::utc) ? "utc" : "localtime";
        }

        time_base string_to_time_base(const std::string& str) {
            return (str == "utc") ? time_base::utc : time_base::localtime;
        }

        // UTF-8 문자열을 JSON 안전 형식으로 인코딩
        std::string escape_json_string(const std::string& str) {
            std::string result;
            result.push_back('"');

            for (unsigned char c : str) {
                if (c == '"') {
                    result += "\\\"";
                }
                else if (c == '\\') {
                    result += "\\\\";
                }
                else if (c == '\b') {
                    result += "\\b";
                }
                else if (c == '\f') {
                    result += "\\f";
                }
                else if (c == '\n') {
                    result += "\\n";
                }
                else if (c == '\r') {
                    result += "\\r";
                }
                else if (c == '\t') {
                    result += "\\t";
                }
                else if (c < 0x20) {
                    result.push_back(c);
                }
                else {
                    result.push_back(c);
                }
            }

            result.push_back('"');
            return result;
        }
    }

    bool task_scheduler_json::to_json_file(
        const task_scheduler& scheduler,
        const std::string& file_path,
        int indent) {

        std::string json_str = to_json_string(scheduler, indent);

        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(json_str.data(), json_str.length());
        file.close();

        return true;
    }

    std::string task_scheduler_json::to_json_string(
        const task_scheduler& scheduler,
        int indent) {
        auto tasks = scheduler.list_tasks();
        auto time_basis = scheduler.get_time_basis();

        // 수동으로 JSON 생성
        std::ostringstream json_stream;

        json_stream << "{\n";
        json_stream << "  \"time_basis\": \"" << time_base_to_string(time_basis) << "\",\n";
        json_stream << "  \"tasks\": [\n";

        for (size_t i = 0; i < tasks.size(); ++i) {
            const auto& task = tasks[i];

            json_stream << "    {\n";
            json_stream << "      \"task_id\": " << task.task_id << ",\n";

            // description 필드
            std::string desc = task.description ? *task.description : "";
            json_stream << "      \"description\": " << escape_json_string(desc) << ",\n";

            json_stream << "      \"strategy_type\": \"" << task.strategy_type << "\",\n";

            // strategy_params_json은 이미 JSON 형식이므로 그대로 삽입
            std::string params = task.strategy_params_json;
            if (params.empty()) {
                params = "{}";
            }
            json_stream << "      \"strategy_params_json\": " << params << "\n";

            json_stream << "    }";
            if (i < tasks.size() - 1) {
                json_stream << ",";
            }
            json_stream << "\n";
        }

        json_stream << "  ]\n";
        json_stream << "}\n";

        return json_stream.str();
    }

    std::vector<task_config> task_scheduler_json::from_json_file(const std::string& file_path) {
        std::string json_str;
        std::ifstream file(file_path, std::ios::binary);

        if (!file.is_open()) {
            return {};
        }

        // 파일 크기 얻기
        file.seekg(0, std::ios::end);
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (file_size == 0) {
            file.close();
            return {};
        }

        json_str.resize(static_cast<size_t>(file_size));
        file.read(&json_str[0], file_size);
        file.close();

        return from_json_string(json_str);
    }

    std::vector<task_config> task_scheduler_json::from_json_string(const std::string& json_str) {
        std::vector<task_config> result;

        if (json_str.empty()) {
            return result;
        }

        auto parsed = mino::core::json::parser::parse(json_str);
        if (!parsed.is_object()) {
            return result;
        }

        const auto& root_obj = parsed.get_object();

        // time_basis 추출
        time_base root_time_basis = time_base::localtime;
        auto tb_it = root_obj.find("time_basis");
        if (tb_it != root_obj.end() && tb_it->second.is_string()) {
            root_time_basis = string_to_time_base(tb_it->second.get_string());
        }

        // tasks 배열 추출
        auto tasks_it = root_obj.find("tasks");
        if (tasks_it == root_obj.end() || !tasks_it->second.is_array()) {
            return result;
        }

        const auto& tasks_array = tasks_it->second.get_array();

        for (size_t i = 0; i < tasks_array.size(); ++i) {
            const auto& task_val = tasks_array[i];
            if (!task_val.is_object()) {
                continue;
            }

            const auto& task_obj = task_val.get_object();

            task_config config;
            config.time_basis = root_time_basis;

            // task_id
            auto id_it = task_obj.find("task_id");
            if (id_it != task_obj.end()) {
                config.task_id = static_cast<uint64_t>(id_it->second.get_number());
            }

            // description
            auto desc_it = task_obj.find("description");
            if (desc_it != task_obj.end()) {
                const auto& desc_str = desc_it->second.get_string();
                config.description = desc_str.empty() ? std::nullopt : std::make_optional(desc_str);
            }

            // strategy_type
            auto st_it = task_obj.find("strategy_type");
            if (st_it != task_obj.end()) {
                config.strategy_type = st_it->second.get_string();
            }

            // strategy_params_json
            auto sp_it = task_obj.find("strategy_params_json");
            if (sp_it != task_obj.end()) {
                config.strategy_params_json = mino::core::json::serializer::serialize(sp_it->second, -1);
                config.strategy_params = sp_it->second;
            }
            else {
                config.strategy_params_json = "{}";
                config.strategy_params = mino::core::json::value();
            }

            result.push_back(config);
        }

        return result;
    }

    std::unique_ptr<period_strategy> task_scheduler_json::strategy_from_config(
        const task_config& config) {

        const auto& params = config.strategy_params;
        if (!params.is_object()) {
            return nullptr;
        }

        const auto& params_obj = params.get_object();

        if (config.strategy_type == "minutely") {
            auto it = params_obj.find("target_second");
            if (it != params_obj.end()) {
                int target_second = static_cast<int>(it->second.get_number());
                return std::make_unique<minutely_strategy>(target_second);
            }
        }
        else if (config.strategy_type == "hourly") {
            auto min_it = params_obj.find("target_minute");
            auto sec_it = params_obj.find("target_second");
            if (min_it != params_obj.end() && sec_it != params_obj.end()) {
                int target_minute = static_cast<int>(min_it->second.get_number());
                int target_second = static_cast<int>(sec_it->second.get_number());
                return std::make_unique<hourly_strategy>(target_minute, target_second);
            }
        }
        else if (config.strategy_type == "daily") {
            auto h_it = params_obj.find("target_hour");
            auto m_it = params_obj.find("target_minute");
            auto s_it = params_obj.find("target_second");
            if (h_it != params_obj.end() && m_it != params_obj.end() && s_it != params_obj.end()) {
                int target_hour = static_cast<int>(h_it->second.get_number());
                int target_minute = static_cast<int>(m_it->second.get_number());
                int target_second = static_cast<int>(s_it->second.get_number());
                return std::make_unique<daily_strategy>(target_hour, target_minute, target_second);
            }
        }
        else if (config.strategy_type == "weekly") {
            auto wd_it = params_obj.find("target_weekday");
            auto h_it = params_obj.find("target_hour");
            auto m_it = params_obj.find("target_minute");
            auto s_it = params_obj.find("target_second");
            if (wd_it != params_obj.end() && h_it != params_obj.end() &&
                m_it != params_obj.end() && s_it != params_obj.end()) {
                int target_weekday = static_cast<int>(wd_it->second.get_number());
                int target_hour = static_cast<int>(h_it->second.get_number());
                int target_minute = static_cast<int>(m_it->second.get_number());
                int target_second = static_cast<int>(s_it->second.get_number());
                return std::make_unique<weekly_strategy>(
                    static_cast<weekday>(target_weekday),
                    target_hour, target_minute, target_second);
            }
        }
        else if (config.strategy_type == "monthly") {
            auto d_it = params_obj.find("target_day");
            auto h_it = params_obj.find("target_hour");
            auto m_it = params_obj.find("target_minute");
            if (d_it != params_obj.end() && h_it != params_obj.end() && m_it != params_obj.end()) {
                int target_day = static_cast<int>(d_it->second.get_number());
                int target_hour = static_cast<int>(h_it->second.get_number());
                int target_minute = static_cast<int>(m_it->second.get_number());
                return std::make_unique<monthly_strategy>(target_day, target_hour, target_minute);
            }
        }

        return nullptr;
    }

    bool task_scheduler_json::restore_tasks_to_scheduler(
        task_scheduler& scheduler,
        const std::string& file_path,
        std::function<std::function<void()>(uint64_t, const std::string&)> work_factory) {

        auto configs = from_json_file(file_path);
        if (configs.empty()) {
            return false;
        }

        if (!configs.empty()) {
            scheduler.set_time_basis(configs[0].time_basis);
        }

        int added_count = 0;
        for (const auto& config : configs) {
            auto strategy = strategy_from_config(config);
            if (!strategy) {
                continue;
            }

            auto work = work_factory(config.task_id, config.strategy_type);
            uint64_t new_task_id = scheduler.add_task(std::move(strategy), work, config.description);

            if (new_task_id > 0) {
                ++added_count;
            }
        }

        return added_count > 0;
    }

    std::unique_ptr<task_config> task_scheduler_json::restore_task_by_id(
        const std::string& file_path,
        uint64_t task_id) {

        auto configs = from_json_file(file_path);

        for (const auto& config : configs) {
            if (config.task_id == task_id) {
                return std::make_unique<task_config>(config);
            }
        }

        return nullptr;
    }

}
