#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <optional>
#include <chrono>

#include "mino/core/datetime/util/datetime_common.hpp"

namespace mino::core::datetime {
    namespace util {

        // ---------------- 새 통일된 변환 API ----------------
        // 실패 가능성 있는 변환은 std::optional<T> 반환으로 통일

        // std::tm + time_zone_mode -> time_t
         std::optional<std::time_t> to_time_t(
            const std::tm& tmv,
            time_zone_mode tzmode);

        // time_t + time_zone_mode -> std::tm
         std::optional<std::tm> to_tm(
            std::time_t t,
            time_zone_mode tzmode);

        // time_point + time_zone_mode -> std::tm (optional 반환)
         std::optional<std::tm> to_tm(
            const std::chrono::system_clock::time_point& tp,
            time_zone_mode tzmode);

        // std::tm + time_zone_mode -> time_point (optional 반환)
         std::optional<std::chrono::system_clock::time_point> to_timepoint_opt(
            const std::tm& tmv,
            time_zone_mode tzmode);

        // ---------------- 기존 API (동작 유지하되 내부에서 새 API 사용) ----------------

        // std::tm + time_zone_mode → time_point
        // 기존 동작(실패 시 epoch 반환)을 유지하기 위해 래퍼로 구현
         std::chrono::system_clock::time_point to_timepoint(
            const std::tm& tmv,
            time_zone_mode tzmode);

        // time_point + time_zone_mode → std::tm (out-param, bool 반환)
        // 기존 서명 유지, 내부적으로 새 to_tm을 사용
         bool to_tm(
            const std::chrono::system_clock::time_point& tp,
            time_zone_mode tzmode,
            std::tm& out);

        /*
        //////////////////////////////////////////////////////
        namespace deprecated {

            // ---------------- 기존 API (호환성 유지, deprecated 래퍼) ----------------

            // std::tm → time_t 변환 (UTC 기준)
            [[deprecated("Use to_time_t(tm, time_zone_mode::utc) instead")]]
             std::optional<std::time_t> tm_to_time_utc(std::tm tmv);

            // std::tm → time_t 변환 (Localtime 기준)
            [[deprecated("Use to_time_t(tm, time_zone_mode::local_time) instead")]]
             std::optional<std::time_t> tm_to_time_local(std::tm tmv);

            // time_t → std::tm 변환 (UTC 기준) (기존 out-param 방식 유지)
            [[deprecated("Use to_tm(time_t, tzmode) returning std::optional<std::tm> instead")]]
             bool time_t_to_utc_tm(std::time_t t, std::tm& out);

            // time_t → std::tm 변환 (Localtime 기준)
            [[deprecated("Use to_tm(time_t, tzmode) returning std::optional<std::tm> instead")]]
             bool time_t_to_local_tm(std::time_t t, std::tm& out);
            // time_point → std::tm 변환 (UTC 기준) (기존 out-param 방식 유지)
            [[deprecated("Use to_tm(time_point, tzmode) returning std::optional<std::tm> instead")]]
             bool get_utc_tm(
                const std::chrono::system_clock::time_point& tp,
                std::tm& out);

            // time_point → std::tm 변환 (Localtime 기준)
            [[deprecated("Use to_tm(time_point, tzmode) returning std::optional<std::tm> instead")]]
             bool get_local_tm(
                const std::chrono::system_clock::time_point& tp,
                std::tm& out);

        } // namespace deprecated
        ///////////////////////////////////////////////////////
        //*/

    } // namespace util
} // namespace mino::core::datetime

