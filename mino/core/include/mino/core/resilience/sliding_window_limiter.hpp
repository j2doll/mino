#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>

#include "mino/core/resilience/limiter_clock.hpp"

namespace mino::core::resilience {

    // 슬라이딩 윈도우 속도 제한기
    // - "최근 window 시간 안에 최대 max_requests 회" 를 허용한다.
    // - 고정 윈도우 방식의 경계 버스트 문제(윈도우 경계에서 최대 2배 허용)를 피한다.
    class sliding_window_limiter
    {
    public:

        // 알고리즘 종류
        enum class algorithm_t
        {
            exact, // 요청 시각을 모두 기록하여 정확히 판정 (메모리 O(max_requests))
            approximate // 이전/현재 윈도우 카운터의 가중 합으로 근사 (메모리 O(1))
        };

        struct config_t
        {
            std::size_t max_requests = 100; // 윈도우당 최대 허용 횟수. 0이면 1로 보정
            std::chrono::milliseconds window{ 1000 }; // 윈도우 길이. 0 이하이면 1ms로 보정
            algorithm_t algorithm = algorithm_t::exact;
        };

        // 생성자 및 소멸자
        sliding_window_limiter() : sliding_window_limiter(config_t()) {}
        explicit sliding_window_limiter(config_t cfg);
        sliding_window_limiter(config_t cfg, limiter_clock_fn_t clock_fn); // 시계 주입 (테스트용)
        sliding_window_limiter(const sliding_window_limiter&) = delete;
        sliding_window_limiter& operator=(const sliding_window_limiter&) = delete;
        ~sliding_window_limiter() = default;

        // n회 요청 허용 여부 판정. 허용되면 기록하고 true, 아니면 (기록 없이) false
        bool try_acquire(std::size_t n = 1);

        // n회 요청이 허용될 때까지 남은 시간 (밀리초 올림). 이미 가능하면 0
        // - exact      : 정확한 값
        // - approximate: 현재 윈도우가 끝날 때까지의 시간 (근사 상한 추정)
        // n > max_requests 이면 std::chrono::milliseconds::max() 를 반환한다.
        std::chrono::milliseconds retry_after(std::size_t n = 1) const;

        // 현재 윈도우에서 사용 중인 요청 수 (approximate 는 가중 추정치를 내림한 값)
        std::size_t used() const;

        // 현재 설정값 조회 (보정이 반영된 값)
        config_t config() const noexcept;

        // 기록 초기화
        void reset();

    private:

        // 아래 *_locked 메서드는 mutex_ 를 잡은 상태에서만 호출한다.
        void purge_locked(limiter_time_point_t now) const;
        void advance_window_locked(limiter_time_point_t now) const;
        double estimate_locked(limiter_time_point_t now) const;

        config_t config_;
        limiter_clock_fn_t clock_fn_;

        mutable std::mutex mutex_;

        // exact 용
        mutable std::deque<limiter_time_point_t> timestamps_;

        // approximate 용
        mutable std::size_t current_count_;
        mutable std::size_t previous_count_;
        mutable limiter_time_point_t window_start_;
    };

}
