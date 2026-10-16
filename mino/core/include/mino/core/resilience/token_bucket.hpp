#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

#include "mino/core/resilience/limiter_clock.hpp"

namespace mino::core::resilience {

    // 토큰 버킷 속도 제한기
    // - 버킷에 최대 capacity 개의 토큰을 보관하고, refill_period 마다 refill_tokens 개씩 보충한다.
    // - 평균 속도는 제한하면서 capacity 만큼의 순간 버스트는 허용한다.
    // - 백그라운드 스레드 없이, 호출 시점에 경과 시간을 계산하여 토큰을 보충한다. (지연 보충)
    // - 보충은 "주기 단위(이산)"로 이루어지며, 정수 연산만 사용하므로 오차가 누적되지 않는다.
    class token_bucket
    {
    public:

        struct config_t
        {
            std::size_t capacity = 10; // 최대 토큰 수 (최대 버스트 크기). 0이면 1로 보정
            std::size_t refill_tokens = 1; // 주기당 보충 토큰 수. 0이면 1로 보정
            std::chrono::milliseconds refill_period{ 1000 }; // 보충 주기. 0 이하이면 1ms로 보정
        };

        // 생성자 및 소멸자
        token_bucket() : token_bucket(config_t()) {}
        explicit token_bucket(config_t cfg);
        token_bucket(config_t cfg, limiter_clock_fn_t clock_fn); // 시계 주입 (테스트용)
        token_bucket(const token_bucket&) = delete;
        token_bucket& operator=(const token_bucket&) = delete;
        ~token_bucket(); // close() 후 대기 중인 모든 스레드가 빠져나갈 때까지 대기

        // 토큰 n개를 즉시 획득 시도. 성공하면 true, 부족하면 (토큰을 소모하지 않고) false
        bool try_acquire(std::size_t n = 1);

        // 토큰 n개를 timeout 동안 기다리며 획득 시도. 성공하면 true
        // - n > capacity 이면 영원히 획득할 수 없으므로 즉시 false
        // - close() 가 호출되면 대기 중이던 호출은 false 로 반환
        // - 대기 시간은 실제 시간으로 흐르므로, 가짜 시계를 주입한 경우에는 사용하지 않는 것을 권장
        bool acquire_for(std::size_t n, std::chrono::milliseconds timeout);

        // 토큰 n개를 획득할 수 있을 때까지 남은 시간 (밀리초 올림). 이미 가능하면 0
        // n > capacity 이면 std::chrono::milliseconds::max() 를 반환한다.
        // HTTP 429 응답의 Retry-After 계산이나 retry_helper 와의 연동에 사용한다.
        std::chrono::milliseconds retry_after(std::size_t n = 1) const;

        // 현재 사용 가능한 토큰 수
        std::size_t available() const;

        // 현재 설정값 조회 (보정이 반영된 값)
        config_t config() const noexcept;

        // 토큰을 가득 채운 초기 상태로 되돌림
        void reset();

        // 종료 처리: 이후 모든 획득은 실패하고, 대기 중인 스레드는 깨어난다.
        void close();
        bool is_closed() const;

    private:

        // 아래 *_locked 메서드는 mutex_ 를 잡은 상태에서만 호출한다.
        void refill_locked(limiter_time_point_t now) const;
        limiter_clock_t::duration wait_time_locked(std::size_t n, limiter_time_point_t now) const;

        config_t config_;
        limiter_clock_fn_t clock_fn_;

        mutable std::mutex mutex_;
        std::condition_variable cv_;
        mutable std::size_t tokens_;
        mutable limiter_time_point_t last_refill_;
        bool closed_;
        std::size_t waiters_;
    };

}
