#pragma once

#include <cstdint>
#include <vector>

namespace mino::external::schedule::weekly {

    // 요일을 나타내는 열거형 (0: 월요일, 1: 화요일, ..., 6: 일요일)
    enum class weekday : uint8_t {
        mon = 0, tue, wed, thu, fri, sat, sun
    };

    // 시간을 나타내는 구조체 (24시간 형식)
    struct  time_hm {
        int hour; // 시간. 0 ~ 23
        int minute; // 분. 0 ~ 59
    };

    // 주간 일정 범위 (시작 요일/시간 ~ 종료 요일/시간)
    struct  weekly_range {
        weekday start_day; // 시작 요일
        time_hm start_time; // 시작 시간
        weekday end_day; // 종료 요일
        time_hm end_time; // 종료 시간
    };

    // 여러 개의 주간 일정 범위
    using weekly_ranges = std::vector<weekly_range>;

}
