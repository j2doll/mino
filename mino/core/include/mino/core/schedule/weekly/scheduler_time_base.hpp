#pragma once

#include <chrono>

namespace mino::core::schedule::weekly {

    // 로컬 타임, UTC 타임
    enum class time_base {
        localtime,
        utc
    };

    // 스케쥴러 시간 기반
    class  scheduler_time_base {
    public:
        explicit scheduler_time_base(time_base base = time_base::localtime);

        std::tm now_tm() const;

        time_base get_base() const {
            return base_;
        }

    private:
        time_base base_;
    };

}
