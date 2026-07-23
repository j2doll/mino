#include <ctime>

#include "mino/external/schedule/weekly/scheduler_time_base.hpp"

namespace mino::external::schedule::weekly {

    scheduler_time_base::scheduler_time_base(time_base base)
        : base_(base) {
    }

    std::tm scheduler_time_base::now_tm() const {
        std::time_t t = std::time(nullptr);
        std::tm out{};
#if defined(_WIN32)
        if (base_ == time_base::utc) {
            // Windows thread-safe variants
            gmtime_s(&out, &t);
        }
        else {
            localtime_s(&out, &t);
        }
#else
        if (base_ == time_base::utc) {
            gmtime_r(&t, &out);
        }
        else {
            localtime_r(&t, &out);
        }
#endif
        return out;
    }

}
