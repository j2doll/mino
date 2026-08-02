#include "mino/core/datetime/util/datetime_string.hpp"
#include "mino/core/datetime/util/datetime_util.hpp"
#include "mino/core/datetime/util/datetime_convert.hpp"

namespace mino::core::datetime {
    namespace util {

        // ---------------- (A) 형식 파서 ----------------

        // 변경: base_tm을 선택적으로 사용하기 위해 포인터 인자로 변경
        static date_time_parse_result
            parse_strict_datetime_ex_impl(const std::string& datetime,
                const std::string& format,
                time_zone_mode tzmode,
                const std::tm* base_tm_opt)
        {
            date_time_parse_result r;
            r.present = {};

            // 초기값 설정:
            // - base_tm_opt가 있으면 그 값을 초기값으로 사용(누락 토큰 보정).
            // - 없으면 임시 tm을 0으로 초기화하되, 보정은 하지 않음(누락 금지).
            std::tm tmv{}; // 초기값 1900년 1월 0일 00:00:00 (0일 → 유효하지 않은 값 (일은 최소 1이어야 정상))
            if (base_tm_opt) {
                tmv = *base_tm_opt;
            }

            int Y = base_tm_opt ? (tmv.tm_year + 1900) : 0;
            int M = base_tm_opt ? (tmv.tm_mon + 1) : 0;
            int D = base_tm_opt ? (tmv.tm_mday) : 0;
            int h = base_tm_opt ? (tmv.tm_hour) : 0;
            int m = base_tm_opt ? (tmv.tm_min) : 0;
            int s = base_tm_opt ? (tmv.tm_sec) : 0;
            int ms = 0;

            size_t ip = 0;
            for (size_t fp = 0; fp < format.size();) {
                if (match_token(format, fp, "YYYY")) {
                    if (!read_ndigits(datetime, ip, 4, Y)) {
                        r.error = "YYYY Error";
                        return r;
                    }
                    r.present.has_year = true;
                    fp += 4;
                }
                else if (match_token(format, fp, "MM")) {
                    if (!read_ndigits(datetime, ip, 2, M)) {
                        r.error = "MM Error";
                        return r;
                    }
                    r.present.has_month = true;
                    fp += 2;
                }
                else if (match_token(format, fp, "DD")) {
                    if (!read_ndigits(datetime, ip, 2, D)) {
                        r.error = "DD Error";
                        return r;
                    }
                    r.present.has_day = true;
                    fp += 2;
                }
                else if (match_token(format, fp, "hh")) {
                    if (!read_ndigits(datetime, ip, 2, h)) {
                        r.error = "hh Error";
                        return r;
                    }
                    r.present.has_hour = true;
                    fp += 2;
                }
                else if (match_token(format, fp, "mm")) {
                    if (!read_ndigits(datetime, ip, 2, m)) {
                        r.error = "mm Error";
                        return r;
                    }
                    r.present.has_minute = true;
                    fp += 2;
                }
                else if (match_token(format, fp, "ss")) {
                    if (!read_ndigits(datetime, ip, 2, s)) {
                        r.error = "ss Error";
                        return r;
                    }
                    r.present.has_second = true;
                    fp += 2;
                }
                else if (match_token(format, fp, "SSS")) {
                    if (!read_ndigits(datetime, ip, 3, ms)) {
                        r.error = "SSS Error";
                        return r;
                    }
                    r.present.has_millisecond = true;
                    fp += 3;
                }
                else {
                    if (ip >= datetime.size()) {
                        r.error = "Literally inconsistent"; return r;
                    }
                    if (format[fp] != datetime[ip]) {
                        r.error = "Literally inconsistent"; return r;
                    }
                    ++fp;
                    ++ip;
                }
            }
            if (ip != datetime.size()) {
                r.error = "Extra character"; return r;
            }

            // base가 없는 경우: 핵심 토큰 누락 금지
            if (!base_tm_opt) {
                const bool date_ok = r.present.has_year && r.present.has_month && r.present.has_day;
                const bool time_ok = r.present.has_hour && r.present.has_minute && r.present.has_second;

                if (!date_ok && !time_ok) {
                    r.error = "Missing tokens without base (require at least YYYY,MM,DD or hh,mm,ss)";
                    return r;
                }

                // 시간만 제공된 경우, tzmode에 따라 오늘 날짜로 보정 (tm 변환 실패 방지)
                if (!date_ok && time_ok) {
                    std::tm today = now_in_zone(tzmode);
                    Y = today.tm_year + 1900;
                    M = today.tm_mon + 1;
                    D = today.tm_mday;
                }
            }

            // === 범위 검증 ===
            if (r.present.has_hour && (h < 0 || h > 23)) {
                r.error = "Hour range error"; return r;
            }
            if (r.present.has_minute && (m < 0 || m > 59)) {
                r.error = "Minute range error"; return r;
            }
            if (r.present.has_second && (s < 0 || s > 59)) {
                r.error = "Second range error"; return r;
            }

            if (r.present.has_year || r.present.has_month || r.present.has_day) {
                if (r.present.has_month && (M < 1 || M > 12)) {
                    r.error = "Month range error"; return r;
                }
                if (!valid_ymd(Y, M, D)) {
                    r.error = "Date Validation Error"; return r;
                }
            }

            // 변환용 tm 구성
            tmv.tm_year = Y - 1900;
            tmv.tm_mon = M - 1;
            tmv.tm_mday = D;
            tmv.tm_hour = h;
            tmv.tm_min = m;
            tmv.tm_sec = s;
            tmv.tm_isdst = -1;

            std::optional<std::time_t> t;
            if (tzmode == time_zone_mode::utc) {
                t = to_time_t(tmv, time_zone_mode::utc);
            }
            else {
                t = to_time_t(tmv, time_zone_mode::local_time);
            }

            if (!t.has_value()) {
                r.error = "Time conversion failure";
                return r;
            }

            // 성공 결과 설정
            r.ok = true;

            if (base_tm_opt) {
                r.broken = tmv;   // base가 있을 때만 tm 반환
            }
            else {
                r.broken = std::nullopt; // base가 없으면 없음 처리
            }

            r.millisecond = (r.present.has_millisecond ? ms : 0); // 밀리초 (0~999)

            r.epoch = *t;               // UNIX epoch (초)

            r.epoch_ms = static_cast<std::int64_t>(r.epoch) * 1000 + r.millisecond; // UNIX epoch (밀리초)

            r.timepoint = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{r.epoch_ms} }; // C++ time_point

            return r;
        }

        date_time_parse_result parse_strict_datetime(const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode) {
            return parse_strict_datetime_ex_impl(datetime, format, tzmode, nullptr);
        }

        date_time_parse_result parse_strict_datetime_with_base(const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode,
            const std::tm& base_tm) {
            return parse_strict_datetime_ex_impl(datetime, format, tzmode, &base_tm);
        }

        date_time_parse_result parse_strict_datetime_optbase(const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode,
            const std::tm* base_or_null) {
            // base_or_null == nullptr → 누락 토큰 금지
            return parse_strict_datetime_ex_impl(datetime, format, tzmode, base_or_null);
        }

        std::optional<std::int64_t> parse_strict_datetime_epoch_ms(const std::string& datetime,
            const std::string& format,
            time_zone_mode tzmode) {
            auto r = parse_strict_datetime(datetime, format, tzmode);
            if (!r.ok) return std::nullopt;
            return r.epoch_ms;
        }

        // ---------------- (B) ISO-8601 파서 ----------------

        static bool read_2d(const std::string& s, size_t& pos, int& v) {
            return read_ndigits(s, pos, 2, v);
        }

        // "Z" | (+|-)HH[:?]MM | (+|-)HH
        static bool parse_tz_offset(const std::string& s, size_t& pos, int& offset_sec) {
            if (pos >= s.size()) return false;
            if (s[pos] == 'Z' || s[pos] == 'z') { offset_sec = 0; ++pos; return true; }
            char sign = s[pos];
            if (sign != '+' && sign != '-') return false;
            ++pos;
            int HH = 0; if (!read_ndigits(s, pos, 2, HH)) return false;
            int MM = 0;
            if (pos < s.size() && s[pos] == ':') {
                ++pos; if (!read_ndigits(s, pos, 2, MM)) return false;
            }
            else {
                if (pos + 2 <= s.size() &&
                    std::isdigit((unsigned char)s[pos]) &&
                    std::isdigit((unsigned char)s[pos + 1])) {
                    if (!read_ndigits(s, pos, 2, MM)) return false;
                }
            }
            int sec = HH * 3600 + MM * 60;
            if (sign == '-') sec = -sec;
            offset_sec = sec;
            return true;
        }

        // .frac → 밀리초(앞 3자리), 부족 시 0 패딩
        static bool parse_fraction_ms(const std::string& s, size_t& pos, int& out_ms) {
            if (pos >= s.size() || s[pos] != '.') return false;
            ++pos;
            int digits = 0;
            int ms = 0;
            while (pos < s.size() && std::isdigit((unsigned char)s[pos]) && digits < 9) {
                int d = s[pos] - '0';
                if (digits < 3) ms = ms * 10 + d;
                ++pos; ++digits;
            }
            if (digits == 0) return false;
            while (digits < 3) { ms *= 10; ++digits; }
            out_ms = ms;
            return true;
        }

        static date_time_parse_result
            parse_iso8601_strict(const std::string& str, time_zone_mode fallback_tzmode)
        {
            date_time_parse_result r;
            size_t p = 0;
            int Y = -1, M = -1, D = -1, hh = -1, mm = -1, ss = -1, ms = 0;
            bool has_date = false, has_time = false, has_tz = false;
            int tz_offset_sec = 0;

            auto parse_date_extended = [&]() -> bool {
                int y, m, d; size_t op = p;
                if (!read_ndigits(str, p, 4, y)) { p = op; return false; }
                if (p >= str.size() || str[p] != '-') { p = op; return false; } ++p;
                if (!read_ndigits(str, p, 2, m)) { p = op; return false; }
                if (p >= str.size() || str[p] != '-') { p = op; return false; } ++p;
                if (!read_ndigits(str, p, 2, d)) { p = op; return false; }
                Y = y; M = m; D = d; return true;
                };
            auto parse_time_extended = [&]()->bool {
                int H, Min; size_t op = p;
                if (!read_2d(str, p, H)) { p = op; return false; }
                if (p >= str.size() || str[p] != ':') { p = op; return false; } ++p;
                if (!read_2d(str, p, Min)) { p = op; return false; }
                int S = -1; if (p < str.size() && str[p] == ':') { ++p; if (!read_2d(str, p, S)) { p = op; return false; } }
                hh = H; mm = Min; ss = S; return true;
                };

            // 날짜(+T/공백)시간, 또는 시간만
            size_t p0 = p;
            if (parse_date_extended()) {
                has_date = true;
                if (p < str.size() && (str[p] == 'T' || str[p] == 't' || str[p] == ' ')) {
                    ++p;
                    if (!parse_time_extended()) { r.error = "Time format error"; return r; }
                    has_time = true;
                }
            }
            else {
                p = p0;
                if (parse_time_extended()) has_time = true;
                else { r.error = "Not in ISO-8601 format"; return r; }
            }

            if (has_time && p < str.size() && str[p] == '.') {
                if (!parse_fraction_ms(str, p, ms)) { r.error = "Fractional ms seconds format error"; return r; }
            }

            if (p < str.size()) {
                size_t tzp = p; int off = 0;
                if (parse_tz_offset(str, tzp, off)) { has_tz = true; tz_offset_sec = off; p = tzp; }
            }

            if (p != str.size()) { r.error = "Extra input characters exist"; return r; }

            // 날짜가 없으면 오늘 날짜 보정
            if (!has_date) {
                std::tm base = has_tz ? today_in_fixed_offset_seconds(tz_offset_sec)
                    : now_in_zone(fallback_tzmode);
                Y = base.tm_year + 1900;
                M = base.tm_mon + 1;
                D = base.tm_mday;
            }
            if (ss < 0) ss = 0;

            std::tm tmv{}; tmv.tm_year = Y - 1900; tmv.tm_mon = M - 1; tmv.tm_mday = D;
            tmv.tm_hour = hh; tmv.tm_min = mm; tmv.tm_sec = ss; tmv.tm_isdst = -1;

            std::optional<std::time_t> t;
            if (has_tz) {
                auto t_as_utc = to_time_t(tmv, time_zone_mode::utc);
                if (!t_as_utc) {
                    r.error = "UTC conversion failed (timegem/_mkgmtime)";
                    return r;
                }
                t = t_as_utc.value() - tz_offset_sec; // 오프셋 적용
            }
            else {
                t = (fallback_tzmode == time_zone_mode::utc) ?
                    to_time_t(tmv, time_zone_mode::utc)
                    : to_time_t(tmv, time_zone_mode::local_time);
            }

            if (!t) {
                r.error = "Time conversion failure";
                return r;
            }

            r.ok = true;
            r.epoch = *t;
            r.millisecond = ms;
            r.epoch_ms = static_cast<std::int64_t>(r.epoch) * 1000 + r.millisecond;
            r.broken = tmv;
            r.timepoint = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{r.epoch_ms} };
            return r;
        }

        // -------- 공개 API --------

        date_time_parse_result parse_iso8601_datetime(const std::string& iso8601,
            time_zone_mode fallback_tzmode) {
            return parse_iso8601_strict(iso8601, fallback_tzmode);
        }

        date_time_parse_result parse_datetime_auto(const std::string& text,
            const std::string& format_or_literal,
            time_zone_mode tzmode) {
            if (format_or_literal == "ISO8601") {
                return parse_iso8601_datetime(text, tzmode);
            }
            return parse_strict_datetime(text, format_or_literal, tzmode);
        }


        std::optional<std::chrono::system_clock::time_point> parse_datetime_timepoint(const std::string& text,
            const std::string& format_or_literal,
            time_zone_mode tzmode)
        {
            auto r = parse_datetime_auto(text, format_or_literal, tzmode);
            if (!r.ok) return std::nullopt;
            return r.timepoint;
        }

        std::string current_time_string(time_zone_mode tzmode, const std::string& format)
        {
            using namespace std::chrono;

            const auto now = system_clock::now();
            const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            const std::time_t tt = system_clock::to_time_t(now);

            std::tm tm{};
#if defined(_MSC_VER)
            if (tzmode == time_zone_mode::utc) {
                ::gmtime_s(&tm, &tt);
            }
            else {
                ::localtime_s(&tm, &tt);
            }
#else
            if (tzmode == time_zone_mode::utc) {
                ::gmtime_r(&tt, &tm);
            }
            else {
                ::localtime_r(&tt, &tm);
            }
#endif

            auto pad2 = [](int v) -> std::string {
                char buf[3];
                std::snprintf(buf, sizeof(buf), "%02d", v);
                return std::string(buf);
                };
            auto pad3 = [](int v) -> std::string {
                char buf[4];
                std::snprintf(buf, sizeof(buf), "%03d", v);
                return std::string(buf);
                };
            auto pad4 = [](int v) -> std::string {
                char buf[5];
                std::snprintf(buf, sizeof(buf), "%04d", v);
                return std::string(buf);
                };

            std::string out;
            out.reserve(format.size() + 8);

            for (std::size_t i = 0; i < format.size();) {
                // Long tokens
                if (i + 4 <= format.size() && format.compare(i, 4, "YYYY") == 0) {
                    out += pad4(tm.tm_year + 1900);
                    i += 4;
                    continue;
                }
                if (i + 3 <= format.size() && format.compare(i, 3, "SSS") == 0) {
                    out += pad3(static_cast<int>(ms.count()));
                    i += 3;
                    continue;
                }

                // 2-char tokens
                if (i + 2 <= format.size()) {
                    const std::string token = format.substr(i, 2);
                    if (token == "MM") {
                        out += pad2(tm.tm_mon + 1);
                        i += 2;
                        continue;
                    }
                    if (token == "DD") {
                        out += pad2(tm.tm_mday);
                        i += 2;
                        continue;
                    }
                    if (token == "hh") {
                        out += pad2(tm.tm_hour);
                        i += 2;
                        continue;
                    }
                    if (token == "mm") {
                        out += pad2(tm.tm_min);
                        i += 2;
                        continue;
                    }
                    if (token == "ss") {
                        out += pad2(tm.tm_sec);
                        i += 2;
                        continue;
                    }
                }

                // Copy literal
                out.push_back(format[i]);
                ++i;
            }

            return out;
        }

    } // namespace util
} // namespace mino::core::datetime 