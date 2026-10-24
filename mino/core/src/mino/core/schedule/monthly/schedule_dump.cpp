#include <sstream>
#include <iomanip>
#include "mino/core/schedule/monthly/schedule_dump.hpp"

namespace mino::core::schedule::monthly {

    std::string dump_rules(const std::vector<monthly_rule>& rules) {
        static const char* wd_names[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        static const char* ord_names[] = { "", "1st", "2nd", "3rd", "4th", "last" };

        std::ostringstream oss;
        for (const auto& r : rules) {
            oss << "[" << (r.id.empty() ? "Rule" : r.id) << "] ";

            if (r.pattern == pattern_type::last_day_of_month) {
                oss << "Last day of month";
            }
            else if (r.pattern == pattern_type::day_of_month) {
                oss << "Day " << r.day_of_month << " of month";
            }
            else {
                int ord_idx = static_cast<int>(r.target_ordinal);
                oss << ord_names[ord_idx] << " " << wd_names[static_cast<int>(r.target_weekday)];
            }

            if (!r.target_months.empty()) {
                oss << " in month(s) (";
                for (size_t i = 0; i < r.target_months.size(); ++i) {
                    oss << r.target_months[i] << (i + 1 < r.target_months.size() ? "," : "");
                }
                oss << ")";
            }
            else if (r.interval_months > 1) {
                oss << " every " << r.interval_months << " month(s)";
            }

            oss << " at "
                << std::setw(2) << std::setfill('0') << r.execution_time.hour << ":"
                << std::setw(2) << std::setfill('0') << r.execution_time.minute << ":"
                << std::setw(2) << std::setfill('0') << r.execution_time.second;

            if (r.window.enabled) {
                oss << " [Window "
                    << std::setw(2) << std::setfill('0') << r.window.start_time.hour << ":"
                    << std::setw(2) << std::setfill('0') << r.window.start_time.minute << " ~ "
                    << std::setw(2) << std::setfill('0') << r.window.end_time.hour << ":"
                    << std::setw(2) << std::setfill('0') << r.window.end_time.minute << "]";
            }
            oss << "\n";
        }
        return oss.str();
    }

} // namespace mino::core::schedule::monthly
