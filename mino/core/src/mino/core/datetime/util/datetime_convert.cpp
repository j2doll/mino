#include "mino/core/datetime/util/datetime_convert.hpp"
#include "mino/core/datetime/util/datetime_util.hpp"

namespace mino::core::datetime {
    namespace util {

        // ---------------- 기존 내부 구현 재사용 / 새 API 구현 ----------------

        // 새 통일된 API: std::tm + tz -> std::time_t (optional)
        std::optional<std::time_t> to_time_t(
            const std::tm& tmv,
            time_zone_mode tzmode)
        {
            if (tzmode == time_zone_mode::utc) {
#if defined(_WIN32)
                std::tm tmp = tmv;
                std::time_t t = _mkgmtime(&tmp);
                if (t == static_cast<std::time_t>(-1)) return std::nullopt;
                return t;
#else
#if defined(__USE_MISC) || defined(_GNU_SOURCE) || defined(__APPLE__)
                std::tm tmp = tmv;
                std::time_t t = timegm(&tmp);
                if (t == static_cast<std::time_t>(-1)) return std::nullopt;
                return t;
#else
                return std::nullopt;
#endif
#endif
            } else {
                std::tm tmp = tmv;
                std::time_t t = std::mktime(&tmp);
                if (t == static_cast<std::time_t>(-1)) return std::nullopt;
                return t;
            }
        }

        // 새 통일된 API: time_t + tz -> std::tm (optional)
        std::optional<std::tm> to_tm(
            std::time_t t,
            time_zone_mode tzmode)
        {
            std::tm out{};
            bool ok = false;
            if (tzmode == time_zone_mode::utc) {
#if defined(_WIN32)
                ok = (gmtime_s(&out, &t) == 0);
#elif defined(_POSIX_VERSION)
                ok = (gmtime_r(&t, &out) != nullptr);
#else
                std::tm* p = std::gmtime(&t);
                if (p) { out = *p; ok = true; }
#endif
            } else {
#if defined(_WIN32)
                ok = (localtime_s(&out, &t) == 0);
#elif defined(_POSIX_VERSION)
                ok = (localtime_r(&t, &out) != nullptr);
#else
                std::tm* p = std::localtime(&t);
                if (p) { out = *p; ok = true; }
#endif
            }

            if (!ok) return std::nullopt;
            return out;
        }

        // 새 통일된 API: time_point + tz -> std::tm (optional)
        std::optional<std::tm> to_tm(
            const std::chrono::system_clock::time_point& tp,
            time_zone_mode tzmode)
        {
            std::time_t t = std::chrono::system_clock::to_time_t(tp);
            return to_tm(t, tzmode);
        }

        // 새 통일된 API: std::tm + tz -> time_point (optional)
        std::optional<std::chrono::system_clock::time_point> to_timepoint_opt(
            const std::tm& tmv,
            time_zone_mode tzmode)
        {
            auto t_opt = to_time_t(tmv, tzmode);
            if (!t_opt.has_value()) return std::nullopt;
            return std::chrono::system_clock::from_time_t(*t_opt);
        }


        // std::tm + time_zone_mode → time_point (기존 동작: 실패 시 epoch 0 반환)
        std::chrono::system_clock::time_point to_timepoint(const std::tm& tmv, time_zone_mode tzmode) {
            auto o = to_timepoint_opt(tmv, tzmode);
            if (!o.has_value()) return std::chrono::system_clock::time_point{};
            return *o;
        }

        // time_point + time_zone_mode → std::tm (out-param, 기존 서명 유지)
        bool to_tm(const std::chrono::system_clock::time_point& tp, time_zone_mode tzmode, std::tm& out) {
            auto o = to_tm(tp, tzmode);
            if (!o.has_value()) return false;
            out = *o; return true;
        }

        /*
        //////////////////////////////////////////////////////////
        namespace deprecated {

            // ---------------- 기존 API: deprecated 래퍼 / 기존 동작 유지 ----------------

            // tm → time_t (UTC)
            std::optional<std::time_t> tm_to_time_utc(std::tm tmv) {
                return to_time_t(tmv, time_zone_mode::utc);
            }

            // tm → time_t (Local)
            std::optional<std::time_t> tm_to_time_local(std::tm tmv) {
                return to_time_t(tmv, time_zone_mode::local_time);
            }

            // time_t → std::tm (UTC) (out-param)
            bool time_t_to_utc_tm(std::time_t t, std::tm& out) {
                auto o = to_tm(t, time_zone_mode::utc);
                if (!o.has_value()) return false;
                out = *o; return true;
            }

            // time_t → std::tm (Local) (out-param)
            bool time_t_to_local_tm(std::time_t t, std::tm& out) {
                auto o = to_tm(t, time_zone_mode::local_time);
                if (!o.has_value()) return false;
                out = *o; return true;
            }

            // time_point → std::tm (UTC)
            bool get_utc_tm(const std::chrono::system_clock::time_point& tp, std::tm& out) {
                auto o = to_tm(tp, time_zone_mode::utc);
                if (!o.has_value()) return false;
                out = *o; return true;
            }

            // time_point → std::tm (Local)
            bool get_local_tm(const std::chrono::system_clock::time_point& tp, std::tm& out) {
                auto o = to_tm(tp, time_zone_mode::local_time);
                if (!o.has_value()) return false;
                out = *o; return true;
            }

        } // namespace deprecated
        /////////////////////////////////////////////////////
        //*/

    } // namespace util
} // namespace mino::core::datetime 