#include <iostream>
#include <cassert>
#include <ctime>
#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_convert.hpp"

// 시간 단위 변환 함수 검증 (std::tm <-> time_t <-> time_point)
void test_conversion_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;

    std::cout << "\n[Test 2] Datetime Conversion Functions" << std::endl;

    std::tm tm_in{};
    tm_in.tm_year = 2026 - 1900;
    tm_in.tm_mon = 7;
    tm_in.tm_mday = 8;
    tm_in.tm_hour = 12;
    tm_in.tm_min = 30;
    tm_in.tm_sec = 0;

    // std::tm -> time_t
    auto t_opt = dtutil::to_time_t(tm_in, time_zone_mode::utc);
    assert(t_opt.has_value());

    // time_t -> std::tm
    auto tm_out_opt = dtutil::to_tm(*t_opt, time_zone_mode::utc);
    assert(tm_out_opt.has_value());
    assert(tm_out_opt->tm_year == tm_in.tm_year);
    assert(tm_out_opt->tm_mon == tm_in.tm_mon);
    assert(tm_out_opt->tm_mday == tm_in.tm_mday);
    assert(tm_out_opt->tm_hour == tm_in.tm_hour);

    // std::tm -> system_clock::time_point
    auto tp_opt = dtutil::to_timepoint_opt(tm_in, time_zone_mode::utc);
    assert(tp_opt.has_value());

    std::cout << "  - to_time_t, to_tm, to_timepoint_opt: PASSED" << std::endl;
}
