#include <iostream>
#include <cassert>
#include <string>
#include <chrono>

#include "mino/core/datetime/unit/date_time.hpp"
#include "mino/core/datetime/util/datetime_util.hpp"

#include "mino/core/schedule/monthly/monthly.hpp"

int main() {
    namespace mcsm = mino::core::schedule::monthly;
    namespace mcsw = mino::core::schedule::weekly;
    namespace mdtu = mino::core::datetime::unit;
    namespace mdt = mino::core::datetime::util;

    std::cout << "=========================================================\n";
    std::cout << " Monthly Scheduler: Full Feature Verification Test Suite \n";
    std::cout << "=========================================================\n\n";

    mcsm::scheduler s;

    // -------------------------------------------------------------
    // [검증 1] 공휴일 등록 (단발성 vs 매년 반복) 및 주말 옵션
    // -------------------------------------------------------------
    bool exclude_sat = true; // 토요일 휴무 지정
    bool exclude_sun = true; // 일요일 휴무 지정
    s.calendar().set_weekend_exclusions(exclude_sat, exclude_sun); // 토/일 휴무 지정

    // (A) 매년 반복되는 공휴일: 월/일만 입력 (신정 1/1, 어린이날 5/5)
    s.calendar().add_recurring_holiday(1, 1);
    s.calendar().add_recurring_holiday(5, 5);

    // (B) 단발성 공휴일: 년/월/일 입력 (2026-03-02 대체공휴일)
    s.calendar().add_holiday(mcsm::date_ymd{ 2026, 3, 2 });

    // 매년 5월 5일은 공휴일이어야 함
    assert(s.calendar().is_holiday(mcsm::date_ymd{ 2026, 5, 5 }) == true);
    assert(s.calendar().is_holiday(mcsm::date_ymd{ 2027, 5, 5 }) == true);

    // 단발성 공휴일은 2026년에만 공휴일이어야 함
    assert(s.calendar().is_holiday(mcsm::date_ymd{ 2026, 3, 2 }) == true);
    assert(s.calendar().is_holiday(mcsm::date_ymd{ 2027, 3, 2 }) == false);

    std::cout << "[Pass 1] Recurring (MM-DD) vs One-time (YYYY-MM-DD) holidays verified.\n";

    // -------------------------------------------------------------
    // [검증 2] 매년 1월 10일 02:10:00 실행 (요구사항 1)
    // -------------------------------------------------------------
    mcsm::monthly_rule r1;

    r1.id = "Annual_Jan_10";
    r1.pattern = mcsm::pattern_type::day_of_month; // 특정 일자 패턴
    r1.day_of_month = 10; // 10일
    r1.target_months = { 1 }; // 매년 1월
    r1.execution_time = mcsm::time_hms{ 2, 10, 0 }; // 02:10:00

    s.add_rule(r1); // 스케줄러(s)에 규칙(r1) 추가

    s.set_now(mcsm::date_ymd{ 2026, 1, 10 }, mcsm::time_hms{ 2, 10, 0 }); // 2026년 1월 10일 02:10:00
    assert(s.is_active_now(true) == true); 

    s.set_now(mcsm::date_ymd{ 2026, 2, 10 }, mcsm::time_hms{ 2, 10, 0 }); // 2026년 2월 10일 02:10:00에는 false
    assert(s.is_active_now(true) == false);

    std::cout << "[Pass 2] Annual schedule (Every year Jan 10 02:10:00) verified.\n";

    // -------------------------------------------------------------
    // [검증 3] 매월 2주째 목요일 실행 (요구사항 2)
    // 2026년 1월: 1일(1주차 목요일), 8일(2주차 목요일)
    // -------------------------------------------------------------
    mcsm::monthly_rule r2;

    r2.id = "Monthly_2nd_Thu";
    r2.pattern = mcsm::pattern_type::ordinal_weekday; // 특정 요일/주차 패턴
    r2.target_ordinal = mcsm::week_ordinal::second; // 매월 2주차 (1~5주차까지 설정 가능)
    r2.target_weekday = mcsw::weekday::thu; // 목요일 (0=월, ... 6=일)
    r2.execution_time = mcsm::time_hms{ 10, 0, 0 }; // 10:00:00

    s.add_rule(r2); // 스케줄러(s)에 규칙(r2) 추가

    s.set_now(mcsm::date_ymd{ 2026, 1, 8 }, mcsm::time_hms{ 10, 0, 0 }); // 2026-01-08은 1월의 2주차 목요일
    assert(s.is_active_now(true) == true);

    s.set_now(mcsm::date_ymd{ 2026, 1, 15 }, mcsm::time_hms{ 10, 0, 0 }); // 3주차 목요일은 false
    assert(s.is_active_now(true) == false);

    std::cout << "[Pass 3] Ordinal weekday schedule (2nd Thursday) verified.\n";

    // -------------------------------------------------------------
    // [검증 4] 공휴일 정책: 다음 영업일 롤링 (요구사항 3)
    // 2026-01-01(목, 신정) -> 다음 날인 2026-01-02(금, 정상 영업일)로 롤링
    // -------------------------------------------------------------
    mcsm::monthly_rule r_hol;

    r_hol.pattern = mcsm::pattern_type::day_of_month; // 특정 일자 패턴
    r_hol.day_of_month = 1; // 매월 1일
    r_hol.execution_time = mcsm::time_hms{ 10, 0, 0 }; // 10:00:00
    r_hol.policy = mcsm::holiday_policy::next_business_day; // 공휴일 정책: 다음 영업일로 롤링

    auto exec_hol = s.get_execution_date_time(r_hol, 2026, 1);
    // 2026년 1월의 실행 날짜/시간 계산 (1일 10:00:00을 사용)
    // 단, 1월 1일이 공휴일인지 검사 => 공휴일이면 다음 영업일로 롤링. (next_business_day)

    assert(exec_hol.has_value()); // 결과가 있어야 함
    assert(exec_hol->first == (mcsm::date_ymd{ 2026, 1, 2 })); // 2026-01-02로 롤링되어야 함
    assert(exec_hol->second == (mcsm::time_hms{ 10, 0, 0 })); // 시간은 그대로 10:00:00

    s.add_rule(r_hol); // 스케줄러(s)에 규칙(r_hol) 추가

    s.set_now(mcsm::date_ymd{ 2026, 1, 2 }, mcsm::time_hms{ 10, 0, 0 }); // 2026-01-02 10:00:00
    assert(s.is_active_now(true) == true); // 2026-01-02 10:00:00은 실행되어야 함

    std::cout << "[Pass 4] Holiday roll to next business day verified.\n";

    // -------------------------------------------------------------
    // [검증 5] N개월 주기, Blacklist 제외, 1회성 변경 (요구사항 4, 5)
    // -------------------------------------------------------------
    mcsm::monthly_rule r3;

    r3.id = "Quarterly_Job";
    r3.pattern = mcsm::pattern_type::day_of_month; // 특정 일자 패턴

    r3.anchor_year = 2026; // 기준 연도
    r3.anchor_month = 1; // 기준 월
    r3.day_of_month = 15; // 매월 15일
    r3.execution_time = mcsm::time_hms{ 14, 0, 0 }; // 14:00:00
    // [5-1] 2026년 1월 15일 14:00:00부터 시작

    r3.interval_months = 3; // [5-2] 3개월마다 실행 (1월, 4월, 7월, 10월, ...)

    r3.overrides.push_back(mcsm::one_time_override{ // [5-3] 1회성 변경 규칙
        mcsm::date_ymd{ 2026, 4, 15 }, // 원래 예정일
        mcsm::date_ymd{ 2026, 4, 16 }, // 변경된 실행일
        mcsm::time_hms{ 16, 30, 0 } // 변경된 실행시간
    });

    r3.excluded_dates.push_back(mcsm::date_ymd{ 2026, 7, 15 }); // [5-4] 1회성 제외 규칙 (7월 15일 제외)

    s.add_rule(r3); // 스케줄러(s)에 규칙(r3) 추가

    //----------------------------------------------
    // 1월 (정상)
    assert(s.get_execution_date_time(r3, 2026, 1)->first == (mcsm::date_ymd{ 2026, 1, 15 }));

    //----------------------------------------------
    // 2월 (주기 아님) (3개월에 한 번이므로 2월은 실행되지 않아야 함)
    assert(!s.get_execution_date_time(r3, 2026, 2).has_value());

    //----------------------------------------------
    // 4월 
    auto ov_res = s.get_execution_date_time(r3, 2026, 4); // 2026년 4월의 실행 날짜/시간 계산하기.
    // 4월 주기는 4월 15일 14:00:00이지만, 1회성 변경 규칙 [5-3]에 의해 4월 16일 16:30:00으로 변경됨.

    assert(ov_res.has_value());
    assert(ov_res->first == (mcsm::date_ymd{ 2026, 4, 16 }));
    assert(ov_res->second == (mcsm::time_hms{ 16, 30, 0 }));

    //----------------------------------------------
    // 7월 ([5-4] Blacklist 제외됨)
    // (1, 4, 7월은 3개월 주기이지만, 7월은 수동으로 제외하였으므로 실행되지 않아야 함)
    assert(!s.get_execution_date_time(r3, 2026, 7).has_value());

    std::cout << "[Pass 5] N-month interval, blacklist, and one-time override verified.\n";

    // -------------------------------------------------------------
    // [검증 6] 업무시간 윈도우(09:00~18:00) 필터링 (요구사항 6)
    // -------------------------------------------------------------
    mcsm::monthly_rule r4;

    r4.pattern = mcsm::pattern_type::day_of_month; // 특정 일자 패턴
    r4.day_of_month = 10; // 매월 10일

    // [6-1] 업무 시간 윈도우 설정
    r4.window.enabled = true; // 업무시간 윈도우 활성화
    r4.window.start_time = mcsm::time_hms{ 9, 0, 0 }; // 09:00:00
    r4.window.end_time = mcsm::time_hms{ 18, 0, 0 }; // 18:00:00

    r4.execution_time = mcsm::time_hms{ 20, 0, 0 }; // [6-2] 실행 시간 

    assert(!s.get_execution_date_time(r4, 2026, 1).has_value()); // 2026년 1월 시간 계산
    // 2026-01-10 20:00:00은 업무시간 윈도우(09:00~18:00)를 벗어나므로, false

    s.add_rule(r4); // 스케줄러(s)에 규칙(r4) 추가

    s.set_now(mcsm::date_ymd{ 2026, 1, 10 }, mcsm::time_hms{ 20, 0, 0 }); // 2026-01-10 20:00:00
    assert(s.is_active_now(true) == false); // 업무시간 윈도우를 벗어나므로 활성 상태가 아님
 
    std::cout << "[Pass 6] Active time window filter (business hours) verified.\n";

    // -------------------------------------------------------------
    // [검증 7] 실제 시스템 현재 시각 연동 (date_time 활용)
    // -------------------------------------------------------------

    mdtu::date_time current_dt = mdtu::date_time::current_date_time(); // 현재 시간
    std::string now_str = mdt::format_datetime(current_dt.to_tm(), "YYYY-MM-DD hh:mm:ss");
    std::cout << "\n[Current Real Time]: " << now_str << "\n";

    // 현재 시점과 정확히 일치하는 테스트용 규칙 생성
    mcsm::monthly_rule cur_rule;

    cur_rule.id = "Realtime_Current_Rule";
    cur_rule.pattern = mcsm::pattern_type::day_of_month; // 특정 일자 패턴
    cur_rule.day_of_month = current_dt.day(); // 현재 일자
    cur_rule.target_months = { current_dt.month() }; // 현재 월
    cur_rule.execution_time = mcsm::time_hms{  // 실행 시간
        current_dt.hour(), // 현재 시각
        current_dt.minute(), // 현재 분
        current_dt.second() // 현재 초
    };

    s.add_rule(cur_rule); // 스케줄러(s)에 현재 시각과 일치하는 규칙 추가

    // date_time의 time_point를 scheduler의 time_provider로 연결
    s.set_now_provider([current_dt]() { // 콜백 함수를 등록하면 동적으로 현재 시간을 제공할 수 있음
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(current_dt.to_time_point());
    });

    assert(s.is_active_now(true) == true); // 현재 시각과 정확히 일치하므로 활성 상태여야 함

    //------------------------------------------------
    // 현재 시간을 scheduler에 직접 설정하는 방법 예시
    //------------------------------------------------
    // 방법 1: time_point로 직접 설정 (권장)
    // mdtu::date_time dt = mdtu::date_time::current_date_time();
    // s.set_now(dt.to_time_point());
    //------------------------------------------------
    // 방법 2: 연/월/일/시/분/초로 분해하여 설정
    // mdtu::date_time dt = mdtu::date_time::current_date_time();
    // // dt.spec();   // time_zone::utc 또는 time_zone::local_time 반환
    // // dt.is_utc(); // true면 UTC, false면 local_time
    // std::tm tmv = dt.to_tm(); // std::tm 구조체로 변환. timezone 기준으로 변환된 값이 들어있음.
    // s.set_now(
    //     mcsm::date_ymd{ tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday },
    //     mcsm::time_hms{ tmv.tm_hour, tmv.tm_min, tmv.tm_sec }
    // );
    //------------------------------------------------

    //------------------------------------------------
    // 현재 시간이 계속 변하는 경우,
    // scheduler에 현재 시간을 주기적으로 업데이트하는 방법 
    //------------------------------------------------
    // (1) 시스템이 계속 실행되는 경우
    // mdtu::date_time dt = mdtu::date_time::current_date_time();
    // s.set_now(dt.to_time_point());
    // 
    // // 필요할 때만 검사
    // if (s.is_active_now(true)) {
    //     // 스케줄된 작업 실행
    // }
    //------------------------------------------------
    // (2) 폴링 루프 (예: 1초마다)
    // while (running) {
    //     mdtu::date_time current = mdtu::date_time::current_date_time();
    //     s.set_now(current.to_time_point());
    //     if (s.is_active_now(true)) {
    //         // 작업 실행. 또는 외부 쓰레드에 트리거를 전달.
    //     }
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
    //------------------------------------------------

    std::cout << "[Pass 7] Real-time date_time injection match verified (is_active_now = true).\n";

    // -------------------------------------------------------------
    // [검증 8] JSON & XML 직렬화 및 역직렬화 (반복 공휴일 포함)
    // -------------------------------------------------------------
    
    // JSON 직렬화 / 역직렬화 검증
    std::string json_data = s.export_to_json_string(2); // pretty print with 2-space indentation
    std::cout << "\n[Serialized JSON Data]:\n" << json_data << "\n";

    mcsm::scheduler s_json;
    assert(s_json.load_from_json_string(json_data) == true); // JSON 역직렬화 성공 여부 확인
    assert(s_json.calendar().recurring_holidays().size() == 2); // 반복 공휴일 2개 확인
    assert(s_json.calendar().holidays().size() == 1); // 단발성 공휴일 1개 확인
    assert(s_json.rules().size() == s.rules().size()); // 규칙 수 일치 확인

    // XML 직렬화 / 역직렬화 검증
    std::string xml_data = s.export_to_xml_string(true); // pretty print enabled
    std::cout << "\n[Serialized XML Data]:\n" << xml_data << "\n";

    mcsm::scheduler s_xml;
    assert(s_xml.load_from_xml_string(xml_data) == true); // XML 역직렬화 성공 여부 확인
    assert(s_xml.calendar().recurring_holidays().size() == 2); // 반복 공휴일 2개 확인
    assert(s_xml.calendar().holidays().size() == 1); // 단발성 공휴일 1개 확인
    assert(s_xml.rules().size() == s.rules().size()); // 규칙 수 일치 확인

    std::cout << "[Pass 8] JSON & XML round-trip serialization verified.\n";

    // 덤프 출력 확인
    std::cout << "\n---------- All Configured Rules Dump ----------\n";
    std::cout << mcsm::dump_rules(s.rules()); // 규칙 덤프 출력

    std::cout << "\n>>> ALL UNIT & INTEGRATION TESTS SUCCEEDED! <<<\n";
    return 0;
}
