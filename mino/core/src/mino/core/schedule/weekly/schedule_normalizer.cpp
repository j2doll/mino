#include <algorithm>

#include "mino/core/schedule/weekly/schedule_normalizer.hpp"

namespace mino::core::schedule::weekly {

    static void push_intervals_from_range(const weekly_range& r, std::vector<interval>& out) {
        int sd = static_cast<int>(r.start_day);
        int ed = static_cast<int>(r.end_day);
        int sm = r.start_time.hour * 60 + r.start_time.minute;
        int em = r.end_time.hour * 60 + r.end_time.minute;

        if (sd == ed) {
            out.push_back({sd, sm, em});
            return;
        }

        // span across days: break into parts
        // start day: sm .. 1439
        out.push_back({sd, sm, 24 * 60 - 1});

        // days in between
        int cur = (sd + 1) % 7;
        while (cur != ed) {
            out.push_back({cur, 0, 24 * 60 - 1});
            cur = (cur + 1) % 7;
        }

        // end day: 0 .. em
        out.push_back({ed, 0, em});
    }

    weekly_ranges schedule_normalizer::normalize(const weekly_ranges& input) {
        std::vector<interval> intervals;
        for (const auto& r : input) {
            push_intervals_from_range(r, intervals);
        }

        // sort by day, start
        std::sort(intervals.begin(), intervals.end(), [](const interval& a, const interval& b) {
            if (a.day != b.day) return a.day < b.day;
            if (a.start != b.start) return a.start < b.start;
            return a.end < b.end;
        });

        // merge overlapping/adjacent intervals per day
        std::vector<interval> merged;
        for (const auto& iv : intervals) {
            if (merged.empty() || merged.back().day != iv.day || iv.start > merged.back().end + 1) {
                merged.push_back(iv);
            } else {
                // overlap or adjacent
                merged.back().end = std::max(merged.back().end, iv.end);
            }
        }

        // convert back to weekly_ranges
        weekly_ranges out;
        for (const auto& m : merged) {
            weekly_range r;
            r.start_day = static_cast<weekday>(m.day);
            r.end_day = static_cast<weekday>(m.day);
            r.start_time.hour = m.start / 60;
            r.start_time.minute = m.start % 60;
            r.end_time.hour = m.end / 60;
            r.end_time.minute = m.end % 60;
            out.push_back(r);
        }

        return out;
    }

}
