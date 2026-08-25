#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <string>

#include "mino/core/string/string.hpp"
#include "mino/core/schedule/weekly/weekly.hpp"
#include "mino/core/json/json.hpp"

int main() {
    namespace mcsw = mino::core::schedule::weekly;
    using weekly_range = mcsw::weekly_range;
    using weekly_ranges = mcsw::weekly_ranges;
    using weekday = mcsw::weekday;
    using time_base = mcsw::time_base;
    using scheduler = mcsw::scheduler;
    using schedule_normalizer = mcsw::schedule_normalizer;

    // 출력 헬퍼 정의
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    // 콘솔 인코딩 변환기 별칭 지정
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce("  Weekly Schedule Module Test Suite     "));
    print(tce("========================================"), endl);

    // ----------------------------------------------------
    // 1. 테스트용 스케줄 데이터 생성[cite: 4]
    // ----------------------------------------------------
    weekly_range work_hours{
        weekday::mon, {9, 0}, // 월요일 09:00
        weekday::mon, {18, 0} // 월요일 18:00
    };

    weekly_range overnight_maintenance{
        weekday::fri, {22, 0}, // 금요일 22:00
        weekday::sat, {4, 0}   // 토요일 04:00
    };

    weekly_ranges sample_ranges = { work_hours, overnight_maintenance };

    // ----------------------------------------------------
    // 2. dump_schedule() 테스트[cite: 1]
    // ----------------------------------------------------
    print(tce("[TEST 1] dump_schedule() Output:"));
    std::string dumped = mcsw::dump_schedule(sample_ranges); // 스케줄 덤프 문자열 생성
    print(tce(dumped)); // 스케쥴 출력
    assert(!dumped.empty());
    // [TEST 1] dump_schedule() Output:
    // Mon 9:0 -> Mon 18:0
    // Fri 22:0 -> Sat 4:0

    // ----------------------------------------------------
    // 3. schedule_normalizer 테스트[cite: 3]
    // ----------------------------------------------------
    print(tce("[TEST 2] schedule_normalizer::normalize():"));
    weekly_ranges normalized = schedule_normalizer::normalize(sample_ranges); // 스케줄 정규화 (중복 제거 및 범위 병합)
    print(tce("Normalized ranges count: "), normalized.size()); // 정규화된 범위 수 출력
    print(tce(mcsw::dump_schedule(normalized))); // 정규화된 스케줄 출력
    // [TEST 2] schedule_normalizer::normalize():
    // Normalized ranges count: 3
    // Mon 9:0 -> Mon 18:0
    // Fri 22:0 -> Fri 23:59
    // Sat 0:0 -> Sat 4:0 

    // ----------------------------------------------------
    // 4. to_xml() 테스트[cite: 5]
    // ----------------------------------------------------
    print(tce("[TEST 3] to_xml() Output:"));
    std::string xml_output = mcsw::to_xml(sample_ranges); // 스케줄을 XML 형식으로 변환
    print(tce(xml_output), endl);
    // [TEST 3] to_xml() Output:
    // <?xml version="1.0" encoding="UTF-8"?>
    // <WeeklySchedule>
    //   <Range start_day="Mon" start_h="9" start_m="0" end_day="Mon" end_h="18" end_m="0" />
    //   <Range start_day="Fri" start_h="22" start_m="0" end_day="Sat" end_h="4" end_m="0" />
    // </WeeklySchedule>

    // ----------------------------------------------------
    // 5. scheduler 활성 상태(is_active_now) 테스트[cite: 6]
    // ----------------------------------------------------
    print(tce("[TEST 4] scheduler is_active_now() Tests:"));
    scheduler sched(time_base::localtime); // 로컬 타임 기준 스케줄러 생성
    sched.add_range(work_hours); // 근무 시간 범위 추가 (월요일 09:00 ~ 18:00)
    sched.add_range(overnight_maintenance); // 야간 작업 범위 추가 (금요일 22:00 ~ 토요일 04:00)

    // 테스트 케이스별로 활성 상태 확인 및 결과 출력
    auto test_and_print = [&](const std::string& test_name, bool condition) {
        if (condition)
        { print(tce("[PASS] "), tce(test_name)); }
        else
        { eprint(tce("[FAIL] "), tce(test_name)); }
    };

    // Case 5-1: 월요일 08:59 (근무 시작 1분 전 -> 비활성)
    sched.set_now(weekday::mon, 8, 59);
    test_and_print("Mon 08:59 (Before work hours -> Inactive)", !sched.is_active_now());
    // [PASS] Mon 08:59 (Before work hours -> Inactive)

    // Case 5-2: 월요일 09:00 (근무 시작 정각 -> 활성)
    sched.set_now(weekday::mon, 9, 0);
    test_and_print("Mon 09:00 (Work start boundary -> Active)", sched.is_active_now());
    // [PASS] Mon 09:00 (Work start boundary -> Active)

    // Case 5-3: 월요일 12:30 (근무 시간 중 -> 활성)
    sched.set_now(weekday::mon, 12, 30);
    test_and_print("Mon 12:30 (During work hours -> Active)", sched.is_active_now());
    // [PASS] Mon 12:30 (During work hours -> Active)

    // Case 5-4: 월요일 18:00 (근무 종료 시간 확인)
    sched.set_now(weekday::mon, 18, 0);
    print(tce("  - Mon 18:00 Active status: "), std::boolalpha, sched.is_active_now());
    //   - Mon 18:00 Active status: true

    // Case 5-5: 수요일 14:00 (설정되지 않은 요일 -> 비활성)
    sched.set_now(weekday::wed, 14, 0);
    test_and_print("Wed 14:00 (Unscheduled day -> Inactive)", !sched.is_active_now());
    // [PASS] Wed 14:00 (Unscheduled day -> Inactive)

    // Case 5-6: 금요일 23:30 (야간 작업 시간 -> 활성)
    sched.set_now(weekday::fri, 23, 30);
    test_and_print("Fri 23:30 (Overnight maintenance -> Active)", sched.is_active_now());
    // [PASS] Fri 23:30 (Overnight maintenance -> Active)

    // Case 5-7: 토요일 02:00 (요일 넘어간 야간 작업 시간 -> 활성)
    sched.set_now(weekday::sat, 2, 0);
    test_and_print("Sat 02:00 (Overnight maintenance next day -> Active)", sched.is_active_now());
    // [PASS] Sat 02:00 (Overnight maintenance next day -> Active)

    // Case 5-8: 토요일 05:00 (야간 작업 종료 후 -> 비활성)
    sched.set_now(weekday::sat, 5, 0);
    test_and_print("Sat 05:00 (After maintenance -> Inactive)", !sched.is_active_now());
    // [PASS] Sat 05:00 (After maintenance -> Inactive)

    // ----------------------------------------------------
    // 6. std::function 기반 now_provider 테스트[cite: 6]
    // ----------------------------------------------------
    print(endl, tce("[TEST 5] scheduler with custom now_provider:"));
    sched.set_now_provider([]() {
        return std::chrono::system_clock::now(); // 현재 시스템 시각을 반환하는 람다 함수
    }); // 현재 시스템 시각을 반환하는 now_provider 설정
    print(tce("Current Real-time Active status: "), std::boolalpha, sched.is_active_now());
    // Current Real - time Active status : true(현재 시간이 스케쥴 안에 있는 경우) 또는 false

    {
        std::cout << std::endl << "-------------------------------"  << std::endl;

        namespace mcsw = mino::core::schedule::weekly;
        using weekly_ranges = mcsw::weekly_ranges;
        using weekday = mcsw::weekday;

        // 예제 스케줄 생성
        weekly_ranges ranges;

        weekly_range r1;
        r1.start_day = weekday::mon; r1.start_time = {  8, 30 };   
        r1.end_day   = weekday::fri; r1.end_time   = { 17,  0 };    
        ranges.push_back(r1);
        // 월요일 08:30 ~ 금요일 17:00

        weekly_range r2;
        r2.start_day = weekday::sat; r2.start_time = { 10,  0 };  
        r2.end_day   = weekday::sat; r2.end_time   = { 12, 30 };
        ranges.push_back(r2);
        // 토요일 10:00 ~ 토요일 12:30

        auto origin_dump = mcsw::dump_schedule(ranges); // 스케줄 덤프 출력
        std::cout << "Original weekly_ranges:\n" << origin_dump << "\n\n";

        // 직렬화 (주단위 일정 -> JSON 문자열)
        int indent = 4; // 들여쓰기 4칸
        std::string json_text = mcsw::to_json_string(ranges, indent);
        std::cout << "Serialized JSON:\n" << json_text << "\n\n";

        // 역직렬화 (JSON 문자열 -> 주단위 일정)
        std::optional<weekly_ranges> parsed = mcsw::from_json_string(json_text);
        if (!parsed.has_value()) {
            std::cerr << "Failed to parse JSON into weekly_ranges\n";
            return 1;
        }

        // 파싱된 결과 출력 (사람이 읽기 쉬운 형태)
        auto parsed_value = parsed.value();
        auto parsed_dump = mcsw::dump_schedule(parsed_value);
        std::cout << "Parsed weekly_ranges:\n" << parsed_dump << std::endl;
    }

    print(endl, tce("All tests completed successfully!"));
    return 0;
}
