#pragma once

#include <string>
#include <optional>

#include "mino/core/schedule/weekly/weekly.hpp"

namespace mino::core::schedule::weekly {

    std::optional<weekday> day_from_string(const std::string& s);

    std::string to_json_string(const weekly_ranges& ranges, int indent = -1);

    std::optional<weekly_ranges> from_json_string(const std::string& json_text);

}  
