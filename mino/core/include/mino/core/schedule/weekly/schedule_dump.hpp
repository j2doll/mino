#pragma once

#include <string>

#include "mino/core/schedule/weekly/schedule_types.hpp"

namespace mino::core::schedule::weekly {

    // 주간 스케줄 범위 목록을 사람이 읽을 수 있는 문자열로 변환
     std::string dump_schedule(const weekly_ranges& ranges);

}
