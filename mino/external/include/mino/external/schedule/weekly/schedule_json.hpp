#pragma once

#include <string>
#include <cctype>
#include <optional>

#include "mino/core/schedule/weekly/schedule_types.hpp"

namespace mino::external::schedule::weekly {

    using weekly_ranges = mino::core::schedule::weekly::weekly_ranges;

    // weekly_ranges -> JSON 문자열
    // (구현부에서 nlohmann::json을 사용하여 직렬화)
    std::string to_json_string(const weekly_ranges& ranges);

    // JSON 문자열 -> weekly_ranges
    // (구현부에서 nlohmann::json을 사용하여 파싱)
    // 예: from_json_string(R"([ { "start_day":"Mon", "start_h":8, ... } ])")
    std::optional<weekly_ranges> from_json_string(const std::string& json_text);

} // namespace mino::external::schedule::weekly 
