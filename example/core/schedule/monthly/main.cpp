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
    s.calendar().set_weekend_exclusions(true, true); // 토/일 휴무 지정

    // (A) 매년 반복되는 공휴일: 월/일만 입력 (신정 1/1, 어린이날 5/5)
    s.calendar().add_recurring_holiday(1, 1);
    s.calendar().add_recurring_holiday(5, 5);

    // (B) 단발성 공휴일: 년/월/일 입력 (2026-03-02 대체공휴일)
    s.calendar().add_holiday(mcsm::date_ymd{ 2026, 3, 2 });

    // 2026년과 2027년 모두 5월 5일은 공휴일이어야 함
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
    r1.pattern = mcsm::pattern_type::day_of_month;
    r1.day_of_month = 10;
    r1.target_months = { 1 }; // 매년 1월
    r1.execution_time = mcsm::time_hms{ 2, 10, 0 };
    s.add_rule(r1);

    s.set_now(mcsm::date_ymd{ 2026, 1, 10 }, mcsm::time_hms{ 2, 10, 0 });
    assert(s.is_active_now(true) == true);

    s.set_now(mcsm::date_ymd{ 2026, 2, 10 }, mcsm::time_hms{ 2, 10, 0 }); // 2월에는 false
    assert(s.is_active_now(true) == false);
    std::cout << "[Pass 2] Annual schedule (Every year Jan 10 02:10:00) verified.\n";

    // -------------------------------------------------------------
    // [검증 3] 매월 2주째 목요일 실행 (요구사항 2)
    // 2026년 1월: 1일(1주차 목요일), 8일(2주차 목요일)
    // -------------------------------------------------------------
    mcsm::monthly_rule r2;
    r2.id = "Monthly_2nd_Thu";
    r2.pattern = mcsm::pattern_type::ordinal_weekday;
    r2.target_ordinal = mcsm::week_ordinal::second;
    r2.target_weekday = mcsw::weekday::thu;
    r2.execution_time = mcsm::time_hms{ 10, 0, 0 };
    s.add_rule(r2);

    s.set_now(mcsm::date_ymd{ 2026, 1, 8 }, mcsm::time_hms{ 10, 0, 0 });
    assert(s.is_active_now(true) == true);

    s.set_now(mcsm::date_ymd{ 2026, 1, 15 }, mcsm::time_hms{ 10, 0, 0 }); // 3주차 목요일은 false
    assert(s.is_active_now(true) == false);
    std::cout << "[Pass 3] Ordinal weekday schedule (2nd Thursday) verified.\n";

    // -------------------------------------------------------------
    // [검증 4] 공휴일 정책: 다음 영업일 롤링 (요구사항 3)
    // 2026-01-01(목, 신정) -> 다음 날인 2026-01-02(금, 정상 영업일)로 롤링
    // -------------------------------------------------------------
    mcsm::monthly_rule r_hol;
    r_hol.pattern = mcsm::pattern_type::day_of_month;
    r_hol.day_of_month = 1;
    r_hol.execution_time = mcsm::time_hms{ 10, 0, 0 };
    r_hol.policy = mcsm::holiday_policy::next_business_day;

    auto exec_hol = s.get_execution_date_time(r_hol, 2026, 1);
    assert(exec_hol.has_value());
    assert(exec_hol->first == (mcsm::date_ymd{ 2026, 1, 2 }));
    std::cout << "[Pass 4] Holiday roll to next business day verified.\n";

    // -------------------------------------------------------------
    // [검증 5] N개월 주기, Blacklist 제외, 1회성 변경 (요구사항 4, 5)
    // -------------------------------------------------------------
    mcsm::monthly_rule r3;
    r3.id = "Quarterly_Job";
    r3.pattern = mcsm::pattern_type::day_of_month;
    r3.day_of_month = 15;
    r3.interval_months = 3; // 3개월마다
    r3.anchor_year = 2026;
    r3.anchor_month = 1;
    r3.execution_time = mcsm::time_hms{ 14, 0, 0 };
    r3.excluded_dates.push_back(mcsm::date_ymd{ 2026, 7, 15 }); // 7월 제외
    r3.overrides.push_back(mcsm::one_time_override{
        mcsm::date_ymd{ 2026, 4, 15 },
        mcsm::date_ymd{ 2026, 4, 16 },
        mcsm::time_hms{ 16, 30, 0 }
        }); // 4월 오버라이드
    s.add_rule(r3);

    // 1월 (정상)
    assert(s.get_execution_date_time(r3, 2026, 1)->first == (mcsm::date_ymd{ 2026, 1, 15 }));
    // 2월 (주기 아님)
    assert(!s.get_execution_date_time(r3, 2026, 2).has_value());
    // 4월 (1회성 변경: 16일 16:30)
    auto ov_res = s.get_execution_date_time(r3, 2026, 4);
    assert(ov_res.has_value());
    assert(ov_res->first == (mcsm::date_ymd{ 2026, 4, 16 }));
    assert(ov_res->second == (mcsm::time_hms{ 16, 30, 0 }));
    // 7월 (Blacklist 제외됨)
    assert(!s.get_execution_date_time(r3, 2026, 7).has_value());
    std::cout << "[Pass 5] N-month interval, blacklist, and one-time override verified.\n";

    // -------------------------------------------------------------
    // [검증 6] 업무시간 윈도우(09:00~18:00) 필터링 (요구사항 6)
    // -------------------------------------------------------------
    mcsm::monthly_rule r4;
    r4.pattern = mcsm::pattern_type::day_of_month;
    r4.day_of_month = 10;
    r4.execution_time = mcsm::time_hms{ 20, 0, 0 }; // 업무시간 외 (20시)
    r4.window.enabled = true;
    r4.window.start_time = mcsm::time_hms{ 9, 0, 0 };
    r4.window.end_time = mcsm::time_hms{ 18, 0, 0 };

    assert(!s.get_execution_date_time(r4, 2026, 1).has_value());
    std::cout << "[Pass 6] Active time window filter (business hours) verified.\n";

    // -------------------------------------------------------------
    // [검증 7] 실제 시스템 현재 시각 연동 (date_time 활용)
    // -------------------------------------------------------------
    mdtu::date_time current_dt = mdtu::date_time::current_date_time();
    std::string now_str = mdt::format_datetime(current_dt.to_tm(), "YYYY-MM-DD hh:mm:ss");
    std::cout << "\n[Current Real Time]: " << now_str << "\n";

    // 현재 시점과 정확히 일치하는 테스트용 규칙 생성
    mcsm::monthly_rule cur_rule;
    cur_rule.id = "Realtime_Current_Rule";
    cur_rule.pattern = mcsm::pattern_type::day_of_month;
    cur_rule.day_of_month = current_dt.day();
    cur_rule.target_months = { current_dt.month() };
    cur_rule.execution_time = mcsm::time_hms{
        current_dt.hour(),
        current_dt.minute(),
        current_dt.second()
    };
    s.add_rule(cur_rule);

    // date_time의 time_point를 scheduler의 time_provider로 연결
    s.set_now_provider([current_dt]() {
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(current_dt.to_time_point());
        });

    assert(s.is_active_now(true) == true);
    std::cout << "[Pass 7] Real-time date_time injection match verified (is_active_now = true).\n";

    // -------------------------------------------------------------
    // [검증 8] JSON & XML 직렬화 및 역직렬화 (반복 공휴일 포함)
    // -------------------------------------------------------------
    // JSON 직렬화 / 역직렬화 검증
    std::string json_data = s.export_to_json_string(2);
    mcsm::scheduler s_json;
    assert(s_json.load_from_json_string(json_data) == true);
    assert(s_json.calendar().recurring_holidays().size() == 2);
    assert(s_json.calendar().holidays().size() == 1);
    assert(s_json.rules().size() == s.rules().size());

    // XML 직렬화 / 역직렬화 검증
    std::string xml_data = s.export_to_xml_string(true);
    mcsm::scheduler s_xml;
    assert(s_xml.load_from_xml_string(xml_data) == true);
    assert(s_xml.calendar().recurring_holidays().size() == 2);
    assert(s_xml.calendar().holidays().size() == 1);
    assert(s_xml.rules().size() == s.rules().size());
    std::cout << "[Pass 8] JSON & XML round-trip serialization verified.\n";

    // 덤프 출력 확인
    std::cout << "\n---------- All Configured Rules Dump ----------\n";
    std::cout << mcsm::dump_rules(s.rules());

    std::cout << "\n>>> ALL UNIT & INTEGRATION TESTS SUCCEEDED! <<<\n";
    return 0;
}
