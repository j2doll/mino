#pragma once

#include <chrono>
#include <functional>

namespace mino::core::resilience {

    // 리미터 공용 시계 타입
    // - 기본은 std::chrono::steady_clock (시스템 시간 변경에 영향받지 않음)
    // - 단위 테스트에서는 시계 함수를 주입하여 시간 흐름을 제어할 수 있다.
    using limiter_clock_t = std::chrono::steady_clock;
    using limiter_time_point_t = limiter_clock_t::time_point;
    using limiter_clock_fn_t = std::function<limiter_time_point_t()>;

}
