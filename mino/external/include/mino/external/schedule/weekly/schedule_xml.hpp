#pragma once

#include <string>

#include "mino/external/schedule/weekly/schedule_types.hpp"

namespace mino::external::schedule::weekly {

    // XML 문자열(std::string)을 반환하도록 변경
     std::string to_xml(const weekly_ranges& ranges);

}

