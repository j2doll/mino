#include <ctime>
#include "mino/core/datetime/util/datetime_convert.hpp"
#include "mino/core/schedule/monthly/scheduler.hpp"
#include "mino/core/schedule/monthly/schedule_json.hpp"
#include "mino/core/schedule/monthly/schedule_xml.hpp"

namespace mino::core::schedule::monthly {

    scheduler::scheduler(time_base base)
        : time_base_(base), calendar_(), rules_(), now_provider_() {
    }

    void scheduler::set_now_provider(std::function<std::chrono::system_clock::time_point()> now_provider) {
        now_provider_ = std::move(now_provider);
    }

    void scheduler::set_now(const date_ymd& d, const time_hms& t) {
        now_provider_ = [d, t]() -> std::chrono::system_clock::time_point {
            namespace dtv = mino::core::datetime::util;
            std::tm tmv{};
            tmv.tm_year = d.year - 1900;
            tmv.tm_mon = d.month - 1;
            tmv.tm_mday = d.day;
            tmv.tm_hour = t.hour;
            tmv.tm_min = t.minute;
            tmv.tm_sec = t.second;
            tmv.tm_isdst = -1;
            return dtv::to_timepoint(tmv, dtv::time_zone_mode::local_time);
            };
    }

    void scheduler::set_now(std::chrono::system_clock::time_point tp) {
        now_provider_ = [tp]() -> std::chrono::system_clock::time_point { return tp; };
    }

    void scheduler::add_rule(const monthly_rule& rule) {
        rules_.push_back(rule);
    }

    date_ymd scheduler::calculate_pattern_date(const monthly_rule& rule, int year, int month) const {
        int max_day = days_in_month(year, month);

        if (rule.pattern == pattern_type::day_of_month) {
            int d = std::min(rule.day_of_month, max_day);
            return date_ymd{ year, month, d };
        }

        int ord = static_cast<int>(rule.target_ordinal);
        if (rule.target_ordinal == week_ordinal::last) {
            for (int d = max_day; d >= 1; --d) {
                if (get_weekday(date_ymd{ year, month, d }) == rule.target_weekday) {
                    return date_ymd{ year, month, d };
                }
            }
        }
        else {
            int count = 0;
            for (int d = 1; d <= max_day; ++d) {
                if (get_weekday(date_ymd{ year, month, d }) == rule.target_weekday) {
                    count++;
                    if (count == ord) {
                        return date_ymd{ year, month, d };
                    }
                }
            }
        }

        return date_ymd{ year, month, 1 };
    }

    std::optional<std::pair<date_ymd, time_hms>> scheduler::get_execution_date_time(
        const monthly_rule& rule, int year, int month) const {

        if (!rule.is_month_applicable(year, month)) {
            return std::nullopt;
        }

        date_ymd base_date = calculate_pattern_date(rule, year, month);

        // 1. 일회성 일정 변경 확인
        for (const auto& ov : rule.overrides) {
            if (ov.target_date == base_date) {
                for (const auto& ex : rule.excluded_dates) {
                    if (ex == ov.rescheduled_date) return std::nullopt;
                }
                if (!rule.window.contains(ov.rescheduled_time)) return std::nullopt;
                return std::make_pair(ov.rescheduled_date, ov.rescheduled_time);
            }
        }

        // 2. 공휴일/휴무일 정책에 따른 일자 보정
        auto adjusted_date_opt = calendar_.adjust_by_policy(base_date, rule.policy);
        if (!adjusted_date_opt.has_value()) {
            return std::nullopt;
        }
        date_ymd target_date = *adjusted_date_opt;

        // 3. 제외 날짜 확인
        for (const auto& ex : rule.excluded_dates) {
            if (ex == target_date) return std::nullopt;
        }

        // 4. 유효 시간대 필터링
        if (!rule.window.contains(rule.execution_time)) {
            return std::nullopt;
        }

        return std::make_pair(target_date, rule.execution_time);
    }

    std::tm scheduler::get_current_tm() const {
        namespace dtv = mino::core::datetime::util;
        std::chrono::system_clock::time_point tp;
        if (now_provider_) tp = now_provider_();
        else tp = std::chrono::system_clock::now();

        std::tm tm{};
        if (time_base_.get_base() == time_base::utc) {
            dtv::to_tm(tp, dtv::time_zone_mode::utc, tm);
        }
        else {
            dtv::to_tm(tp, dtv::time_zone_mode::local_time, tm);
        }
        return tm;
    }

    bool scheduler::is_active_now(bool match_seconds) const {
        std::tm now_tm = get_current_tm();
        date_ymd cur_date{ now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday };
        time_hms cur_time{ now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec };

        for (const auto& rule : rules_) {
            auto exec_opt = get_execution_date_time(rule, cur_date.year, cur_date.month);
            if (!exec_opt) continue;

            const auto& [ed, et] = *exec_opt;
            if (ed == cur_date && et.hour == cur_time.hour && et.minute == cur_time.minute) {
                if (!match_seconds || et.second == cur_time.second) {
                    return true;
                }
            }
        }
        return false;
    }

    bool scheduler::load_from_json_string(const std::string& json_text) {
        auto parsed = from_json_string(json_text);
        if (!parsed) return false;
        calendar_ = parsed->first;
        rules_ = parsed->second;
        return true;
    }

    bool scheduler::load_from_json_file(const std::string& file_path) {
        auto parsed = from_json_file(file_path);
        if (!parsed) return false;
        calendar_ = parsed->first;
        rules_ = parsed->second;
        return true;
    }

    std::string scheduler::export_to_json_string(int indent) const {
        return to_json_string(calendar_, rules_, indent);
    }

    bool scheduler::load_from_xml_string(const std::string& xml_text) {
        auto parsed = from_xml_string(xml_text);
        if (!parsed) return false;
        calendar_ = parsed->first;
        rules_ = parsed->second;
        return true;
    }

    bool scheduler::load_from_xml_file(const std::filesystem::path& file_path) {
        auto parsed = from_xml_file(file_path);
        if (!parsed) return false;
        calendar_ = parsed->first;
        rules_ = parsed->second;
        return true;
    }

    std::string scheduler::export_to_xml_string(bool pretty_print) const {
        return to_xml_string(calendar_, rules_, pretty_print);
    }

    bool scheduler::export_to_xml_file(const std::filesystem::path& file_path, bool pretty_print) const {
        return to_xml_file(file_path, calendar_, rules_, pretty_print);
    }

} // namespace mino::core::schedule::monthly
