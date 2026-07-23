#pragma once

#include <functional>
#include <string>
#include <algorithm>
#include <chrono>

#include "mino/external/schedule/weekly/schedule_types.hpp"
#include "mino/external/schedule/weekly/scheduler_time_base.hpp"
#include "mino/external/schedule/weekly/schedule_xml.hpp"
#include "mino/external/schedule/weekly/schedule_dump.hpp"
#include "mino/external/schedule/weekly/schedule_normalizer.hpp"

namespace mino::external::schedule::weekly {

    // 주간 스케줄러 클래스
    class  scheduler {
    public:
        explicit scheduler(time_base base = time_base::localtime);

        // 타임 포인트로 현재 시각 제공자 설정
        void set_now_provider(std::function<std::chrono::system_clock::time_point()> now_provider);
 
        // 요일, 시, 분으로 현재 시각 설정 
        void set_now(weekday wd, int hour, int minute);

        // 타임 포인트로 현재 시각 설정
        void set_now(std::chrono::system_clock::time_point tp);

        // JSON 문자열로부터 스케줄을 로드 (기존 범위는 추가됨)
        // (헤더에는 nlohmann::json 포함하지 않음 — 문자열 기반 API)
        void load_from_json_string(const std::string& json_text);

        // 현재 설정된 시간이 스케쥴에 해당되는지 여부 판정
        bool is_active_now() const;
        bool is_active_now(std::function<std::chrono::system_clock::time_point()> now_provider_tp) const;

        // 주간 스케쥴을 추가
        void add_range(const weekly_range& r);

        // 현재 설정된 주간 스케쥴들을 반환
        const weekly_ranges& ranges() const;
         
    private:
        // 내부 헬퍼: 특정 tm 기준으로 활성 여부 판정
        bool is_active_at_tm(const std::tm& tm) const;

        scheduler_time_base time_base_;
        weekly_ranges ranges_;
        // now_provider_는 time_point을 반환하는 프로바이더로 변경
        std::function<std::chrono::system_clock::time_point()> now_provider_;
    };

    // 요일, 시, 분으로 tm 구조체 생성
    std::tm make_tm_with_wday_hour_min(
        weekday wd,
        int hour,
        int minute
    );

}
