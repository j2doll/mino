#include <iostream>
#include <cassert>
#include <string>
#include <chrono>

#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_convert.hpp"
#include "mino/core/datetime/util/datetime_string.hpp"
#include "mino/core/datetime/util/datetime_util.hpp"

#include "mino/core/string/to_console_encoding.hpp"
 
// 1. 기본 유틸리티 함수 검증 (윤년, 날짜 유효성, 요일 계산)
void test_helper_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;
    using weekday = dtutil::weekday;

    std::cout << "[Test 1] Helper & Utility Functions" << std::endl;

    // 윤년 테스트
    assert(dtutil::is_leap(2024) == true); // 2024년은 윤년
    assert(dtutil::is_leap(2025) == false); // 2025년은 평년
    assert(dtutil::is_leap(2000) == true); // 2000년은 윤년
    assert(dtutil::is_leap(1900) == false); // 1900년은 평년
    std::cout << "  - is_leap: PASSED" << std::endl;

    // 날짜 유효성 테스트
    assert(dtutil::valid_ymd(2024, 2, 29) == true);  // 2024년 2월 29일은 유효함 (윤년)
    assert(dtutil::valid_ymd(2025, 2, 29) == false); // 2025년 2월 29일은 유효하지 않음 (평년)
    assert(dtutil::valid_ymd(2026, 8, 8) == true); // 2026년 8월 8일은 유효함
    assert(dtutil::valid_ymd(2026, 13, 1) == false); // 13월은 유효하지 않음
    std::cout << "  - valid_ymd: PASSED" << std::endl;

    // 요일 계산 테스트 (2026년 8월 8일 = 토요일)
    weekday w = dtutil::get_weekday(2026, 8, 8, time_zone_mode::utc);
    assert(w == weekday::saturday);
    std::cout << "  - get_weekday: PASSED" << std::endl;
}

// 2. 시간 단위 변환 함수 검증 (std::tm <-> time_t <-> time_point)
void test_conversion_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;

    std::cout << "\n[Test 2] Datetime Conversion Functions" << std::endl;

    std::tm tm_in{};
    tm_in.tm_year = 2026 - 1900; // 2026년 (0:1900년)
    tm_in.tm_mon = 7; // 8월 (0:1월, 1:2월, ..., 11:12월)
    tm_in.tm_mday = 8; // 8일
    tm_in.tm_hour = 12; // 12시
    tm_in.tm_min = 30; // 30분
    tm_in.tm_sec = 0; // 0초

    // std::tm  -> time_t
    auto t_opt = dtutil::to_time_t(tm_in, time_zone_mode::utc);
    assert(t_opt.has_value());
    //
    // time_t는 기준시(Epoch)로부터 경과한 초(Second) 수를 나타내는 단일 정수형(타임스탬프) 타입
    //
    // std::tm은 년·월·일·시·분·초 등의 개별 날짜 및 시간 요소를 멤버 변수로 나누어 담는 구조체(Broken-down time)

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
    // 
    // std::tm은 년·월·일·시·분·초 등으로 분해된 날짜와 시간(Broken-down time)을 표현하는 C 스타일 구조체
    // 
    // std::chrono::system_clock::time_point는 특정 기준시(Epoch)로부터 경과한 시간을 표현하는 Modern C++의 타입 안전한 연속적 시간 점(Time point)

    std::cout << "  - to_time_t, to_tm, to_timepoint_opt: PASSED" << std::endl;
}

// 3. 문자열 포맷팅 함수 검증
void test_formatting_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = dtutil::time_zone_mode;

    std::cout << "\n[Test 3] Datetime Formatting Functions" << std::endl;

    std::tm tmv{};
    tmv.tm_year = 2026 - 1900; // 2026년 (0:1900년)
    tmv.tm_mon = 7; // 8월 (0:1월, 1:2월, ..., 11:12월)
    tmv.tm_mday = 8; // 8일
    tmv.tm_hour = 15; // 15시
    tmv.tm_min = 45; // 45분
    tmv.tm_sec = 30; // 30초

    // (1) std::tm 포맷팅 (std::tm 자체는 타임존 정보가 없음.)
    std::string formatted_tm = dtutil::format_datetime(tmv);
    assert(formatted_tm == "2026-08-08 15:45:30.000");

    // (2) time_t 포맷팅 (UTC 기준)
    std::optional<std::time_t> t_utc_opt = dtutil::to_time_t(tmv, time_zone_mode::utc); // UTC 기준 time_t 변환
    assert(t_utc_opt.has_value());

    std::string formatted_utc_t = dtutil::format_datetime(*t_utc_opt, time_zone_mode::utc);
    assert(formatted_utc_t == "2026-08-08 15:45:30.000");

    std::string formatted_local_t = dtutil::format_datetime(*t_utc_opt, time_zone_mode::local_time);
    assert(formatted_local_t == "2026-08-09 00:45:30.000");

    // (3) epoch_ms 포맷팅 (밀리초 포함)
    std::uint64_t epoch_ms = static_cast<std::uint64_t>(*t_utc_opt) * 1000 + 456; // 456밀리초 추가

    std::string formatted_utc_ms = dtutil::format_datetime(epoch_ms, "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::utc);
    assert(formatted_utc_ms == "2026-08-08 15:45:30.456");

    std::string formatted_local_ms = dtutil::format_datetime(epoch_ms, "YYYY-MM-DD hh:mm:ss.SSS", time_zone_mode::local_time);
    assert(formatted_local_ms == "2026-08-09 00:45:30.456");

    std::cout << "  - format_datetime (overloads): PASSED" << std::endl;
}

// 4. 시간 문자열 파싱 함수 검증 (Strict & ISO-8601)
void test_parsing_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;

    std::cout << "\n[Test 4] Datetime Parsing Functions" << std::endl;

    // (1) 엄격한 포맷 파싱
    std::string dt_str = "2026-08-08 12:34:56.789";
    std::string fmt = "YYYY-MM-DD hh:mm:ss.SSS"; 

    auto res_strict = dtutil::parse_strict_datetime(dt_str, fmt, time_zone_mode::utc);
    assert(res_strict.ok == true); // 파싱 성공 
    assert(res_strict.millisecond == 789);
    assert(res_strict.present.has_year == true); // has_year는 년도값(2026)이 입력 문자열에 포함되어 있으므로 true
    assert(res_strict.present.has_millisecond == true); // has_millisecond는 밀리초값(789)이 입력 문자열에 포함되어 있으므로 true
    std::cout << "  - parse_strict_datetime: PASSED" << std::endl;

    // (2) ISO-8601 파싱
    std::string iso_str = "2026-08-08T12:34:56.789Z"; // T는 날짜와 시간 구분. Z는 UTC를 의미. (한국 시간대(KST)는 +09:00를 Z 대신 사용)
    auto res_iso = dtutil::parse_iso8601_datetime(iso_str, time_zone_mode::utc);
    assert(res_iso.ok == true); // 파싱 성공
    assert(res_iso.epoch == res_strict.epoch); // ISO-8601 파싱 결과와 엄격한 포맷 파싱 결과의 epoch 값이 동일해야 함
    assert(res_iso.millisecond == 789);
    std::cout << "  - parse_iso8601_datetime: PASSED" << std::endl;

    // (3) 자동 판별 파싱 (ISO8601 토큰 사용)
    auto res_auto = dtutil::parse_datetime_auto(iso_str, "ISO8601", time_zone_mode::utc);
    assert(res_auto.ok == true); // 자동 판별 파싱 성공
    assert(res_auto.epoch == res_strict.epoch); // 자동 판별 파싱 결과와 엄격한 포맷 파싱 결과의 epoch 값이 동일해야 함
    std::cout << "  - parse_datetime_auto: PASSED" << std::endl;

    // (4) 실패 케이스 검증
    auto res_fail = dtutil::parse_strict_datetime("2026-02-29 10:00:00", "YYYY-MM-DD hh:mm:ss", time_zone_mode::utc);
    assert(res_fail.ok == false); // 파싱 실패. 2026년은 평년이므로 2월 29일은 유효하지 않음.
    assert(!res_fail.error.empty()); // 실패 사유(res_fail.error): "Date Validation Error"
    std::cout << "  - Invalid date parse error handling: PASSED" << std::endl;
}

// 5. 현재 시간 문자열 생성 유틸리티 검증
void test_current_time_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;

    std::cout << "\n[Test 5] Current Time String Generator" << std::endl;

    std::string current_local = dtutil::current_time_string(time_zone_mode::local_time, "YYYY-MM-DD hh:mm:ss.SSS");
    std::string current_utc = dtutil::current_time_string(time_zone_mode::utc, "YYYY-MM-DD hh:mm:ss.SSS");

    std::cout << "  - Local Time : " << current_local << std::endl;
    std::cout << "  - UTC Time   : " << current_utc << std::endl;
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "   mino::core::datetime Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;

    try {
        test_helper_functions();
        test_conversion_functions();
        test_formatting_functions();
        test_parsing_functions();
        test_current_time_functions();

        std::cout << "\n=========================================" << std::endl;
        std::cout << "   ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
        std::cout << "=========================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
