#include <algorithm>
#include <vector>
#include <cctype>
#include <chrono>
#include <ctime>

#include "mino/core/datetime/util/datetime_convert.hpp"

#include "mino/core/schedule/weekly/scheduler.hpp"
#include "mino/core/schedule/weekly/schedule_json.hpp"

namespace mino::core::schedule::weekly {

    // 편의: weekday enum -> std::tm 생성 (이번 주 기준, 로컬 날짜로 정규화)
    std::tm make_tm_with_wday_hour_min(
        weekday wd,
        int hour,
        int minute
    ) {
        int wd_int = static_cast<int>(wd); // mon=0..sun=6

        // 현재 로컬 날짜를 얻어 이번 주 기준으로 목표 요일로 보정
        std::time_t now = std::time(nullptr);
        std::tm cur{};
#if defined(_WIN32)
        localtime_s(&cur, &now);
#else
        localtime_r(&now, &cur);
#endif

        // cur.tm_wday: Sun=0..Sat=6 -> convert to mon=0..sun=6
        int cur_wd = (cur.tm_wday + 6) % 7;
        int delta = wd_int - cur_wd;

        std::tm t = cur; // copy year/month/day
        t.tm_mday = cur.tm_mday + delta;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = 0;
        t.tm_isdst = -1;

        // 정규화: tm -> time_t -> tm (로컬) 해서 tm_yday/tm_wday 등 보정
        namespace dtv = mino::core::datetime::util;
        auto t_opt = dtv::to_time_t(t, dtv::time_zone_mode::local_time);
        if (t_opt) {
            auto tm_opt = dtv::to_tm(*t_opt, dtv::time_zone_mode::local_time);
            if (tm_opt) {
                return *tm_opt;
            }
        }
        // 실패하면 원본 tm 반환
        return t;
    }

    scheduler::scheduler(time_base base)
        : time_base_(base),
          ranges_(),
          now_provider_() {
    }

    // time_point 기반 프로바이더 설정
    void scheduler::set_now_provider(std::function<std::chrono::system_clock::time_point()> now_provider) {
        now_provider_ = std::move(now_provider);
    }

    void scheduler::set_now(weekday wd, int hour, int minute) {
        // 편의: weekday+hour/min -> time_point 프로바이더로 저장
        now_provider_ = [wd, hour, minute]() -> std::chrono::system_clock::time_point {
            namespace dtv = mino::core::datetime::util;
            std::tm tmv = make_tm_with_wday_hour_min(wd, hour, minute);
            return dtv::to_timepoint(tmv, dtv::time_zone_mode::local_time);
        };
    }

    // time_point 기반 set_now: 즉시 주입된 time_point을 반환하는 고정 프로바이더로 저장
    void scheduler::set_now(std::chrono::system_clock::time_point tp) {
        now_provider_ = [tp]() -> std::chrono::system_clock::time_point { return tp; };
    }

    // JSON 문자열에서 읽어 스케줄에 추가합니다 (to_json_string으로 만든 형식을 읽음)
    /*
    void scheduler::load_from_json_string(const std::string& json_text) {
        auto parsed = from_json_string(json_text); // schedule_json::from_json_string
        for (const auto& r : parsed) {
            add_range(r);
        }
    }
    //*/

    // Add range and merge with existing ranges if they overlap or are adjacent (weekly wrap-aware)
    void scheduler::add_range(const weekly_range& r) {
        constexpr long long MIN_PER_DAY = 24LL * 60LL;
        constexpr long long WEEK_MINUTES = 7LL * MIN_PER_DAY;

        auto to_pair = [&](const weekly_range& x) -> std::pair<long long, long long> {
            long long s = static_cast<long long>(static_cast<int>(x.start_day)) * MIN_PER_DAY
                          + static_cast<long long>(x.start_time.hour) * 60LL
                          + static_cast<long long>(x.start_time.minute);
            long long e = static_cast<long long>(static_cast<int>(x.end_day)) * MIN_PER_DAY
                          + static_cast<long long>(x.end_time.hour) * 60LL
                          + static_cast<long long>(x.end_time.minute);
            if (e < s) e += WEEK_MINUTES;
            return {s, e};
        };

        auto new_pair = to_pair(r);
        long long new_s = new_pair.first;
        long long new_e = new_pair.second;

        std::vector<char> removed(ranges_.size(), 0);

        for (size_t i = 0; i < ranges_.size(); ++i) {
            auto p = to_pair(ranges_[i]);
            long long es0 = p.first;
            long long ee0 = p.second;

            for (int shift = -1; shift <= 1; ++shift) {
                long long shiftv = static_cast<long long>(shift) * WEEK_MINUTES;
                long long es = es0 + shiftv;
                long long ee = ee0 + shiftv;

                if (es <= new_e + 1 && ee >= new_s - 1) {
                    removed[i] = 1;
                    new_s = std::min(new_s, es);
                    new_e = std::max(new_e, ee);
                    break;
                }
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < ranges_.size(); ++i) {
                if (removed[i]) continue;
                auto p = to_pair(ranges_[i]);
                long long es0 = p.first;
                long long ee0 = p.second;

                for (int shift = -1; shift <= 1; ++shift) {
                    long long shiftv = static_cast<long long>(shift) * WEEK_MINUTES;
                    long long es = es0 + shiftv;
                    long long ee = ee0 + shiftv;

                    if (es <= new_e + 1 && ee >= new_s - 1) {
                        removed[i] = 1;
                        new_s = std::min(new_s, es);
                        new_e = std::max(new_e, ee);
                        changed = true;
                        break;
                    }
                }
            }
        }

        weekly_ranges out;
        out.reserve(ranges_.size() + 1);
        for (size_t i = 0; i < ranges_.size(); ++i) {
            if (!removed[i]) out.push_back(ranges_[i]);
        }

        long long span = new_e - new_s;
        weekly_range merged{};
        if (span + 1 >= WEEK_MINUTES) {
            merged.start_day = weekday::mon;
            merged.start_time.hour = 0;
            merged.start_time.minute = 0;
            merged.end_day = weekday::sun;
            merged.end_time.hour = 23;
            merged.end_time.minute = 59;
        } else {
            long long s_mod = ((new_s % WEEK_MINUTES) + WEEK_MINUTES) % WEEK_MINUTES;
            long long e_mod = ((new_e % WEEK_MINUTES) + WEEK_MINUTES) % WEEK_MINUTES;

            merged.start_day = static_cast<weekday>(static_cast<int>(s_mod / MIN_PER_DAY));
            {
                long long m = s_mod % MIN_PER_DAY;
                merged.start_time.hour = static_cast<int>(m / 60);
                merged.start_time.minute = static_cast<int>(m % 60);
            }

            merged.end_day = static_cast<weekday>(static_cast<int>(e_mod / MIN_PER_DAY));
            {
                long long m = e_mod % MIN_PER_DAY;
                merged.end_time.hour = static_cast<int>(m / 60);
                merged.end_time.minute = static_cast<int>(m % 60);
            }
        }

        out.push_back(merged);
        ranges_.swap(out);
    }

    const weekly_ranges& scheduler::ranges() const {
        return ranges_;
    }

    bool scheduler::is_active_now() const {
        namespace dtv = mino::core::datetime::util;

        std::chrono::system_clock::time_point tp;
        if (now_provider_) tp = now_provider_();
        else tp = std::chrono::system_clock::now();

        std::tm tm{};
        if (time_base_.get_base() == time_base::utc) {
            dtv::to_tm(tp, dtv::time_zone_mode::utc, tm);
        } else {
            dtv::to_tm(tp, dtv::time_zone_mode::local_time, tm);
        }
        return is_active_at_tm(tm);
    }

    bool scheduler::is_active_now(std::function<std::chrono::system_clock::time_point()> now_provider_tp) const {
        namespace dtv = mino::core::datetime::util;
        auto tp = now_provider_tp();
        std::tm tm{};
        if (time_base_.get_base() == time_base::utc) {
            dtv::to_tm(tp, dtv::time_zone_mode::utc, tm);
        } else {
            dtv::to_tm(tp, dtv::time_zone_mode::local_time, tm);
        }
        return is_active_at_tm(tm);
    }

    // 내부 헬퍼: 특정 tm 기준으로 활성 여부 판정
    bool scheduler::is_active_at_tm(const std::tm& tm) const {
        int now_day = (tm.tm_wday + 6) % 7; // mon=0
        int now_min = tm.tm_hour * 60 + tm.tm_min;

        for (const auto& r : ranges_) {
            int sd = static_cast<int>(r.start_day);
            int ed = static_cast<int>(r.end_day);

            int sm = r.start_time.hour * 60 + r.start_time.minute;
            int em = r.end_time.hour * 60 + r.end_time.minute;

            bool day_match =
                (sd <= ed && now_day >= sd && now_day <= ed) ||
                (sd > ed && (now_day >= sd || now_day <= ed));

            if (!day_match) continue;

            if (sd == ed) {
                if (now_min >= sm && now_min <= em) return true;
            }
            else if (now_day == sd) {
                if (now_min >= sm) return true;
            }
            else if (now_day == ed) {
                if (now_min <= em) return true;
            }
            else {
                return true;
            }
        }
        return false;
    }

}  
