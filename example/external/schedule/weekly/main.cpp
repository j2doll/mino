#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

#include "mino/core/schedule/weekly/schedule_types.hpp"
#include "mino/core/string/string.hpp"

#include "mino/external/schedule/weekly/schedule_json.hpp"

// 출력 및 콘솔 인코딩 유틸리티 설정
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

int main(int argc, char* argv[]) {
    namespace mcsw = mino::core::schedule::weekly;
    namespace mesw = mino::external::schedule::weekly;

    print(tce("========================================"));
    print(tce("  Schedule JSON Unit Tests Starting     "));
    print(tce("========================================"));

    // -------------------------------------------------------------------------
    // Test 1: to_json_string 직렬화 테스트
    // -------------------------------------------------------------------------
    {
        print(tce("\n[Test 1] to_json_string() 직렬화 검증..."));

        mcsw::weekly_ranges ranges{
            mcsw::weekly_range{ mcsw::weekday::tue, {23,  0}, mcsw::weekday::wed, {10, 30} },
            mcsw::weekly_range{ mcsw::weekday::sat, {12, 15}, mcsw::weekday::sun, {18, 45} }
        };
        // 화요일 23:00 ~ 수요일 10:30
        // 토요일 12:15 ~ 일요일 18:45

        std::string json_str = mesw::to_json_string(ranges); // 직렬화 수행
        print(tce("생성된 JSON 문자열: "), tce(json_str));
        // [{"end_day":"Wed","end_h":10,"end_m":30,"start_day":"Tue","start_h":23,"start_m":0},{"end_day":"Sun","end_h":18,"end_m":45,"start_day":"Sat","start_h":12,"start_m":15}]

        nlohmann::json j = nlohmann::json::parse(json_str); // 놀먼 JSON 파싱
        if (!j.is_array() || j.size() != 2) {
            eprint(tce("Test 1 Failed: JSON 배열 크기가 올바르지 않습니다."));
            return 1;
        }

        // 첫 번째 스케쥴의 시작 요일, 시간, 분 검증
        assert(j[0]["start_day"] == "Tue");
        assert(j[0]["start_h"] == 23);
        assert(j[0]["start_m"] == 0);

        // 첫 번째 스케쥴의 종료 요일, 시간, 분 검증
        assert(j[0]["end_day"] == "Wed");
        assert(j[0]["end_h"] == 10);
        assert(j[0]["end_m"] == 30);

        // 두 번째 스케쥴의 시작 요일, 시간, 분 검증
        assert(j[1]["start_day"] == "Sat");
        assert(j[1]["start_h"] == 12);
        assert(j[1]["start_m"] == 15);

        // 두 번째 스케쥴의 종료 요일, 시간, 분 검증
        assert(j[1]["end_day"] == "Sun");
        assert(j[1]["end_h"] == 18);
        assert(j[1]["end_m"] == 45);

        print(tce("-> [PASS] 직렬화 검증 성공"));
    }

    // -------------------------------------------------------------------------
    // Test 2: from_json_string 역직렬화 (문자열 요일 및 대소문자/공백 처리 검증)
    // -------------------------------------------------------------------------
    {
        print(tce("\n[Test 2] from_json_string() 문자열 요일 파싱 검증..."));

        std::string json_text = R"([
            {
                "start_day": "  mon  ", 
                "start_h": 9, 
                "start_m": 0, 
                "end_day": "FRI", 
                "end_h": 18, 
                "end_m": 0
            }
        ])";

        auto result_opt = mesw::from_json_string(json_text); // 역직렬화 수행
        if (!result_opt.has_value()) {
            eprint(tce("Test 2 Failed: 문자열 요일 파싱 실패"));
            return 1;
        }

        const auto& parsed_ranges = *result_opt;

        assert(parsed_ranges.size() == 1); // 1개 스케쥴

        // 시작 요일, 시간, 분 검증
        assert(parsed_ranges[0].start_day == mcsw::weekday::mon);
        assert(parsed_ranges[0].start_time.hour == 9);
        assert(parsed_ranges[0].start_time.minute == 0);

        // 종료 요일, 시간, 분 검증
        assert(parsed_ranges[0].end_day == mcsw::weekday::fri);
        assert(parsed_ranges[0].end_time.hour == 18);
        assert(parsed_ranges[0].end_time.minute == 0);

        print(tce("-> [PASS] 대소문자 및 공백 처리 포함 문자열 요일 파싱 성공"));
    }

    // -------------------------------------------------------------------------
    // Test 3: from_json_string 역직렬화 (정수형 요일 지원 검증)
    // -------------------------------------------------------------------------
    {
        print(tce("\n[Test 3] from_json_string() 정수형 요일 파싱 검증..."));

        std::string json_text = R"([
            {
                "start_day": 0, 
                "start_h": 10, 
                "start_m": 30, 
                "end_day": 1, 
                "end_h": 12, 
                "end_m": 0
            }
        ])";

        auto result_opt = mesw::from_json_string(json_text); // 역직렬화 수행
        if (!result_opt.has_value()) {
            eprint(tce("Test 3 Failed: 정수형 요일 파싱 실패"));
            return 1;
        }

        const auto& parsed_ranges = *result_opt;

        assert(parsed_ranges.size() == 1); // 1개 스케쥴

        // 정수형 요일 검증 (0: 월요일, 1: 화요일)
        assert(static_cast<int>(parsed_ranges[0].start_day) == 0);
        assert(static_cast<int>(parsed_ranges[0].end_day) == 1);

        // 시작 시간, 분 검증
        assert(parsed_ranges[0].start_time.hour == 10);
        assert(parsed_ranges[0].start_time.minute == 30);

        print(tce("-> [PASS] 정수형 요일 파싱 성공"));
    }

    // -------------------------------------------------------------------------
    // Test 4: from_json_string 실패 케이스 (std::nullopt 반환 검증)
    // -------------------------------------------------------------------------
    {
        print(tce("\n[Test 4] from_json_string() 비정상 JSON 실패 처리 검증..."));

        // 1) 배열이 아닌 경우
        assert(!mesw::from_json_string(R"({"start_day":"Mon"})").has_value());
        // JSON이 배열이 아니므로 실패

        // 2) 필수 필드가 누락된 경우 (end_m 누락)
        std::string missing_field = R"(
            [{
                "start_day":"Mon",
                "start_h":10,
                "start_m":0,
                "end_day":"Tue",
                "end_h":11
            }]
        )";
        assert(!mesw::from_json_string(missing_field).has_value());
        // end_m 필드가 누락되어 있으므로 실패

        // 3) 잘못된 요일 문자열
        std::string invalid_day_str = R"(
            [{
                "start_day":"InvalidDay",
                "start_h":10,
                "start_m":0,
                "end_day":"Tue",
                "end_h":11,
                "end_m":0
            }]
        )";
        assert(!mesw::from_json_string(invalid_day_str).has_value());
        // start_day가 "InvalidDay"이며, 이는 유효한 요일 문자열이 아니므로 실패

        // 4) 잘못된 요일 숫자 (0~6 범위 밖)
        std::string invalid_day_int = R"(
            [{
                "start_day":7,
                "start_h":10,
                "start_m":0,
                "end_day":1,
                "end_h":11,
                "end_m":0
            }]
        )";
        assert(!mesw::from_json_string(invalid_day_int).has_value());
        // start_day가 7이며, 이는 0(월요일) ~ 6(일요일) 범위를 벗어나므로 실패

        print(tce("-> [PASS] 모든 비정상 입력에 대해 nullopt 정상 반환 확인"));
    }

    // -------------------------------------------------------------------------
    // Test 5: Round-trip (직렬화 -> 역직렬화) 일치성 검증
    // -------------------------------------------------------------------------
    {
        print(tce("\n[Test 5] Round-trip (직렬화 -> 역직렬화) 일치성 검증..."));

        mcsw::weekly_ranges origin{
            mcsw::weekly_range{ mcsw::weekday::mon, {1, 10}, mcsw::weekday::mon, {2, 20} },
            mcsw::weekly_range{ mcsw::weekday::wed, {15, 0}, mcsw::weekday::thu, {3, 30} },
            mcsw::weekly_range{ mcsw::weekday::sun, {20, 0}, mcsw::weekday::mon, {5, 0} }
        };
        // 월요일 01:10 ~ 월요일 02:20
        // 수요일 15:00 ~ 목요일 03:30
        // 일요일 20:00 ~ 월요일 05:00

        std::string serialized = mesw::to_json_string(origin); // 직렬화 수행
        auto restored_opt = mesw::from_json_string(serialized); // 역직렬화 수행

        if (!restored_opt.has_value()) {
            eprint(tce("Test 5 Failed: 복원 실패"));
            return 1;
        }

        const auto& restored = *restored_opt;
        assert(origin.size() == restored.size());

        // 각 스케쥴 항목의 시작 요일, 시간, 분 및 종료 요일, 시간, 분 일치 여부 검증
        for (size_t i = 0; i < origin.size(); ++i) {
            assert(origin[i].start_day == restored[i].start_day);
            assert(origin[i].start_time.hour == restored[i].start_time.hour);
            assert(origin[i].start_time.minute == restored[i].start_time.minute);
            assert(origin[i].end_day == restored[i].end_day);
            assert(origin[i].end_time.hour == restored[i].end_time.hour);
            assert(origin[i].end_time.minute == restored[i].end_time.minute);
        }

        print(tce("-> [PASS] 원본 데이터와 복원 데이터 일치 확인"));
    }

    print(tce("\n========================================"));
    print(tce("  All Schedule JSON Tests Passed!       "));
    print(tce("========================================"));

    return 0;
}
