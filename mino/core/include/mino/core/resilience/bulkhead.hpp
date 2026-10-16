#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace mino::core::resilience {

    // 벌크헤드 (동시 실행 수 제한기)
    // - 동시에 실행 중인 작업을 최대 max_concurrent 개로 제한하여 자원 고갈과 장애 전파를 막는다.
    // - 획득한 슬롯은 permit(RAII)이 소멸되거나 release() 될 때 자동으로 반환된다.
    // - 주의: 모든 permit 은 bulkhead 보다 먼저 소멸(반환)되어야 한다.
    class bulkhead
    {
    public:

        struct config_t
        {
            std::size_t max_concurrent = 10; // 최대 동시 실행 수. 0이면 1로 보정
        };

        // 슬롯 획득 증표 (이동만 가능)
        class permit
        {
        public:
            permit() noexcept : owner_(nullptr) {}
            permit(permit&& other) noexcept;
            permit& operator=(permit&& other) noexcept;
            permit(const permit&) = delete;
            permit& operator=(const permit&) = delete;
            ~permit();

            // 슬롯 획득에 성공했는지 여부
            explicit operator bool() const noexcept { return owner_ != nullptr; }

            // 슬롯을 즉시 반환 (여러 번 호출해도 안전)
            void release() noexcept;

        private:
            friend class bulkhead;
            explicit permit(bulkhead* owner) noexcept : owner_(owner) {}

            bulkhead* owner_;
        };

        // 생성자 및 소멸자
        bulkhead() : bulkhead(config_t()) {}
        explicit bulkhead(config_t cfg);
        bulkhead(const bulkhead&) = delete;
        bulkhead& operator=(const bulkhead&) = delete;
        ~bulkhead(); // close() 후 대기 중인 모든 스레드가 빠져나갈 때까지 대기

        // 슬롯 즉시 획득 시도. 실패하면 false 로 평가되는 빈 permit 반환
        permit try_acquire();

        // 슬롯을 timeout 동안 기다리며 획득 시도. 실패하면 false 로 평가되는 빈 permit 반환
        // timeout 에는 합리적인 범위의 값을 사용할 것 (예: 수 시간 이내)
        permit acquire_for(std::chrono::milliseconds timeout);

        // 현재 실행 중인 수 / 남은 슬롯 수
        std::size_t active() const;
        std::size_t available() const;

        // 현재 설정값 조회 (보정이 반영된 값)
        config_t config() const noexcept;

        // 종료 처리: 이후 모든 획득은 실패하고, 대기 중인 스레드는 깨어난다.
        void close();
        bool is_closed() const;

    private:

        void release_slot() noexcept;

        config_t config_;

        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::size_t active_;
        bool closed_;
        std::size_t waiters_;
    };

}
