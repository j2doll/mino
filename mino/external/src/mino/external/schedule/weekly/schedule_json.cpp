#include <string>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <optional>

#include <nlohmann/json.hpp>

#include "mino/external/schedule/weekly/schedule_json.hpp"

namespace mino::external::schedule::weekly {

    std::string to_json_string(const mino::core::schedule::weekly::weekly_ranges& ranges) {
        nlohmann::json arr = nlohmann::json::array();
        static const char* day_name[] = { "Mon","Tue","Wed","Thu","Fri","Sat","Sun" };

        for (const auto& r : ranges) {
            nlohmann::json obj;
            obj["start_day"] = std::string(day_name[static_cast<int>(r.start_day)]);
            obj["start_h"]   = r.start_time.hour;
            obj["start_m"]   = r.start_time.minute;
            obj["end_day"]   = std::string(day_name[static_cast<int>(r.end_day)]);
            obj["end_h"]     = r.end_time.hour;
            obj["end_m"]     = r.end_time.minute;
            arr.push_back(obj);
        }
        return arr.dump();
    }

    namespace {
        static std::string lower_trim3(const std::string& s) {
            std::string t;
            t.reserve(3);
            for (char c : s) {
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                    if (t.size() == 3) 
                        break;
                }
            }
            return t;
        }

        static std::optional<mino::core::schedule::weekly::weekday> parse_day_value(const nlohmann::json& v)
        {
            if (v.is_string()) {
                using weekday = mino::core::schedule::weekly::weekday;
                std::string s = v.get<std::string>();
                std::string p = lower_trim3(s);
                if (p == "mon") return weekday::mon;
                if (p == "tue") return weekday::tue;
                if (p == "wed") return weekday::wed;
                if (p == "thu") return weekday::thu;
                if (p == "fri") return weekday::fri;
                if (p == "sat") return weekday::sat;
                if (p == "sun") return weekday::sun;

                return std::nullopt;
            }
            else if (v.is_number_integer()) {
                using weekday = mino::core::schedule::weekly::weekday;
                int i = v.get<int>();
                if (i < 0 || i > 6) {
                    return std::nullopt;
                }
                return static_cast<weekday>(i);
            }
            else {
                return std::nullopt;
            }
            return std::nullopt;
        }
    } // namespace

    std::optional <weekly_ranges> from_json_string(const std::string& json_text) {

        nlohmann::json j = nlohmann::json::parse(json_text);
        if (!j.is_array()) {
            return std::nullopt;
        }

        mino::core::schedule::weekly::weekly_ranges out;
        for (const auto& el : j) {
            if (!el.is_object()) {
                return std::nullopt;
            }

            if (!el.contains("start_day") ||
                !el.contains("start_h") ||
                !el.contains("start_m") ||
                !el.contains("end_day") ||
                !el.contains("end_h") ||
                !el.contains("end_m") )
            {
                return std::nullopt;
            }

            mino::core::schedule::weekly::weekly_range r;

            auto sd = parse_day_value(el.at("start_day"));
            if (sd) {
                r.start_day = *sd;
            }
            else {
                return std::nullopt;
            }

            r.start_time.hour = el.at("start_h").get<int>();
            r.start_time.minute = el.at("start_m").get<int>();

            auto ed = parse_day_value(el.at("end_day"));
            if (ed) {
                r.end_day = *ed;
            }
            else {
                return std::nullopt;
            }

            r.end_time.hour = el.at("end_h").get<int>();
            r.end_time.minute = el.at("end_m").get<int>();

            out.push_back(r);
        }

        return out;
    }

}  // namespace mino::external::schedule::weekly
