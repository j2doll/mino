#include "mino/core/datetime/util/datetime_util.hpp"
#include "mino/core/datetime/util/datetime_convert.hpp"

namespace mino::core::datetime {
    namespace util {

        // ---------------- 내부 유틸 ----------------

        // 숫자 n자리 읽기
        bool read_ndigits(const std::string& s, size_t& pos, int ndig, int& out) {
            if (pos + static_cast<size_t>(ndig) > s.size()) return false;
            int v = 0;
            for (int i = 0; i < ndig; ++i) {
                char c = s[pos + i];
                if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                v = v * 10 + (c - '0');
            }
            out = v;
            pos += static_cast<size_t>(ndig);
            return true;
        }

        // 형식 토큰 일치 검사
        bool match_token(const std::string& fmt, size_t pos, const char* tok) {
            for (int i = 0; tok[i]; ++i) {
                if (pos + static_cast<size_t>(i) >= fmt.size()) return false;
                if (fmt[pos + static_cast<size_t>(i)] != tok[i]) return false;
            }
            return true;
        }

        // 윤년
        bool is_leap(int y) {
            return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
        }

        // 일자 유효성
        bool valid_ymd(int Y, int M, int D) {
            if (Y < 0 || M < 1 || M > 12) return false;
            static const int mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
            int dim = mdays[M - 1];
            if (M == 2 && is_leap(Y)) dim = 29;
            return D >= 1 && D <= dim;
        }


        // tzmode 기준 "현재" 시각/날짜
        std::tm now_in_zone(time_zone_mode tz) {
            std::tm out{};
            auto now = std::time(nullptr);
#if defined(_WIN32)
            if (tz == time_zone_mode::utc) { gmtime_s(&out, &now); }
            else { localtime_s(&out, &now); }
#else
            if (tz == time_zone_mode::utc) { out = *std::gmtime(&now); }
            else { out = *std::localtime(&now); }
#endif
            return out;
        }

        // 고정 오프셋(초) 기준의 "오늘 날짜"
        std::tm today_in_fixed_offset_seconds(int offset_sec) {
            std::tm out{};
            auto now = std::time(nullptr);
            std::time_t shifted = now + offset_sec;
#if defined(_WIN32)
            gmtime_s(&out, &shifted);
#else
            out = *std::gmtime(&shifted);
#endif
            return out;
        }


        // 내부 유틸: 0 채움 숫자 쓰기
        void append_ndigits(std::string& out, int value, int width) {
            int v = value;
            if (v < 0) v = 0;
            char buf[32];
            int pos = 31; buf[pos] = '\0';
            for (int i = 0; i < width; ++i) {
                int d = v % 10; v /= 10;
                buf[--pos] = static_cast<char>('0' + d);
            }
            out.append(&buf[pos]);
        }

        // 내부 유틸: 토큰 일치 검사
        bool fmt_match(const std::string& fmt, size_t fp, const char* tok) {
            for (int i = 0; tok[i]; ++i) {
                if (fp + static_cast<size_t>(i) >= fmt.size()) return false;
                if (fmt[fp + static_cast<size_t>(i)] != tok[i]) return false;
            }
            return true;
        }

        // 핵심 포맷터: tm → 문자열
        std::string format_from_tm_core(const std::tm& tmv,
            const std::string& format,
            int milliseconds) {
            std::string out;
            out.reserve(format.size() + 16);

            for (size_t fp = 0; fp < format.size();) {
                if (fmt_match(format, fp, "YYYY")) {
                    append_ndigits(out, tmv.tm_year + 1900, 4);
                    fp += 4;
                }
                else if (fmt_match(format, fp, "MM")) {
                    append_ndigits(out, tmv.tm_mon + 1, 2);
                    fp += 2;
                }
                else if (fmt_match(format, fp, "DD")) {
                    append_ndigits(out, tmv.tm_mday, 2);
                    fp += 2;
                }
                else if (fmt_match(format, fp, "hh")) {
                    append_ndigits(out, tmv.tm_hour, 2);
                    fp += 2;
                }
                else if (fmt_match(format, fp, "mm")) {
                    append_ndigits(out, tmv.tm_min, 2);
                    fp += 2;
                }
                else if (fmt_match(format, fp, "ss")) {
                    append_ndigits(out, tmv.tm_sec, 2);
                    fp += 2;
                }
                else if (fmt_match(format, fp, "SSS")) {
                    int ms = 0;
                    if (milliseconds >= 0) {
                        ms = milliseconds;
                        if (ms < 0) ms = 0;
                        if (ms > 999) ms = ms % 1000;
                    }
                    // milliseconds < 0 -> treat as 0 (no sub-second info)
                    append_ndigits(out, ms, 3);
                    fp += 3;
                }
                else {
                    out.push_back(format[fp]);
                    ++fp;
                }
            }
            return out;
        }


        // (1) std::tm + format
        std::string format_datetime(
            const std::tm& tmv,
            const std::string& format) {
            return format_from_tm_core(tmv, format, -1);
        }

        // (2) time_t + tzmode + format
        std::string format_datetime(
            std::time_t t,
            mino::core::datetime::util::time_zone_mode tzmode,
            const std::string& format) {
            std::tm tmv{};

            // deprecated 대신 to_tm(time_t, tzmode) 사용
            std::optional<std::tm> o = to_tm(t, tzmode);
            if (!o) return std::string{};
            tmv = *o;

            return format_from_tm_core(tmv, format, -1);
        }

        // (3) time_point + tzmode + format
        std::string format_datetime(
            const std::chrono::system_clock::time_point& tp,
            mino::core::datetime::util::time_zone_mode tzmode,
            const std::string& format) {
            std::time_t t = std::chrono::system_clock::to_time_t(tp);
            std::tm tmv{};
            std::optional<std::tm> o = to_tm(t, tzmode);
            if (!o) return std::string{};

            tmv = *o;

            // tp에서 밀리초 추출 (epoch 기준)
            auto ms_total = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
            int msec = static_cast<int>((ms_total % 1000 + 1000) % 1000); // 안전하게 양수화
            return format_from_tm_core(tmv, format, msec);
        }

        // (4) epoch_ms + tzmode + format
        std::string format_datetime(
            std::uint64_t epoch_ms,
            const std::string& format,
            mino::core::datetime::util::time_zone_mode tzmode)
        {
            auto tp = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{epoch_ms}
            };
            return format_datetime(tp, tzmode, format);
        }

        weekday get_weekday(
            const uint32_t year,
            const uint32_t month,
            const uint32_t day,
            time_zone_mode tz) {

            if (year <= 1900) {
                return weekday::no_statement; // 1900년 이전은 지원하지 않음
            }

            if (!valid_ymd(year, month, day)) {
                return weekday::no_statement;
            }

            std::tm tm{};
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1; // tm.tm_mon은 0..11
            tm.tm_mday = day; // tm.tm_mday는 1..31
            tm.tm_hour = 0;
            tm.tm_min  = 0;
            tm.tm_sec  = 0; 

            // tm → time_t (UTC 또는 Local)로 변환한 뒤, 다시 tm으로 변환해서 tm_wday를 얻음.
            // 이렇게 하면 mktime/timegm 계열의 정규화(normalization)와 요일 계산을 안전하게 수행할 수 있음.
            std::optional<std::time_t> topt;
            if (tz == time_zone_mode::utc) {
                topt = to_time_t(tm, time_zone_mode::utc);
            }
            else {
                topt = to_time_t(tm, time_zone_mode::local_time);
            }

            if (!topt.has_value()) {
                return weekday::no_statement;
            }

            std::optional<std::tm> out_opt = to_tm(*topt, tz);
            if (!out_opt.has_value()) {
                return weekday::no_statement;
            }

            std::tm out_tm = *out_opt;

            int w = out_tm.tm_wday; // 0=Sunday, 1=Monday, ..., 6=Saturday
            if (w < 0 || w > 6) {
                return weekday::no_statement;
            }

            return static_cast<weekday>(w);
        }

    } // namespace util
} // namespace mino::core::datetime 
