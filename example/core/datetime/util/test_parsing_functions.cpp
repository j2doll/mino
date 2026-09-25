#include <iostream>
#include <cassert>
#include <string>
#include "mino/core/datetime/util/datetime_common.hpp"
#include "mino/core/datetime/util/datetime_string.hpp"

// 시간 문자열 파싱 함수 검증 (Strict, ISO-8601 & RFC-3339)
void test_parsing_functions() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;

    std::cout << "\n[Test 4] Datetime Parsing Functions" << std::endl;

    // (1) 엄격한 포맷 파싱
    std::string dt_str = "2026-08-08 12:34:56.789";
    std::string fmt = "YYYY-MM-DD hh:mm:ss.SSS";

    auto res_strict = dtutil::parse_strict_datetime(dt_str, fmt, time_zone_mode::utc);
    assert(res_strict.ok == true);
    assert(res_strict.millisecond == 789);
    assert(res_strict.present.has_year == true);
    assert(res_strict.present.has_millisecond == true);
    std::cout << "  - parse_strict_datetime: PASSED" << std::endl;

    // (2) ISO-8601 파싱
    std::string iso_str = "2026-08-08T12:34:56.789Z";
    auto res_iso = dtutil::parse_iso8601_datetime(iso_str, time_zone_mode::utc);
    assert(res_iso.ok == true);
    assert(res_iso.epoch == res_strict.epoch);
    assert(res_iso.millisecond == 789);
    std::cout << "  - parse_iso8601_datetime: PASSED" << std::endl;

    // (3) 자동 판별 파싱 (ISO8601 토큰 사용)
    auto res_auto = dtutil::parse_datetime_auto(iso_str, "ISO8601", time_zone_mode::utc);
    assert(res_auto.ok == true);
    assert(res_auto.epoch == res_strict.epoch);
    std::cout << "  - parse_datetime_auto (ISO8601): PASSED" << std::endl;

    // (4) RFC-3339 전용 함수 파싱
    std::string rfc_no_ms = "2026-08-08T12:34:56Z";
    auto res_rfc_no_ms = dtutil::parse_rfc3339_datetime(rfc_no_ms, time_zone_mode::utc);
    assert(res_rfc_no_ms.ok == true);
    assert(res_rfc_no_ms.epoch == res_strict.epoch);
    assert(res_rfc_no_ms.millisecond == 0);
    std::cout << "  - parse_rfc3339_datetime (without ms): PASSED" << std::endl;

    // (5) RFC-3339 양수 오프셋 (+09:00) & 공백 구분자 허용 검증
    std::string rfc_kst = "2026-08-08 21:34:56.789+09:00";
    auto res_rfc_kst = dtutil::parse_rfc3339_datetime(rfc_kst, time_zone_mode::utc);
    assert(res_rfc_kst.ok == true);
    assert(res_rfc_kst.epoch == res_strict.epoch);
    assert(res_rfc_kst.millisecond == 789);
    std::cout << "  - parse_rfc3339_datetime (+09:00 & space separator): PASSED" << std::endl;

    // (6) RFC-3339 음수 오프셋 (-04:00)
    std::string rfc_neg = "2026-08-08T08:34:56.789-04:00";
    auto res_rfc_neg = dtutil::parse_rfc3339_datetime(rfc_neg, time_zone_mode::utc);
    assert(res_rfc_neg.ok == true);
    assert(res_rfc_neg.epoch == res_strict.epoch);
    assert(res_rfc_neg.millisecond == 789);
    std::cout << "  - parse_rfc3339_datetime (-04:00): PASSED" << std::endl;

    // (7) 자동 판별 파싱 (RFC3339 토큰 사용)
    auto res_auto_rfc = dtutil::parse_datetime_auto(rfc_kst, "RFC3339", time_zone_mode::utc);
    assert(res_auto_rfc.ok == true);
    assert(res_auto_rfc.epoch == res_strict.epoch);
    std::cout << "  - parse_datetime_auto (RFC3339): PASSED" << std::endl;

    // (8) 실패 케이스 검증
    auto res_fail = dtutil::parse_strict_datetime("2026-02-29 10:00:00", "YYYY-MM-DD hh:mm:ss", time_zone_mode::utc);
    assert(res_fail.ok == false);
    assert(!res_fail.error.empty());

    auto res_rfc_invalid_offset = dtutil::parse_rfc3339_datetime("2026-08-08T12:34:56+09", time_zone_mode::utc);
    assert(res_rfc_invalid_offset.ok == false);
    std::cout << "  - Parse error handling: PASSED" << std::endl;
}
