#include <string>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <optional>
#include <numeric>
#include <cmath>

#include "mino/core/json/json.hpp"
#include "mino/core/schedule/weekly/schedule_json.hpp"

namespace mino::core::schedule::weekly {

    std::optional<weekday> day_from_string(const std::string& s) {
        const char* day_name[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

        for (int i = 0; i < 7; ++i) {
            // std::string dn = day_name[i];
            // std::string lower = [](std::string s) { for (char& c : s) c = std::tolower(static_cast<unsigned char>(c)); return s; }(dn);
            if (s == day_name[i])
                return static_cast<weekday>(i);
        }
        return std::nullopt;
    }

    std::string to_json_string(const weekly_ranges& ranges, int indent) {
        const char* day_name[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

        using namespace mino::core::json;

        array_t arr;
        arr.reserve(ranges.size());

        for (const auto& r : ranges) {
            object_t obj;
            obj["start_day"] = value(std::string(day_name[static_cast<int>(r.start_day)]));
            obj["start_h"] = value(r.start_time.hour);
            obj["start_m"] = value(r.start_time.minute);
            obj["end_day"] = value(std::string(day_name[static_cast<int>(r.end_day)]));
            obj["end_h"] = value(r.end_time.hour);
            obj["end_m"] = value(r.end_time.minute);
            arr.push_back(value(std::move(obj)));
        }

        auto ret = serializer::serialize( value(std::move(arr)), indent );
        return ret; 
    }

    std::optional<weekly_ranges> from_json_string(const std::string& json_text) {
        using namespace mino::core::json;

        value root = parser::parse(json_text);
        if (!root.is_array()) return std::nullopt;

        const auto& arr = std::get<array_t>(root.data);
        weekly_ranges result;
        result.reserve(arr.size());

        for (const auto& item : arr) {
            if (!item.is_object()) return std::nullopt;
            const auto& obj = std::get<object_t>(item.data);

            // required fields
            auto it_sd = obj.find("start_day");
            auto it_sh = obj.find("start_h");
            auto it_sm = obj.find("start_m");
            auto it_ed = obj.find("end_day");
            auto it_eh = obj.find("end_h");
            auto it_em = obj.find("end_m");

            if (it_sd == obj.end() || it_sh == obj.end() || it_sm == obj.end()
                || it_ed == obj.end() || it_eh == obj.end() || it_em == obj.end()) {
                return std::nullopt;
            }

            // start_day / end_day (strings)
            if (!it_sd->second.is_string() || !it_ed->second.is_string()) return std::nullopt;
            const std::string& sd = std::get<std::string>(it_sd->second.data);
            const std::string& ed = std::get<std::string>(it_ed->second.data);

            auto sd_opt = day_from_string(sd);
            auto ed_opt = day_from_string(ed);
            if (!sd_opt.has_value() || !ed_opt.has_value()) return std::nullopt;

            // hours / minutes (numbers parsed as double)
            if (!it_sh->second.is_number() || !it_sm->second.is_number()
                || !it_eh->second.is_number() || !it_em->second.is_number()) {
                return std::nullopt;
            }

            double sh_d = std::get<double>(it_sh->second.data);
            double sm_d = std::get<double>(it_sm->second.data);
            double eh_d = std::get<double>(it_eh->second.data);
            double em_d = std::get<double>(it_em->second.data);

            // ensure they are integers
            auto is_integral = [](double x) {
                return std::fabs(x - std::round(x)) < 1e-9;
                };

            if (!is_integral(sh_d) || !is_integral(sm_d) || !is_integral(eh_d) || !is_integral(em_d)) {
                return std::nullopt;
            }

            int sh = static_cast<int>(std::lround(sh_d));
            int sm = static_cast<int>(std::lround(sm_d));
            int eh = static_cast<int>(std::lround(eh_d));
            int em = static_cast<int>(std::lround(em_d));

            // range checks
            if (sh < 0 || sh > 23 || eh < 0 || eh > 23 || sm < 0 || sm > 59 || em < 0 || em > 59) {
                return std::nullopt;
            }

            weekly_range wr;
            wr.start_day = sd_opt.value();
            wr.start_time.hour = sh;
            wr.start_time.minute = sm;
            wr.end_day = ed_opt.value();
            wr.end_time.hour = eh;
            wr.end_time.minute = em;

            result.push_back(std::move(wr));
        }

        return result;
    }


} // namespace mino::core::schedule::weekly

