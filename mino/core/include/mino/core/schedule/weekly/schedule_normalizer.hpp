#pragma once

#include "mino/core/schedule/weekly/schedule_types.hpp"

namespace mino::core::schedule::weekly {

    // 내부 표현: 요일(0=Mon..6=Sun), 시작분, 종료분 (0..1440)
    struct  interval {
        int day;
        int start; // inclusive minutes
        int end;   // inclusive minutes
    };

    // schedule_normalizer 클래스는 weekly_ranges를 내부적으로 interval로 변환
    class  schedule_normalizer {
    public:
        static weekly_ranges normalize(const weekly_ranges& input);
    };

}
