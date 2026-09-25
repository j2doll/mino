#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>
#include <optional>

#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_convert.hpp"
#include "mino/core/datetime/util/datetime_string.hpp"
#include "mino/core/datetime/util/datetime_util.hpp" 

// 문자열 포맷팅 함수 검증
void test_formatting_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;

    std::cout << "\n[Test 3] Datetime Formatting Functions" << std::endl;

    std::tm tmv{};
    tmv.tm_year = 2026 - 1900;
    tmv.tm_mon = 7;
    tmv.tm_mday = 8;
    tmv.tm_hour = 15;
    tmv.tm_min = 45;
    tmv.tm_sec = 30;

    // (1) std::tm 포맷팅
    std::string formatted_tm = dtutil::format_datetime(tmv);
    assert(formatted_tm == "2026-08-08 15:45:30.000");

    // (2) time_t 포맷팅 (UTC 기준)
    std::optional<std::time_t> t_utc_opt = dtutil::to_time_t(tmv, time_zone_mode::utc);
    assert(t_utc_opt.has_value());

    std::string formatted_utc_t = dtutil::format_datetime(*t_utc_opt, time_zone_mode::utc);
    assert(formatted_utc_t == "2026-08-08 15:45:30.000");

    std::string formatted_local_t = dtutil::format_datetime(*t_utc_opt, time_zone_mode::local_time);
    assert(formatted_local_t == "2026-08-09 00:45:30.000");

    // (3) epoch_ms 포맷팅 (밀리초 포함)
    std::uint64_t epoch_ms = static_cast<std::uint64_t>(*t_utc_opt) * 1000 + 456;

    std::string formatted_utc_ms = dtutil::format_datetime(epoch_ms, "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::utc);
    assert(formatted_utc_ms == "2026-08-08 15:45:30.456");

    std::string formatted_local_ms = dtutil::format_datetime(epoch_ms, "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::local_time);
    assert(formatted_local_ms == "2026-08-09 00:45:30.456");

    std::cout << "  - format_datetime (overloads): PASSED" << std::endl;
}
