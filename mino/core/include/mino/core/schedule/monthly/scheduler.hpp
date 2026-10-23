#pragma once

#include <vector>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <filesystem>

#include "mino/core/schedule/weekly/scheduler_time_base.hpp"
#include "mino/core/schedule/monthly/schedule_types.hpp"
#include "mino/core/schedule/monthly/monthly_rule.hpp"
#include "mino/core/schedule/monthly/holiday_calendar.hpp"

namespace mino::core::schedule::monthly {

    using weekly::time_base;
    using weekly::scheduler_time_base;

    class scheduler {
    public:
        explicit scheduler(time_base base = time_base::localtime);

        void set_now_provider(std::function<std::chrono::system_clock::time_point()> now_provider);
        void set_now(const date_ymd& d, const time_hms& t);
        void set_now(std::chrono::system_clock::time_point tp);

        holiday_calendar& calendar() noexcept { return calendar_; }
        const holiday_calendar& calendar() const noexcept { return calendar_; }

        void add_rule(const monthly_rule& rule);
        void clear_rules() noexcept { rules_.clear(); }
        const std::vector<monthly_rule>& rules() const noexcept { return rules_; }

        bool is_active_now(bool match_seconds = false) const;

        std::optional<std::pair<date_ymd, time_hms>> get_execution_date_time(
            const monthly_rule& rule, int year, int month) const;

        // JSON 직렬화/역직렬화
        bool load_from_json_string(const std::string& json_text);
        bool load_from_json_file(const std::string& file_path);
        std::string export_to_json_string(int indent = -1) const;

        // XML 직렬화/역직렬화
        bool load_from_xml_string(const std::string& xml_text);
        bool load_from_xml_file(const std::filesystem::path& file_path);
        std::string export_to_xml_string(bool pretty_print = true) const;
        bool export_to_xml_file(const std::filesystem::path& file_path, bool pretty_print = true) const;

    private:
        date_ymd calculate_pattern_date(const monthly_rule& rule, int year, int month) const;
        std::tm get_current_tm() const;

        scheduler_time_base time_base_;
        holiday_calendar calendar_;
        std::vector<monthly_rule> rules_;
        std::function<std::chrono::system_clock::time_point()> now_provider_;
    };

} // namespace mino::core::schedule::monthly
