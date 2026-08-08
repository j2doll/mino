#include <cmath>
#include <cerrno>

#include "mino/core/datetime/unit/date_time.hpp"

namespace mino::core::datetime {
    namespace unit {

    ////////////////////////////////////////

    #if defined(_WIN32) || defined(_WIN64)
    #   include <windows.h>
    #   define LOCALTIME_S(tm_ptr, time_t_ptr) localtime_s(tm_ptr, time_t_ptr)
    #   define GMTIME_S(tm_ptr, time_t_ptr) gmtime_s(tm_ptr, time_t_ptr)
    #   define MKGMTIME(tm_ptr) _mkgmtime(tm_ptr)
    #else
    #   define LOCALTIME_S(tm_ptr, time_t_ptr) localtime_r(time_t_ptr, tm_ptr)
    #   define GMTIME_S(tm_ptr, time_t_ptr) gmtime_r(time_t_ptr, tm_ptr)
    #   define MKGMTIME(tm_ptr) timegm(tm_ptr)
    #endif

    ////////////////////////////////////////

    // time_span
    time_span::time_span(std::chrono::milliseconds ms)
        : total_ms_(ms) {
    }

    time_span time_span::from_days(long long d) {
        return time_span(std::chrono::hours(d * 24));
    }

    time_span time_span::from_hours(long long h) {
        return time_span(std::chrono::hours(h));
    }

    time_span time_span::from_minutes(long long m) {
        return time_span(std::chrono::minutes(m));
    }

    time_span time_span::from_seconds(long long s) {
        return time_span(std::chrono::seconds(s));
    }

    time_span time_span::from_msecs(long long ms) {
        return time_span(std::chrono::milliseconds(ms));
    }

    long long time_span::total_msecs() const {
        return total_ms_.count();
    }

    long long time_span::total_secs() const {
        return total_ms_.count() / 1000LL;
    }

    std::chrono::milliseconds time_span::duration() const {
        return total_ms_;
    }

    bool time_span::operator==(const time_span& other) const {
        return total_ms_ == other.total_ms_;
    }
    
    //////////////////////////////////////////
    // date_time
    date_time::date_time(time_zone zone)
        : tp_(std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now())),
        zone_(zone) {
    }

    date_time::date_time(time_point tp, time_zone zone)
        : tp_(tp),
        zone_(zone) {
    }

    date_time date_time::current_date_time() {
        return date_time();
    }

    date_time date_time::from_time_point(time_point tp, time_zone zone) {
        return date_time(tp, zone);
    }

    date_time date_time::from_time_t(std::time_t t, time_zone zone) {
        auto ret = date_time(std::chrono::time_point_cast<std::chrono::milliseconds>(clock::from_time_t(t)), zone);
        return ret; 
    }

    // from_tm: 실패 시 std::nullopt 반환하도록 구현
    std::optional<date_time> date_time::from_tm(std::tm* tm_ptr, time_zone zone) {
        if (tm_ptr == nullptr) return std::nullopt;

        errno = 0;
        std::time_t t = (zone == time_zone::utc) ? MKGMTIME(tm_ptr) : std::mktime(tm_ptr);

        if (t == static_cast<std::time_t>(-1)) {
            // 변환 실패
            return std::nullopt;
        }

        // 정상 변환
        auto dt = from_time_t(t, zone);
        return std::optional<date_time>(dt);
    }

    // 연/월/일/시/분/초/밀리초 구성요소로부터 생성
    std::optional<date_time> date_time::from_datetime(int year, int month, int day, int hour, int min, int sec, int msec, time_zone spec) {
        std::tm parts{};
        parts.tm_year = year - 1900;
        parts.tm_mon = month - 1;
        parts.tm_mday = day;
        parts.tm_hour = hour;
        parts.tm_min = min;
        parts.tm_sec = sec;

        errno = 0;
        std::time_t t = (spec == time_zone::utc) ? MKGMTIME(&parts) : std::mktime(&parts);

        if (t == static_cast<std::time_t>(-1)) {
            // 변환 실패 -> nullopt 반환
            return std::nullopt;
        }

        auto tp_base = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::from_time_t(t));
        auto tp = tp_base + std::chrono::milliseconds(msec);
        return std::optional<date_time>(date_time(tp, spec));
    }

    // epoch 밀리초(64-bit)에서 생성 (범위 검사 후 실패 시 std::nullopt 반환)
    std::optional<date_time> date_time::from_epoch_msecs(long long epoch_msecs, time_zone spec) {
        using ms = std::chrono::milliseconds;

        ms msecs(epoch_msecs);

        // time_point 최소/최대의 time_since_epoch()를 안전하게 얻기 위해
        // Windows의 min/max 매크로 충돌을 피하도록 일시적으로 undef 처리
    #if defined(_WIN32) || defined(_WIN64)
    #   pragma push_macro("min")
    #   pragma push_macro("max")
    #   ifdef min
    #       undef min
    #   endif
    #   ifdef max
    #       undef max
    #   endif
    #endif

        auto tp_min_dur = time_point::min().time_since_epoch();
        auto tp_max_dur = time_point::max().time_since_epoch();

    #if defined(_WIN32) || defined(_WIN64)
    #   pragma pop_macro("max")
    #   pragma pop_macro("min")
    #endif

        // 비교를 위해 동일한 duration 타입으로 변환
        using dur_t = decltype(tp_min_dur);
        dur_t msecs_dur = std::chrono::duration_cast<dur_t>(msecs);

        if (msecs_dur < tp_min_dur || msecs_dur > tp_max_dur) {
            return std::nullopt;
        }

        time_point tp = time_point(msecs);
        return std::optional<date_time>(date_time(tp, spec));
    }

    date_time::time_point date_time::to_time_point() const {
        return tp_;
    }

    std::time_t date_time::to_time_t() const {
        auto ret = clock::to_time_t(tp_);
        return ret; 
    }

    // epoch 밀리초(64-bit) 반환
    long long date_time::to_epoch_msecs() const {
        auto ret = std::chrono::duration_cast<std::chrono::milliseconds>(tp_.time_since_epoch()).count();
        return ret; 
    }

    std::tm date_time::to_tm() const {
        std::time_t t_c = to_time_t();
        std::tm parts;
        if (zone_ == time_zone::utc) {
            GMTIME_S(&parts, &t_c);
        }
        else {
            LOCALTIME_S(&parts, &t_c);
        }
        return parts;
    }

    date_time date_time::to_utc() const {
        auto ret = date_time(tp_, time_zone::utc);
        return ret; 
    }

    date_time date_time::to_local_time() const {
        auto ret = date_time(tp_, time_zone::local_time);
        return ret; 
    }

    long long date_time::utc_offset_seconds() {
    #if defined(_WIN32) || defined(_WIN64)
        TIME_ZONE_INFORMATION tzi;
        DWORD res = GetTimeZoneInformation(&tzi);
        // Bias is minutes to add to local time to get UTC. Final bias includes daylight/standard bias if active.
        LONG bias = tzi.Bias;
        if (res == TIME_ZONE_ID_DAYLIGHT) bias += tzi.DaylightBias;
        else if (res == TIME_ZONE_ID_STANDARD) bias += tzi.StandardBias;
        // Offset (seconds) = -bias minutes * 60
        return static_cast<long long>(-bias) * 60LL;
    #else
        std::time_t now = std::time(nullptr);
        std::tm l_tm;
        LOCALTIME_S(&l_tm, &now);

        std::time_t local_epoch = std::mktime(&l_tm);
        std::time_t gm_of_local = MKGMTIME(&l_tm);

        return static_cast<long long>(gm_of_local - local_epoch);
    #endif
    }

    long long date_time::utc_offset_minutes() {
        auto ret = utc_offset_seconds() / 60LL;
        return ret; 
    }

    int date_time::utc_offset_hours() {
        auto ret = static_cast<int>(utc_offset_minutes() / 60LL);
        return ret; 
    }

    std::string date_time::utc_offset_string(bool short_format) {
        long long total_secs = utc_offset_seconds();
        int hours = static_cast<int>(total_secs / 3600);
        char buf[10];
        if (short_format) {
            int mins = std::abs(static_cast<int>((total_secs % 3600) / 60));
            std::snprintf(buf, sizeof(buf), "%+03d:%02d", hours, mins);
        }
        else {
            std::snprintf(buf, sizeof(buf), "%+d", hours);
        }
        auto ret = std::string(buf);
        return ret; 
    }

    int date_time::day_of_week() const { return to_tm().tm_wday; }
    std::string date_time::day_name(bool short_name) const {
        std::tm parts = to_tm();
        char buf[32];
        std::strftime(buf, sizeof(buf), short_name ? "%a" : "%A", &parts);
        auto ret = std::string(buf);
        return ret; 
    }

    time_zone date_time::spec() const {
        return zone_;
    }

    bool date_time::is_utc() const {
        return zone_ == time_zone::utc;
    }

    int date_time::year() const {
        std::tm parts = to_tm();
        auto ret = parts.tm_year + 1900;
        return ret; 
    }

    int date_time::month() const {
        std::tm parts = to_tm();
        auto ret = parts.tm_mon + 1;
        return ret; 
    }

    int date_time::day() const {
        std::tm parts = to_tm();
        auto ret = parts.tm_mday;
        return ret; 
    }

    int date_time::hour() const {
        std::tm parts = to_tm();
        auto ret = parts.tm_hour;
        return ret; 
    }

    int date_time::minute() const {
        std::tm parts = to_tm();
        return parts.tm_min;
    }

    int date_time::second() const {
        std::tm parts = to_tm();
        return parts.tm_sec;
    }

    int date_time::msec() const {
        auto rem = tp_.time_since_epoch() % std::chrono::seconds(1);
        auto ret = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(rem).count());
        return ret;
    }

    std::string date_time::to_string(const std::string& format) const {
        std::tm parts = to_tm();
        char buf[128];
        std::strftime(buf, sizeof(buf), format.c_str(), &parts);
        return std::string(buf);
    }

    date_time date_time::add_msecs(long long ms) const {
        return date_time(tp_ + std::chrono::milliseconds(ms), zone_);
    }

    date_time date_time::add_secs(long long s) const {
        return date_time(tp_ + std::chrono::seconds(s), zone_);
    }

    date_time date_time::add_minutes(long long m) const {
        return date_time(tp_ + std::chrono::minutes(m), zone_);
    }

    date_time date_time::add_hours(long long h) const {
        return date_time(tp_ + std::chrono::hours(h), zone_);
    }

    date_time date_time::add_days(long long d) const {
        return date_time(tp_ + std::chrono::hours(d * 24), zone_);
    }

    date_time date_time::add_months(int months) const {
        return adjust_calendar([months](std::tm& t) { t.tm_mon += months; });
    }

    date_time date_time::add_years(int years) const {
        return adjust_calendar([years](std::tm& t) { t.tm_year += years; });
    }

    time_span date_time::secs_to(const date_time& other) const {
        return time_span(std::chrono::duration_cast<std::chrono::milliseconds>(other.tp_ - this->tp_));
    }

    time_span date_time::operator-(const date_time& other) const {
        return time_span(std::chrono::duration_cast<std::chrono::milliseconds>(tp_ - other.tp_));
    }

    date_time date_time::operator+(const time_span& span) const {
        return date_time(tp_ + span.duration(), zone_);
    }

    date_time date_time::operator-(const time_span& span) const {
        return date_time(tp_ - span.duration(), zone_);
    }

    bool date_time::operator==(const date_time& other) const {
        return tp_ == other.tp_ && zone_ == other.zone_;
    }
     
    } // namespace unit
} // namespace mino::core::datetime
