#include <memory>
#include <algorithm>
#include <string>

#include "mino/core/xml/xml.hpp"
#include "mino/core/schedule/monthly/schedule_xml.hpp"

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

    static std::unique_ptr<mino::core::xml::xml_node> build_xml_tree(
        const holiday_calendar& cal, const std::vector<monthly_rule>& rules) {

        using namespace mino::core::xml;

        auto root = std::make_unique<xml_node>();
        root->name = "MonthlySchedule";

        auto cal_node = std::make_unique<xml_node>();
        cal_node->name = "Calendar";
        cal_node->attributes.push_back({ "exclude_saturday", cal.is_exclude_sat() ? "true" : "false", "", "" });
        cal_node->attributes.push_back({ "exclude_sunday", cal.is_exclude_sun() ? "true" : "false", "", "" });

        // 단발성 공휴일
        auto holidays_node = std::make_unique<xml_node>();
        holidays_node->name = "Holidays";
        for (const auto& h : cal.holidays()) {
            auto h_node = std::make_unique<xml_node>();
            h_node->name = "Holiday";
            h_node->attributes.push_back({ "year", std::to_string(h.year), "", "" });
            h_node->attributes.push_back({ "month", std::to_string(h.month), "", "" });
            h_node->attributes.push_back({ "day", std::to_string(h.day), "", "" });
            holidays_node->children.push_back(std::move(h_node));
        }
        cal_node->children.push_back(std::move(holidays_node));

        // 매년 반복 공휴일
        auto rec_holidays_node = std::make_unique<xml_node>();
        rec_holidays_node->name = "RecurringHolidays";
        for (const auto& rh : cal.recurring_holidays()) {
            auto rh_node = std::make_unique<xml_node>();
            rh_node->name = "RecurringHoliday";
            rh_node->attributes.push_back({ "month", std::to_string(rh.month), "", "" });
            rh_node->attributes.push_back({ "day", std::to_string(rh.day), "", "" });
            rec_holidays_node->children.push_back(std::move(rh_node));
        }
        cal_node->children.push_back(std::move(rec_holidays_node));

        root->children.push_back(std::move(cal_node));

        // Rules
        auto rules_node = std::make_unique<xml_node>();
        rules_node->name = "Rules";

        for (const auto& r : rules) {
            auto r_node = std::make_unique<xml_node>();
            r_node->name = "Rule";

            r_node->attributes.push_back({ "id", r.id, "", "" });
            r_node->attributes.push_back({ "pattern", r.pattern == pattern_type::day_of_month ? "day_of_month" : "ordinal_weekday", "", "" });
            r_node->attributes.push_back({ "day_of_month", std::to_string(r.day_of_month), "", "" });
            r_node->attributes.push_back({ "target_weekday", weekday_to_str(r.target_weekday), "", "" });
            r_node->attributes.push_back({ "target_ordinal", std::to_string(static_cast<int>(r.target_ordinal)), "", "" });
            r_node->attributes.push_back({ "interval_months", std::to_string(r.interval_months), "", "" });
            r_node->attributes.push_back({ "anchor_year", std::to_string(r.anchor_year), "", "" });
            r_node->attributes.push_back({ "anchor_month", std::to_string(r.anchor_month), "", "" });
            r_node->attributes.push_back({ "policy", policy_to_str(r.policy), "", "" });

            auto time_node = std::make_unique<xml_node>();
            time_node->name = "ExecutionTime";
            time_node->attributes.push_back({ "hour", std::to_string(r.execution_time.hour), "", "" });
            time_node->attributes.push_back({ "minute", std::to_string(r.execution_time.minute), "", "" });
            time_node->attributes.push_back({ "second", std::to_string(r.execution_time.second), "", "" });
            r_node->children.push_back(std::move(time_node));

            if (!r.target_months.empty()) {
                auto tmonths_node = std::make_unique<xml_node>();
                tmonths_node->name = "TargetMonths";
                for (int m : r.target_months) {
                    auto m_node = std::make_unique<xml_node>();
                    m_node->name = "Month";
                    m_node->attributes.push_back({ "value", std::to_string(m), "", "" });
                    tmonths_node->children.push_back(std::move(m_node));
                }
                r_node->children.push_back(std::move(tmonths_node));
            }

            auto win_node = std::make_unique<xml_node>();
            win_node->name = "Window";
            win_node->attributes.push_back({ "enabled", r.window.enabled ? "true" : "false", "", "" });
            win_node->attributes.push_back({ "start_h", std::to_string(r.window.start_time.hour), "", "" });
            win_node->attributes.push_back({ "start_m", std::to_string(r.window.start_time.minute), "", "" });
            win_node->attributes.push_back({ "start_s", std::to_string(r.window.start_time.second), "", "" });
            win_node->attributes.push_back({ "end_h", std::to_string(r.window.end_time.hour), "", "" });
            win_node->attributes.push_back({ "end_m", std::to_string(r.window.end_time.minute), "", "" });
            win_node->attributes.push_back({ "end_s", std::to_string(r.window.end_time.second), "", "" });
            r_node->children.push_back(std::move(win_node));

            if (!r.excluded_dates.empty()) {
                auto ex_node = std::make_unique<xml_node>();
                ex_node->name = "ExcludedDates";
                for (const auto& ex : r.excluded_dates) {
                    auto d_node = std::make_unique<xml_node>();
                    d_node->name = "Date";
                    d_node->attributes.push_back({ "year", std::to_string(ex.year), "", "" });
                    d_node->attributes.push_back({ "month", std::to_string(ex.month), "", "" });
                    d_node->attributes.push_back({ "day", std::to_string(ex.day), "", "" });
                    ex_node->children.push_back(std::move(d_node));
                }
                r_node->children.push_back(std::move(ex_node));
            }

            if (!r.overrides.empty()) {
                auto ovs_node = std::make_unique<xml_node>();
                ovs_node->name = "Overrides";
                for (const auto& ov : r.overrides) {
                    auto o_node = std::make_unique<xml_node>();
                    o_node->name = "Override";
                    o_node->attributes.push_back({ "t_year", std::to_string(ov.target_date.year), "", "" });
                    o_node->attributes.push_back({ "t_month", std::to_string(ov.target_date.month), "", "" });
                    o_node->attributes.push_back({ "t_day", std::to_string(ov.target_date.day), "", "" });
                    o_node->attributes.push_back({ "r_year", std::to_string(ov.rescheduled_date.year), "", "" });
                    o_node->attributes.push_back({ "r_month", std::to_string(ov.rescheduled_date.month), "", "" });
                    o_node->attributes.push_back({ "r_day", std::to_string(ov.rescheduled_date.day), "", "" });
                    o_node->attributes.push_back({ "r_hour", std::to_string(ov.rescheduled_time.hour), "", "" });
                    o_node->attributes.push_back({ "r_minute", std::to_string(ov.rescheduled_time.minute), "", "" });
                    o_node->attributes.push_back({ "r_second", std::to_string(ov.rescheduled_time.second), "", "" });
                    ovs_node->children.push_back(std::move(o_node));
                }
                r_node->children.push_back(std::move(ovs_node));
            }

            rules_node->children.push_back(std::move(r_node));
        }

        root->children.push_back(std::move(rules_node));
        return root;
    }

    static std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> parse_xml_node(mino::core::xml::xml_node* root) {
        using namespace mino::core::xml;
        if (!root || root->name != "MonthlySchedule") {
            return std::nullopt;
        }

        holiday_calendar cal(true, true);
        std::vector<monthly_rule> rules;

        // Calendar 파싱
        xml_node* cal_node = root->find_child("Calendar");
        if (cal_node) {
            bool ex_sat = true;
            bool ex_sun = true;
            if (auto* a = cal_node->find_attribute("exclude_saturday")) ex_sat = (a->value == "true" || a->value == "1");
            if (auto* a = cal_node->find_attribute("exclude_sunday"))   ex_sun = (a->value == "true" || a->value == "1");
            cal.set_weekend_exclusions(ex_sat, ex_sun);

            // 단발성 공휴일
            xml_node* holidays_node = cal_node->find_child("Holidays");
            if (holidays_node) {
                for (const auto& h_node : holidays_node->children) {
                    if (h_node->name == "Holiday") {
                        date_ymd d{};
                        if (auto* y = h_node->find_attribute("year"))  d.year = std::stoi(y->value);
                        if (auto* m = h_node->find_attribute("month")) d.month = std::stoi(m->value);
                        if (auto* day = h_node->find_attribute("day")) d.day = std::stoi(day->value);
                        cal.add_holiday(d);
                    }
                }
            }

            // 매년 반복 공휴일
            xml_node* rec_holidays_node = cal_node->find_child("RecurringHolidays");
            if (rec_holidays_node) {
                for (const auto& rh_node : rec_holidays_node->children) {
                    if (rh_node->name == "RecurringHoliday") {
                        date_md md{};
                        if (auto* m = rh_node->find_attribute("month")) md.month = std::stoi(m->value);
                        if (auto* day = rh_node->find_attribute("day")) md.day = std::stoi(day->value);
                        cal.add_recurring_holiday(md);
                    }
                }
            }
        }

        // Rules 파싱
        xml_node* rules_node = root->find_child("Rules");
        if (rules_node) {
            for (const auto& r_node : rules_node->children) {
                if (r_node->name != "Rule") continue;

                monthly_rule r;
                if (auto* a = r_node->find_attribute("id")) r.id = a->value;
                if (auto* a = r_node->find_attribute("pattern")) {
                    r.pattern = (a->value == "ordinal_weekday") ? pattern_type::ordinal_weekday : pattern_type::day_of_month;
                }
                if (auto* a = r_node->find_attribute("day_of_month")) r.day_of_month = std::stoi(a->value);
                if (auto* a = r_node->find_attribute("target_weekday")) {
                    auto wd_opt = weekday_from_str(a->value);
                    if (wd_opt) r.target_weekday = *wd_opt;
                }
                if (auto* a = r_node->find_attribute("target_ordinal")) {
                    r.target_ordinal = static_cast<week_ordinal>(std::stoi(a->value));
                }
                if (auto* a = r_node->find_attribute("interval_months")) r.interval_months = std::stoi(a->value);
                if (auto* a = r_node->find_attribute("anchor_year"))     r.anchor_year = std::stoi(a->value);
                if (auto* a = r_node->find_attribute("anchor_month"))    r.anchor_month = std::stoi(a->value);
                if (auto* a = r_node->find_attribute("policy"))          r.policy = policy_from_str(a->value);

                if (auto* t_node = r_node->find_child("ExecutionTime")) {
                    if (auto* a = t_node->find_attribute("hour"))   r.execution_time.hour = std::stoi(a->value);
                    if (auto* a = t_node->find_attribute("minute")) r.execution_time.minute = std::stoi(a->value);
                    if (auto* a = t_node->find_attribute("second")) r.execution_time.second = std::stoi(a->value);
                }

                if (auto* tm_node = r_node->find_child("TargetMonths")) {
                    for (const auto& m_child : tm_node->children) {
                        if (m_child->name == "Month") {
                            if (auto* a = m_child->find_attribute("value")) {
                                r.target_months.push_back(std::stoi(a->value));
                            }
                        }
                    }
                }

                if (auto* win_node = r_node->find_child("Window")) {
                    if (auto* a = win_node->find_attribute("enabled")) r.window.enabled = (a->value == "true" || a->value == "1");
                    if (auto* a = win_node->find_attribute("start_h")) r.window.start_time.hour = std::stoi(a->value);
                    if (auto* a = win_node->find_attribute("start_m")) r.window.start_time.minute = std::stoi(a->value);
                    if (auto* a = win_node->find_attribute("start_s")) r.window.start_time.second = std::stoi(a->value);
                    if (auto* a = win_node->find_attribute("end_h"))   r.window.end_time.hour = std::stoi(a->value);
                    if (auto* a = win_node->find_attribute("end_m"))   r.window.end_time.minute = std::stoi(a->value);
                    if (auto* a = win_node->find_attribute("end_s"))   r.window.end_time.second = std::stoi(a->value);
                }

                if (auto* ex_node = r_node->find_child("ExcludedDates")) {
                    for (const auto& d_child : ex_node->children) {
                        if (d_child->name == "Date") {
                            date_ymd d{};
                            if (auto* a = d_child->find_attribute("year"))  d.year = std::stoi(a->value);
                            if (auto* a = d_child->find_attribute("month")) d.month = std::stoi(a->value);
                            if (auto* a = d_child->find_attribute("day"))   d.day = std::stoi(a->value);
                            r.excluded_dates.push_back(d);
                        }
                    }
                }

                if (auto* ov_node = r_node->find_child("Overrides")) {
                    for (const auto& o_child : ov_node->children) {
                        if (o_child->name == "Override") {
                            one_time_override ov{};
                            if (auto* a = o_child->find_attribute("t_year"))   ov.target_date.year = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("t_month"))  ov.target_date.month = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("t_day"))    ov.target_date.day = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_year"))   ov.rescheduled_date.year = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_month"))  ov.rescheduled_date.month = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_day"))    ov.rescheduled_date.day = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_hour"))   ov.rescheduled_time.hour = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_minute")) ov.rescheduled_time.minute = std::stoi(a->value);
                            if (auto* a = o_child->find_attribute("r_second")) ov.rescheduled_time.second = std::stoi(a->value);
                            r.overrides.push_back(ov);
                        }
                    }
                }

                rules.push_back(std::move(r));
            }
        }

        return std::make_pair(std::move(cal), std::move(rules));
    }

    std::string to_xml_string(const holiday_calendar& cal, const std::vector<monthly_rule>& rules, bool pretty_print) {
        auto root = build_xml_tree(cal, rules);
        mino::core::xml::write_options opt;
        opt.pretty_print = pretty_print;
        opt.include_declaration = true;
        mino::core::xml::xml_writer writer(opt);
        return writer.write_to_string(*root);
    }

    bool to_xml_file(const std::filesystem::path& file_path, const holiday_calendar& cal, const std::vector<monthly_rule>& rules, bool pretty_print) {
        auto root = build_xml_tree(cal, rules);
        mino::core::xml::write_options opt;
        opt.pretty_print = pretty_print;
        opt.include_declaration = true;
        mino::core::xml::xml_writer writer(opt);
        return writer.write_to_file(file_path, *root);
    }

    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_xml_string(const std::string& xml_text) {
        mino::core::xml::xml_parser parser(xml_text);
        auto root = parser.parse();
        if (!root) return std::nullopt;
        return parse_xml_node(root.get());
    }

    std::optional<std::pair<holiday_calendar, std::vector<monthly_rule>>> from_xml_file(const std::filesystem::path& file_path) {
        mino::core::xml::xml_parser parser;
        auto root = parser.parse_file(file_path);
        if (!root) return std::nullopt;
        return parse_xml_node(root.get());
    }

} // namespace mino::core::schedule::monthly
