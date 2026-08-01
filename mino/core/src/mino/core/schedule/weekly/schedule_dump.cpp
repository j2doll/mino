#include <sstream>

#include "mino/core/schedule/weekly/schedule_dump.hpp"

namespace mino::core::schedule::weekly {

    static const char* day_name[] =
    { "Mon","Tue","Wed","Thu","Fri","Sat","Sun" };

    std::string dump_schedule(const weekly_ranges& ranges) {
        std::ostringstream oss;
        for (const auto& r : ranges) {
            oss << day_name[(int)r.start_day] << " "
                << r.start_time.hour << ":" << r.start_time.minute
                << " -> "
                << day_name[(int)r.end_day] << " "
                << r.end_time.hour << ":" << r.end_time.minute
                << "\n";
        }
        return oss.str();
    }

}
