#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <optional>
#include <chrono>

#include "mino/core/datetime/v0/datetime_common.hpp"

namespace mino::core::datetime {
    namespace v0 {

        // ---------------- 헬퍼 ----------------
        //

        // 문자열에서 ndig 자리수 정수 읽기
        // 인자:
        //  s: 입력 문자열
        //  pos: 읽기 시작 위치(성공 시 ndig만큼 증가)
        //  ndig: 읽을 자리수
        //  out: 읽은 정수 값을 받을 참조
        // 반환: 성공 시 true, 실패 시 false
        // 예제:
        //  s="2025-10-17", pos=0, ndig=4 → out=2025, pos=4, 반환 true
        //  s="2025-10-17", pos=5, ndig=2 → out=10, pos=7, 반환 true
         bool read_ndigits(const std::string& s, size_t& pos, int ndig, int& out);

        // format의 pos 위치에서 tok 토큰과 일치하는지 검사
        // 인자:
        //  fmt: 형식 문자열
        //  pos: 검사 시작 위치
        //  tok: 검사할 토큰 문자열(예: "YYYY", "MM" 등)
        // 반환: 일치하면 true, 아니면 false
        // 예제:
        //  fmt="YYYY-MM-DD", pos=0, tok="YYYY" → 반환
         bool match_token(const std::string& fmt, size_t pos, const char* tok);

        // 윤년 여부 반환
        // 인자:
        //  y: 연도(예: 2024)
        // 반환: 윤년이면 true, 아니면 false
         bool is_leap(int y);

        // 유효한 연월일(YYYY-MM-DD)인지 검사
        // 인자:
        //  Y: 연도(예: 2025)
        //  M: 월(1~12)
        //  D: 일(1~31)
        // 반환: 유효하면 true, 아니면 false
        // 예제:
        //  valid_ymd(2025, 2, 29) → false (2025년은 윤년 아님)
         bool valid_ymd(int Y, int M, int D);

        // 현재 시각을 지정된 타임존 모드(UTC/Localtime)로 얻기
        // 인자:
        //  tz: 타임존 모드
        // 반환: 현재 시각의 std::tm 구조체
         std::tm now_in_zone(time_zone_mode tz);

        // 오늘 날짜(00:00:00)를 지정된 타임존 오프셋(초)으로 얻기
        // 인자:
        //  offset_sec: UTC로부터의 오프셋(초)
        // 반환: 오늘 날짜의 std::tm 구조체
        // 예제:
        //  offset_sec=0 → UTC 오늘 날짜
        //  offset_sec=32400 → UTC+9 오늘 날짜
         std::tm today_in_fixed_offset_seconds(int offset_sec);

        // ---------------- 포맷터 유틸 ----------------

        // 내부 유틸: 0 채움 숫자 쓰기
        // 인자:
        //  out: 결과 문자열 버퍼
        //  value: 쓸 정수 값
         void append_ndigits(std::string& out, int value, int width);

        // 내부 유틸: format의 pos 위치에서 tok 토큰과 일치하는지 검사
        // 인자:
        //  fmt: 형식 문자열 
        //  pos: 검사 시작 위치
        //  tok: 검사할 토큰 문자열(예: "YYYY", "MM" 등)
        // 반환: 일치하면 true, 아니면 false
         bool fmt_match(const std::string& fmt, size_t pos, const char* tok);

        // 내부 유틸: std::tm 구조체를 format에 따라 포맷
        // 인자:
        //  tmv: 포맷할 std::tm 구조체
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS"
        //  milliseconds: 선택적 밀리초 값(0..999). 음수면 밀리초 정보 없음(기본값)
        // 반환: 포맷된 문자열
         std::string format_from_tm_core(
            const std::tm& tmv,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS",
            int milliseconds = -1);

        // ---------------- 포맷터 API ----------------

        // (1) std::tm + format
        // 인자:
        //  tmv: 포맷할 std::tm 구조체 (tm에는 타임존 정보가 없음)
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS"
        // 반환: 포맷된 문자열
         std::string format_datetime(const std::tm& tmv,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS");

        // (2) time_t + tzmode + format
        // 인자:
        //  t: 포맷할 time_t 값 (epoch seconds)
        //  tzmode: t를 UTC/Localtime 중 어느 타임존으로 변환할지 지정
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS"
        // 반환: 포맷된 문자열
         std::string format_datetime(
            std::time_t t,
            time_zone_mode tzmode = time_zone_mode::local_time,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS");

        // (3) time_point + tzmode + format
        // 인자:
        //  tp: 포맷할 time_point 값 (epoch time_point. nanoseconds precision)
        //  tzmode: tp를 UTC/Localtime 중 어느 타임존으로 변환할지 지정
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS"
        // 반환: 포맷된 문자열    
         std::string format_datetime(
            const std::chrono::system_clock::time_point& tp,
            time_zone_mode tzmode = time_zone_mode::local_time,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS");

        // (4) epoch_ms + tzmode + format
        // 인자:
        //  epoch_ms: 포맷할 epoch 밀리초 값
        //  tzmode: epoch_ms를 UTC/Localtime 중 어느 타임존으로 변환할지 지정
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS"
        // 반환: 포맷된 문자열
         std::string format_datetime(
            std::uint64_t epoch_ms,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS",
            time_zone_mode tzmode = time_zone_mode::local_time);

         weekday get_weekday(const uint32_t year, const uint32_t month, const uint32_t day, time_zone_mode tz = time_zone_mode::local_time);

    } // namespace v0
} // namespace mino::core::datetime
