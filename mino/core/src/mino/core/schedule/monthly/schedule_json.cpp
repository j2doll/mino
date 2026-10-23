#include "mino/core/json/json.hpp"
#include "mino/core/schedule/monthly/schedule_json.hpp"

namespace mino::core::schedule::monthly {

    static const char* weekday_to_str(weekday wd) {
        static const char* names[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        return names[static_cast<int>(wd)];
    }

    static std::optional<weekday> weekday_from_str(const std::string& s) {
        static const char* names[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        for (int i = 0; i < 7; ++i) {
            if (s == names[i]) return static_cast<weekday>(i);
        }
        return std::nullopt;
    }

    static const char* policy_to_str(holiday_policy p) {
        switch (p) {
        case holiday_policy::previous_business_day: return "previous_business_day";
        case holiday_policy::next_business_day:     return "next_business_day";
        case holiday_policy::skip:                  return "skip";
        case holiday_policy::exact: default:        return "exact";
        }
    }

    static holiday_policy policy_from_str(const std::string& s) {
        if (s == "previous_business_day") return holiday_policy::previous_business_day;
        if (s == "next_business_day")     return holiday_policy::next_business_day;
        if (s == "skip")                  return holiday_policy::skip;
        return holiday_policy::exact;
    }

    static std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> parse_json_value(const mino::core::json::value& root) {
        using namespace mino::core::json;

        if (!root.is_object()) return std::nullopt;
        const auto& root_obj = root.get_object();

        holiday_calendar cal(true, true);
        auto it_cal = root_obj.find("calendar");
        if (it_cal != root_obj.end() && it_cal->second.is_object()) {
            const auto& cal_obj = it_cal->second.get_object();
            bool ex_sat = true, ex_sun = true;
            if (cal_obj.count("exclude_saturday") && cal_obj.at("exclude_saturday").is_bool())
                ex_sat = cal_obj.at("exclude_saturday").get_bool();
            if (cal_obj.count("exclude_sunday") && cal_obj.at("exclude_sunday").is_bool())
                ex_sun = cal_obj.at("exclude_sunday").get_bool();
            cal.set_weekend_exclusions(ex_sat, ex_sun);

            // 단발성 공휴일 파싱
            if (cal_obj.count("holidays") && cal_obj.at("holidays").is_array()) {
                for (const auto& hv : cal_obj.at("holidays").get_array()) {
                    if (hv.is_object()) {
                        const auto& ho = hv.get_object();
                        date_ymd d{};
                        if (ho.count("year") && ho.at("year").is_number()) d.year = static_cast<int>(ho.at("year").get_number());
                        if (ho.count("month") && ho.at("month").is_number()) d.month = static_cast<int>(ho.at("month").get_number());
                        if (ho.count("day") && ho.at("day").is_number()) d.day = static_cast<int>(ho.at("day").get_number());
                        cal.add_holiday(d);
                    }
                }
            }

            // 매년 반복 공휴일 파싱
            if (cal_obj.count("recurring_holidays") && cal_obj.at("recurring_holidays").is_array()) {
                for (const auto& rhv : cal_obj.at("recurring_holidays").get_array()) {
                    if (rhv.is_object()) {
                        const auto& rho = rhv.get_object();
                        date_md md{};
                        if (rho.count("month") && rho.at("month").is_number()) md.month = static_cast<int>(rho.at("month").get_number());
                        if (rho.count("day") && rho.at("day").is_number()) md.day = static_cast<int>(rho.at("day").get_number());
                        cal.add_recurring_holiday(md);
                    }
                }
            }
        }

        std::vector<monthly_rule> rules;
        auto it_rules = root_obj.find("rules");
        if (it_rules == root_obj.end() || !it_rules->second.is_array()) {
            return std::nullopt;
        }

        const auto& rules_arr = it_rules->second.get_array();
        rules.reserve(rules_arr.size());

        for (const auto& item : rules_arr) {
            if (!item.is_object()) continue;
            const auto& obj = item.get_object();

            monthly_rule rule;
            if (obj.count("id") && obj.at("id").is_string()) rule.id = obj.at("id").get_string();
            if (obj.count("pattern") && obj.at("pattern").is_string()) {
                rule.pattern = (obj.at("pattern").get_string() == "ordinal_weekday")
                    ? pattern_type::ordinal_weekday
                    : pattern_type::day_of_month;
            }
            if (obj.count("day_of_month") && obj.at("day_of_month").is_number()) rule.day_of_month = static_cast<int>(obj.at("day_of_month").get_number());
            if (obj.count("target_weekday") && obj.at("target_weekday").is_string()) {
                auto wd_opt = weekday_from_str(obj.at("target_weekday").get_string());
                if (wd_opt) rule.target_weekday = *wd_opt;
            }
            if (obj.count("target_ordinal") && obj.at("target_ordinal").is_number()) rule.target_ordinal = static_cast<week_ordinal>(static_cast<int>(obj.at("target_ordinal").get_number()));
            if (obj.count("interval_months") && obj.at("interval_months").is_number()) rule.interval_months = static_cast<int>(obj.at("interval_months").get_number());
            if (obj.count("anchor_year") && obj.at("anchor_year").is_number()) rule.anchor_year = static_cast<int>(obj.at("anchor_year").get_number());
            if (obj.count("anchor_month") && obj.at("anchor_month").is_number()) rule.anchor_month = static_cast<int>(obj.at("anchor_month").get_number());

            if (obj.count("target_months") && obj.at("target_months").is_array()) {
                for (const auto& mv : obj.at("target_months").get_array()) {
                    if (mv.is_number()) rule.target_months.push_back(static_cast<int>(mv.get_number()));
                }
            }

            if (obj.count("execution_time") && obj.at("execution_time").is_object()) {
                const auto& to = obj.at("execution_time").get_object();
                if (to.count("hour") && to.at("hour").is_number()) rule.execution_time.hour = static_cast<int>(to.at("hour").get_number());
                if (to.count("minute") && to.at("minute").is_number()) rule.execution_time.minute = static_cast<int>(to.at("minute").get_number());
                if (to.count("second") && to.at("second").is_number()) rule.execution_time.second = static_cast<int>(to.at("second").get_number());
            }

            if (obj.count("policy") && obj.at("policy").is_string()) rule.policy = policy_from_str(obj.at("policy").get_string());

            if (obj.count("window") && obj.at("window").is_object()) {
                const auto& wo = obj.at("window").get_object();
                if (wo.count("enabled") && wo.at("enabled").is_bool()) rule.window.enabled = wo.at("enabled").get_bool();
                if (wo.count("start_h") && wo.at("start_h").is_number()) rule.window.start_time.hour = static_cast<int>(wo.at("start_h").get_number());
                if (wo.count("start_m") && wo.at("start_m").is_number()) rule.window.start_time.minute = static_cast<int>(wo.at("start_m").get_number());
                if (wo.count("start_s") && wo.at("start_s").is_number()) rule.window.start_time.second = static_cast<int>(wo.at("start_s").get_number());
                if (wo.count("end_h") && wo.at("end_h").is_number()) rule.window.end_time.hour = static_cast<int>(wo.at("end_h").get_number());
                if (wo.count("end_m") && wo.at("end_m").is_number()) rule.window.end_time.minute = static_cast<int>(wo.at("end_m").get_number());
                if (wo.count("end_s") && wo.at("end_s").is_number()) rule.window.end_time.second = static_cast<int>(wo.at("end_s").get_number());
            }

            if (obj.count("excluded_dates") && obj.at("excluded_dates").is_array()) {
                for (const auto& ev : obj.at("excluded_dates").get_array()) {
                    if (ev.is_object()) {
                        const auto& eo = ev.get_object();
                        date_ymd d{};
                        if (eo.count("year") && eo.at("year").is_number()) d.year = static_cast<int>(eo.at("year").get_number());
                        if (eo.count("month") && eo.at("month").is_number()) d.month = static_cast<int>(eo.at("month").get_number());
                        if (eo.count("day") && eo.at("day").is_number()) d.day = static_cast<int>(eo.at("day").get_number());
                        rule.excluded_dates.push_back(d);
                    }
                }
            }

            if (obj.count("overrides") && obj.at("overrides").is_array()) {
                for (const auto& ov_item : obj.at("overrides").get_array()) {
                    if (ov_item.is_object()) {
                        const auto& oo = ov_item.get_object();
                        one_time_override ov{};
                        if (oo.count("t_year")) ov.target_date.year = static_cast<int>(oo.at("t_year").get_number());
                        if (oo.count("t_month")) ov.target_date.month = static_cast<int>(oo.at("t_month").get_number());
                        if (oo.count("t_day")) ov.target_date.day = static_cast<int>(oo.at("t_day").get_number());
                        if (oo.count("r_year")) ov.rescheduled_date.year = static_cast<int>(oo.at("r_year").get_number());
                        if (oo.count("r_month")) ov.rescheduled_date.month = static_cast<int>(oo.at("r_month").get_number());
                        if (oo.count("r_day")) ov.rescheduled_date.day = static_cast<int>(oo.at("r_day").get_number());
                        if (oo.count("r_hour")) ov.rescheduled_time.hour = static_cast<int>(oo.at("r_hour").get_number());
                        if (oo.count("r_minute")) ov.rescheduled_time.minute = static_cast<int>(oo.at("r_minute").get_number());
                        if (oo.count("r_second")) ov.rescheduled_time.second = static_cast<int>(oo.at("r_second").get_number());
                        rule.overrides.push_back(ov);
                    }
                }
            }

            rules.push_back(std::move(rule));
        }

        return std::make_pair(std::move(cal), std::move(rules));
    }

    std::string to_json_string(const holiday_calendar& cal, const std::vector<monthly_rule>& rules, int indent) {
        using namespace mino::core::json;

        object_t root;

        object_t cal_obj;
        cal_obj["exclude_saturday"] = value(cal.is_exclude_sat());
        cal_obj["exclude_sunday"] = value(cal.is_exclude_sun());

        array_t h_arr;
        for (const auto& h : cal.holidays()) {
            object_t ho;
            ho["year"] = value(h.year);
            ho["month"] = value(h.month);
            ho["day"] = value(h.day);
            h_arr.push_back(value(std::move(ho)));
        }
        cal_obj["holidays"] = value(std::move(h_arr));

        array_t rh_arr;
        for (const auto& rh : cal.recurring_holidays()) {
            object_t rho;
            rho["month"] = value(rh.month);
            rho["day"] = value(rh.day);
            rh_arr.push_back(value(std::move(rho)));
        }
        cal_obj["recurring_holidays"] = value(std::move(rh_arr));

        root["calendar"] = value(std::move(cal_obj));

        array_t arr;
        arr.reserve(rules.size());

        for (const auto& r : rules) {
            object_t obj;
            obj["id"] = value(r.id);
            obj["pattern"] = value(r.pattern == pattern_type::day_of_month ? "day_of_month" : "ordinal_weekday");
            obj["day_of_month"] = value(r.day_of_month);
            obj["target_weekday"] = value(std::string(weekday_to_str(r.target_weekday)));
            obj["target_ordinal"] = value(static_cast<int>(r.target_ordinal));
            obj["interval_months"] = value(r.interval_months);
            obj["anchor_year"] = value(r.anchor_year);
            obj["anchor_month"] = value(r.anchor_month);

            array_t m_arr;
            for (int m : r.target_months) m_arr.push_back(value(m));
            obj["target_months"] = value(std::move(m_arr));

            object_t t_obj;
            t_obj["hour"] = value(r.execution_time.hour);
            t_obj["minute"] = value(r.execution_time.minute);
            t_obj["second"] = value(r.execution_time.second);
            obj["execution_time"] = value(std::move(t_obj));

            obj["policy"] = value(std::string(policy_to_str(r.policy)));

            object_t win_obj;
            win_obj["enabled"] = value(r.window.enabled);
            win_obj["start_h"] = value(r.window.start_time.hour);
            win_obj["start_m"] = value(r.window.start_time.minute);
            win_obj["start_s"] = value(r.window.start_time.second);
            win_obj["end_h"] = value(r.window.end_time.hour);
            win_obj["end_m"] = value(r.window.end_time.minute);
            win_obj["end_s"] = value(r.window.end_time.second);
            obj["window"] = value(std::move(win_obj));

            array_t ex_arr;
            for (const auto& ex : r.excluded_dates) {
                object_t d_obj;
                d_obj["year"] = value(ex.year);
                d_obj["month"] = value(ex.month);
                d_obj["day"] = value(ex.day);
                ex_arr.push_back(value(std::move(d_obj)));
            }
            obj["excluded_dates"] = value(std::move(ex_arr));

            array_t ov_arr;
            for (const auto& ov : r.overrides) {
                object_t o_obj;
                o_obj["t_year"] = value(ov.target_date.year);
                o_obj["t_month"] = value(ov.target_date.month);
                o_obj["t_day"] = value(ov.target_date.day);
                o_obj["r_year"] = value(ov.rescheduled_date.year);
                o_obj["r_month"] = value(ov.rescheduled_date.month);
                o_obj["r_day"] = value(ov.rescheduled_date.day);
                o_obj["r_hour"] = value(ov.rescheduled_time.hour);
                o_obj["r_minute"] = value(ov.rescheduled_time.minute);
                o_obj["r_second"] = value(ov.rescheduled_time.second);
                ov_arr.push_back(value(std::move(o_obj)));
            }
            obj["overrides"] = value(std::move(ov_arr));

            arr.push_back(value(std::move(obj)));
        }

        root["rules"] = value(std::move(arr));

        return serializer::serialize(value(std::move(root)), indent);
    }

    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_json_string(const std::string& json_text) {
        using namespace mino::core::json;
        value root = parser::parse(json_text);
        return parse_json_value(root);
    }

    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_json_file(const std::string& file_path) {
        using namespace mino::core::json;
        value root = file_reader::read_file(file_path);
        return parse_json_value(root);
    }

} // namespace mino::core::schedule::monthly
