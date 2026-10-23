#pragma once

#include <string>
#include <vector>
#include <optional>
#include <utility>

#include "mino/core/schedule/monthly/monthly_rule.hpp"
#include "mino/core/schedule/monthly/holiday_calendar.hpp"

namespace mino::core::schedule::monthly {

    std::string to_json_string(const holiday_calendar& cal, const std::vector<monthly_rule>& rules, int indent = -1);
    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_json_string(const std::string& json_text);
    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_json_file(const std::string& file_path);

} // namespace mino::core::schedule::monthly
