#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <optional>
#include <chrono>
#include <cctype>
#include <limits>

#include "mino/core/datetime/util/datetime_common.hpp"

namespace mino::core::datetime {
    namespace util {

        // - C++17 전용 날짜/시간 파서 
        // - 두 가지 모드:
        //   (A) 형식 엄격 일치: "YYYY, MM, DD, hh, mm, ss, SSS" 토큰과 리터럴을
        //       정확히 매칭
        //       · 형식에 없는 토큰은 tzmode 기준의 현재 날짜/시간으로 보정
        //       · 예: "mm:ss" → 시/년월일은 현재, 분/초는 입력값
        //   (B) ISO-8601: YYYY-MM-DDThh:mm[:ss][.frac][Z|±HH:MM|±HHMM|±HH]
        //       · 날짜가 없고 시간만 → 현재 날짜 보정
        //       · 분수초는 1~9자리 허용, 밀리초는 앞 3자리 사용(부족 시 0 패딩)
        // - 타임존 처리:
        //   · (A) 형식 파서: 인자의 time_zone_mode(UTC/local_time) 기준의 현재를 사용
        //   · (B) ISO 파서: 문자열의 Z/±오프셋이 우선, 없으면 time_zone_mode 기준
        // - 결과: time_t(초), epoch_ms(밀리초), time_point(system_clock) 제공
        //
        // 주의:
        // - UTC 변환은 timegm(리눅스/맥)/_mkgmtime(윈도우)에 의존합니다.

        // 형식 파서에서 실제 등장한 토큰 표기
        // YYYY : 년, MM : 월, DD : 일, hh : 시, mm : 분, ss : 초, SSS : 밀리초
        struct  present_flags {
            bool has_year = false;
            bool has_month = false;
            bool has_day = false;
            bool has_hour = false;
            bool has_minute = false;
            bool has_second = false;
            bool has_millisecond = false;
        };

        // 시간 문자열 파싱 결과 구조체
        struct  date_time_parse_result {
            bool ok = false; // 시간 정보 파싱 성공 여부.
            std::string error; // 시간 정보 파싱 실패 사유. 실패(ok==false) 시에만 설정됨.

            //-----------------------------------------------------------------
            // NOTE: 성공(ok ==true) 시, 다음의 세 가지 시간 결과가 모두 설정됨.
            //       설정되는 시간은 모두 같은 시각을 나타냄.
            //       실패 시에는 모두 0 또는 std::nullopt.

            // [1] UNIX epoch 결과 
            std::time_t epoch = 0; // UNIX epoch (단위: 초). 1970-01-01 00:00:00 UTC부터의 경과 초(second) 수.
            int millisecond = 0;   // 밀리초(0~999)

            // [2] 밀리초 epoch 결과 
            std::int64_t epoch_ms = 0; // UNIX epoch (단위: 밀리초). 1970-01-01 00:00:00 UTC부터의 경과 밀리초(millisecond) 수.

            // [3] C++ time_point 결과
            std::chrono::system_clock::time_point timepoint{}; // 타임 포인트 (epoch, 단위는 나노초(MSVC/Clang/GCC)이지만, 시간형식이 밀리초까지 이므로 정밀도는 밀리초까지만 설정됨)

            //-----------------------------------------------------------------

            // 변환 직전 tm(보정 포함). (사용하지 않을 경우 std::nullopt)
            // 시간 형식에 날자(연월일)가 없고 시각(시분초)만 있는 경우, 보정치인 tm의 날자를 사용함. 
            std::optional<std::tm> broken{};

            // 형식 파서 전용: 등장 토큰 
            present_flags present{};
        };

        // 시간 문자열을 지정된 시간 형식으로 파싱
        // 인자:
        //  datetime: 파싱할 문자열. 형식과 정확히 일치해야 함.
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS", "YYYY/MM/DD",
        //          "YYYYMMDDhhmmss" 등
        //  tzmode: 형식에 없는 토큰을 보정할 때 사용할 기준 시각의 타임존 모드.
        //          UTC 또는 local_time
        // 반환: date_time_parse_result 구조체
         date_time_parse_result parse_strict_datetime(
            const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode = time_zone_mode::local_time);

        // 시간 문자열을 지정된 시간 형식으로 파싱하고, 형식에 없는 토큰은 base_tm의
        // 값으로 보정
        // 인자:
        //  datetime: 파싱할 문자열. 형식과 정확히 일치해야 함.
        //  format: 형식 문자열. 예: "hh:mm", "mm:ss" 등.
        //  tzmode: base_tm이 UTC인지 local_time인지 지정
        //  base_tm: format에 없는 토큰을 보정할 때 사용할 기준. format에 날자가
        //           없는 경우 연/월/일이 이 값으로 채워짐.
        // 반환: date_time_parse_result 구조체
         date_time_parse_result parse_strict_datetime_with_base(
            const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode,
            const std::tm& base_tm);

        // 선택적 base 버전: base_or_null이 nullptr이면 누락 토큰을 금지(오류),
        // 포인터면 해당 base_tm으로 누락 토큰 보정.
        // 인자:
        //  datetime: 파싱할 문자열. 형식과 정확히 일치해야 함
        //  format: 형식 문자열. 예: "hh:mm", "mm:ss" 등.
        //  tzmode: base_tm이 UTC인지 local_time인지 지정
        //  base_or_null: nullptr이면 누락 토큰 금지, 포인터면 해당 tm으로 누락 토큰 보정
        // 반환: date_time_parse_result 구조체
         date_time_parse_result parse_strict_datetime_optbase(
            const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode,
            const std::tm* base_or_null);

        // 시간 문자열을 지정된 시간 형식으로 파싱하고, epoch ms(밀리초) 반환
        // 인자:
        //  datetime: 파싱할 문자열. 형식과 정확히 일치해야 함
        //  format: 형식 문자열. 예: "YYYY-MM-DD hh:mm:ss.SSS", "YYYY/MM/DD", 등
        //  tzmode: 형식에 없는 토큰을 보정할 때 사용할 기준
        //          시각의 타임존 모드. UTC 또는 local_time
        // 반환: 성공 시 epoch_ms, 실패 시 std::nullopt
         std::optional<std::int64_t> parse_strict_datetime_epoch_ms(
            const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode);

        // ISO-8601 시간 문자열 파싱
        //
        // 예: "2025-10-17T00:30:05.123Z"
        //     "2025-10-17 09:30:05+09:00"
        //     "20251017T093005.5+0900"
        //     "12:34:56.789Z"   (날짜 없음 → UTC 오늘)
        //     "23:15"           (날짜 없음/오프셋 없음 → tzmode 기준 오늘)
        // 인자:
        //  iso8601: 파싱할 ISO-8601 형식 문자열
        //  fallback_tzmode: 문자열에 타임존 정보(Z/오프셋)가
        //                   없을 때 사용할 기준 시각의 타임존 모드.
        //                   UTC 또는 local_time
        // 반환: date_time_parse_result 구조체
         date_time_parse_result parse_iso8601_datetime(
            const std::string& iso8601,
            time_zone_mode fallback_tzmode = time_zone_mode::local_time);

        // 시간 문자열 자동 판별 파싱
        // 인자:
        //  text: 파싱할 문자열
        //  format_or_literal: "ISO8601" 이면 ISO-8601 파싱 시도,
        //                     아니면 형식 파서로 처리
        //  tzmode: 형식에 없는 토큰을 보정할 때 사용할 기준 시각의 타임존 모드.
        //          UTC 또는 local_time
        // 반환: date_time_parse_result 구조체
         date_time_parse_result parse_datetime_auto(
            const std::string& text,
            const std::string& format_or_literal,
            time_zone_mode tzmode = time_zone_mode::local_time);

        // 시간 문자열을 지정된 형식으로 파싱하여 time_point 반환
        // 인자:
        //  text: 파싱할 문자열
        //  format_or_literal: "ISO8601" 이면 ISO-8601 파
        //                     싱 시도, 아니면 형식 파서로 처리
        //  tzmode: 형식에 없는 토큰을 보정할 때 사용할 기준
        //          시각의 타임존 모드. UTC 또는 local_time
        // 반환: 성공 시 time_point, 실패 시 std::nullopt
         std::optional<std::chrono::system_clock::time_point> parse_datetime_timepoint(
            const std::string& text,
            const std::string& format_or_literal,
            time_zone_mode tzmode = time_zone_mode::local_time);

        // ---------------------------------------------------------------------
        // 현재 시간 문자열 생성 유틸리티 (선언)
        //
        // 기본 형식: "YYYY-MM-DD hh:mm:ss"
        // 지원 토큰: YYYY, MM, DD, hh, mm, ss, SSS
         std::string current_time_string(
            time_zone_mode tzmode = time_zone_mode::local_time,
            const std::string& format = "YYYY-MM-DD hh:mm:ss.SSS");

    }  // namespace util
}  // namespace mino::core::datetime
