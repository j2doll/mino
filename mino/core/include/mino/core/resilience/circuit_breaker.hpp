#pragma once

#include <any>
#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <tuple>
#include <utility>

namespace mino::core::resilience {

    // 써킷 오픈 예외 클래스 (하위 호환성 유지용)
    class circuit_open_exception : public std::runtime_error
    {
    public:
        explicit circuit_open_exception(const char* msg = "circuit is open")
            : std::runtime_error(msg)
        {
        }
    };

    // 써킷 브레이커 클래스
    class circuit_breaker
    {
    public:

        // 써킷 상태 
        enum class state
        {
            open, // 회로가 열려 있어 모든 호출이 "차단"됨
            closed, // 회로가 닫혀 있어 모든 호출이 정상적으로 전달됨
            half_open // 회로가 반쯤 열려 있어 제한된 호출만 허용되고 결과에 따라 상태가 전환됨
        };

        struct config_t
        {
            std::size_t failure_threshold = 5; // 5회 연속 실패 시 open
            std::optional<std::chrono::milliseconds> reset_timeout = std::chrono::milliseconds{ 30000 };
            std::size_t half_open_success_threshold = 2;
        };

        // 생성자 및 소멸자
        circuit_breaker() noexcept : circuit_breaker(config_t()) {}
        explicit circuit_breaker(config_t cfg) noexcept;
        circuit_breaker(const circuit_breaker&) = delete;
        circuit_breaker& operator=(const circuit_breaker&) = delete;
        ~circuit_breaker();

        // 실행 메서드
        void execute_void(
            std::function<void()> op
        );

        std::any execute_any(
            std::function<std::any()> op
        );

        std::future<std::any> execute_async_any(
            std::function<std::any()> op
        );

        void execute_with_callback_any(
            std::function<std::any()> op,
            std::function<void(std::optional<std::any>,
                std::exception_ptr)> cb
        );

        // 현재 써킷 상태 조회
        state current_state() const noexcept;

        // 현재 설정값 조회
        config_t config() const noexcept;

        // 써킷 상태 초기화
        void reset() noexcept;

        // 템플릿 실행 메서드
        template <typename Callable, typename... Args,
            typename result_t = std::invoke_result_t<Callable, Args...>>
            auto execute(Callable&& callable, Args&&... args) -> result_t
        {
            if constexpr (std::is_void_v<result_t>)
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                execute_void(std::function<void()>(std::move(bound)));
                return;
            }
            else
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                std::function<std::any()> any_fn = [b = std::move(bound)]() mutable -> std::any {
                    return std::any(std::invoke(b));
                    };

                std::any a = execute_any(std::move(any_fn));
                if (a.has_value())
                {
                    return std::any_cast<result_t>(std::move(a));
                }
                return result_t{};
            }
        }

        // 템플릿 비동기 실행 메서드
        template <typename Callable, typename... Args,
            typename result_t = std::invoke_result_t<Callable, Args...>>
            auto execute_async(Callable&& callable, Args&&... args) -> std::future<result_t>
        {
            if constexpr (std::is_void_v<result_t>)
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                std::function<std::any()> any_fn = [b = std::move(bound)]() mutable -> std::any {
                    std::invoke(b);
                    return std::any{};
                    };

                auto fut_any = execute_async_any(std::move(any_fn));
                return std::async(std::launch::async, [f = std::move(fut_any)]() mutable {
                    f.get();
                    });
            }
            else
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                std::function<std::any()> any_fn = [b = std::move(bound)]() mutable -> std::any {
                    return std::any(std::invoke(b));
                    };

                auto fut_any = execute_async_any(std::move(any_fn));
                return std::async(std::launch::async, [f = std::move(fut_any)]() mutable -> result_t {
                    std::any a = f.get();
                    if (a.has_value())
                    {
                        return std::any_cast<result_t>(std::move(a));
                    }
                    return result_t{};
                    });
            }
        }

        // 템플릿 콜백 실행 메서드
        template <typename Callable, typename Callback, typename... Args,
            typename result_t = std::invoke_result_t<Callable, Args...>>
            void execute_with_callback(Callable&& callable, Callback&& callback, Args&&... args)
        {
            if constexpr (std::is_void_v<result_t>)
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                std::function<std::any()> any_fn = [b = std::move(bound)]() mutable -> std::any {
                    std::invoke(b);
                    return std::any{};
                    };

                auto any_cb = [cb = std::forward<Callback>(callback)](std::optional<std::any> /*r*/, std::exception_ptr eptr) mutable {
                    cb(eptr);
                    };

                execute_with_callback_any(std::move(any_fn), std::move(any_cb));
            }
            else
            {
                auto bound = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
                std::function<std::any()> any_fn = [b = std::move(bound)]() mutable -> std::any {
                    return std::any(std::invoke(b));
                    };

                auto any_cb = [cb = std::forward<Callback>(callback)](std::optional<std::any> r, std::exception_ptr eptr) mutable {
                    cb(std::move(r), eptr);
                    };

                execute_with_callback_any(std::move(any_fn), std::move(any_cb));
            }
        }

    protected:

        // 내부 상태 관리 메서드 (실행 가능 여부 반환)
        bool pre_execute_check();

        // 호출 성공 시 상태 업데이트
        void on_success() noexcept;

        // 호출 실패 시 상태 업데이트
        void on_failure() noexcept;

        config_t config_;
        std::atomic<state> state_;
        std::atomic<std::size_t> consecutive_failures_;
        std::atomic<std::size_t> consecutive_successes_;
        std::chrono::steady_clock::time_point last_failure_time_;
        mutable std::mutex mutex_;
    };

}
