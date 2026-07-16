#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <optional>
#include <chrono>
#include <cctype>
#include <limits>

namespace mino::core::datetime {
    namespace v0 {

        enum class time_zone_mode { // 타임존 모드
            utc, // UTC 기준
            local_time // 로컬 타임 기준 (KST 등)
        }; 

        // 요일 열거형: std::tm::tm_wday(0=Sunday..6=Saturday)과 일치하도록 정의
        enum class weekday : int {
            sunday = 0,
            monday = 1,
            tuesday = 2,
            wednesday = 3,
            thursday = 4,
            friday = 5,
            saturday = 6,

            no_statement = -1 // 요일 정보 없음
        };

    } // namespace v0
} // namespace mino::core::datetime

