#include <iostream>
#include <cassert>
#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_util.hpp"

// 기본 유틸리티 함수 검증 (윤년, 날짜 유효성, 요일 계산)
void test_helper_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;
    using weekday = dtutil::weekday;

    std::cout << "[Test 1] Helper & Utility Functions" << std::endl;

    // 윤년 테스트
    assert(dtutil::is_leap(2024) == true);
    assert(dtutil::is_leap(2025) == false);
    assert(dtutil::is_leap(2000) == true);
    assert(dtutil::is_leap(1900) == false);
    std::cout << "  - is_leap: PASSED" << std::endl;

    // 날짜 유효성 테스트
    assert(dtutil::valid_ymd(2024, 2, 29) == true);
    assert(dtutil::valid_ymd(2025, 2, 29) == false);
    assert(dtutil::valid_ymd(2026, 8, 8) == true);
    assert(dtutil::valid_ymd(2026, 13, 1) == false);
    std::cout << "  - valid_ymd: PASSED" << std::endl;

    // 요일 계산 테스트 (2026년 8월 8일 = 토요일)
    weekday w = dtutil::get_weekday(2026, 8, 8, time_zone_mode::utc);
    assert(w == weekday::saturday);
    std::cout << "  - get_weekday: PASSED" << std::endl;
}
