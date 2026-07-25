#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <memory>
#include <type_traits> 

// #include <spdlog/spdlog.h>
#include "mino/core/log/tinylog/tinylog.hpp"

namespace mino::core::thread {

    // 스레드에서 주기적으로 실행할 작업의 인터페이스
    class thread_task {
    public:
        virtual ~thread_task() = default;   // 다형 삭제를 위한 가상 소멸자
        virtual void performTask() = 0;     // 실제 작업 본문
    };

    class dynamic_thread {
    public:
        dynamic_thread();
        ~dynamic_thread();

        // 외부에서 spdlog::logger를 등록 (nullptr로 설정하면 등록 해제)
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger);

        // 작업 주기 설정
        void setInterval(std::chrono::milliseconds newInterval);

        // 템플릿 오버로드: "진짜 호출 가능한 대상"일 때만 참여(SFINAE)
        template <typename Callable, typename... Args,
            typename = std::enable_if_t<std::is_invocable_v<Callable, Args...>>>
        void start(Callable&& func, Args&&... args) {
            if (running_.load(std::memory_order_acquire)) {
                if (s_logger_) {
                    s_logger_->warn("Thread is already running.");
                }
                return;
            }
            // 콜러블을 바인딩 또는 람다로 감싸 std::function<void()>에 저장
            task_ = std::bind(std::forward<Callable>(func), std::forward<Args>(args)...);
            running_.store(true, std::memory_order_release);
            workerThread_ = std::thread(&dynamic_thread::threadFunction, this);
        }

        // 전용 오버로드: thread_task 객체만 등록 (참조 버전)
        // 주의: 참조 대상의 수명은 호출자가 보장해야 합니다.
        void start(thread_task& obj) {
            if (running_.load(std::memory_order_acquire)) {
                if (s_logger_) {
                    s_logger_->warn("Thread is already running.");
                }
                return;
            }
            // parameter 'obj' is a local variable; capturing it by reference would
            // create a dangling reference after this function returns.
            // Store a reference_wrapper in the closure so the closure keeps a valid
            // reference to the original object.
            task_ = [ref = std::ref(obj)] { ref.get().performTask(); };
            running_.store(true, std::memory_order_release);
            workerThread_ = std::thread(&dynamic_thread::threadFunction, this);
        }

        // 전용 오버로드: thread_task 객체만 등록 (shared_ptr 버전)
        // 장점: 스레드가 도는 동안 객체의 수명 자동 관리
        void start(std::shared_ptr<thread_task> obj) {
            if (running_.load(std::memory_order_acquire)) {
                if (s_logger_) {
                    s_logger_->warn("Thread is already running.");
                }
                return;
            }
            task_obj_ = std::move(obj);            // 내부 보관으로 수명 유지
            task_ = [sp = task_obj_] { if (sp) sp->performTask(); };
            running_.store(true, std::memory_order_release);
            workerThread_ = std::thread(&dynamic_thread::threadFunction, this);
        }

        // 스레드 정지
        void stop();

        // 실행 여부 확인
        bool isRunning() const;

    protected:
        void threadFunction();

        std::thread workerThread_;                 // 스레드 객체
        std::atomic<bool> running_{ false };         // 실행 상태
        std::function<void()> task_{ nullptr };      // 실행할 작업(콜러블)
        std::chrono::milliseconds interval_{ 100 };  // 작업 주기 (기본 100ms)

        // shared_ptr 버전 start() 사용 시 수명 보조용 보관
        std::shared_ptr<thread_task> task_obj_{};

        // 외부에서 주입된 spdlog 로거 
        std::shared_ptr<mino::core::log::tinylog::logger> s_logger_;

    };

} 
