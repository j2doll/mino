#include <iostream>
#include <string>
#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_string.hpp"

// 현재 시간 문자열 생성 유틸리티 검증
void test_current_time_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;

    std::cout << "\n[Test 5] Current Time String Generator" << std::endl;

    std::string current_local = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
    std::string current_utc = dtutil::current_time_string(time_zone_mode::utc, "YYYY-MM-DD hh:mm:ss.SSS");

    std::cout << "  - Local Time : " << current_local << std::endl;
    std::cout << "  - UTC Time   : " << current_utc << std::endl;
}
