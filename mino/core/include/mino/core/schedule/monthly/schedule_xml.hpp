#pragma once

#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <filesystem>

#include "mino/core/schedule/monthly/monthly_rule.hpp"
#include "mino/core/schedule/monthly/holiday_calendar.hpp"

namespace mino::core::schedule::monthly {

    std::string to_xml_string(const holiday_calendar& cal, const std::vector<monthly_rule>& rules, bool pretty_print = true);
    bool to_xml_file(const std::filesystem::path& file_path, const holiday_calendar& cal, const std::vector<monthly_rule>& rules, bool pretty_print = true);

    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_xml_string(const std::string& xml_text);
    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_xml_file(const std::filesystem::path& file_path);

} // namespace mino::core::schedule::monthly
