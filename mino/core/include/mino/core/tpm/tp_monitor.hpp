#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <atomic>
#include <any>
#include <tuple>
#include <type_traits>

// #include <spdlog/spdlog.h>
// #include <spdlog/sinks/stdout_color_sinks.h>
#include "mino/core/log/tinylog/tinylog.hpp"

namespace mino::core::tpm {

    // `transaction_context`
    //  - 트랜잭션의 최소한의 공통 정보를 제공하는 베이스 클래스입니다.
    //  - `id`: 트랜잭션 식별자
    //  - `is_aborted`: 서비스 처리 도중 `abort()`가 호출되면 true로 설정됩니다.
    //  - `set_logger`/`get_logger`: 트랜잭션 단위 로그 출력을 위해 로거를 주입/획득하는 인터페이스.
    class  transaction_context {
    public:
        uint64_t id;
        bool is_aborted{ false };

        // 생성자: 고유 tx id를 전달받아 초기화합니다.
        explicit transaction_context(uint64_t tx_id);
        virtual ~transaction_context() = default;

        // 트랜잭션을 중단 상태로 표시합니다.
        // -> 서비스 처리 중 치명적 오류가 발생했을 때 호출됩니다.
        void abort();

        // 트랜잭션 전용 로거를 설정/획득합니다.
        // void set_logger(std::shared_ptr<spdlog::logger> logger);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger);

        std::shared_ptr<mino::core::log::tinylog::logger> get_logger() const;

    protected:
        // 트랜잭션 범위 로거: 설정되어 있지 않으면 모니터의 공통 로거를 사용합니다.
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;
    };

    // `tp_monitor`
    //  - 서비스 등록, 워커 스레드 관리, 비동기 서비스 요청 처리 등을 담당합니다.
    //  - 서비스 함수는 `service_function` 시그니처를 따라야 하며,
    //    `bool(transaction_context&, const std::vector<std::any>&)` 형태입니다.
    class  tp_monitor {
    public:
        using service_function = std::function<bool(transaction_context&, const std::vector<std::any>&)>;

        tp_monitor();
        ~tp_monitor();
        tp_monitor(const tp_monitor&) = delete;
        tp_monitor& operator=(const tp_monitor&) = delete;

        // 워커 스레드를 `num_workers` 만큼 시작합니다.
        // 내부적으로 호출 시 이미 시작된 워커가 있으면 추가로 시작하거나,
        // 내부 정리 로직에 따라 다르게 동작할 수 있습니다(구현체 확인 필요).
        void start_workers(size_t num_workers);

        // 공통 모니터 로거 설정/획득
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger);

        std::shared_ptr<mino::core::log::tinylog::logger> get_logger() const;

        // 서비스 등록: `service_name`으로 라우팅됩니다.
        void register_service(const std::string& service_name, service_function func);

        // 💡 서비스별 개별 컨텍스트 로거를 등록하는 멤버 함수
        //  - 특정 서비스에 대해 별도 로거를 사용하고자 할 때 호출합니다.
        void set_ctx_logger(std::string service_name, std::shared_ptr<mino::core::log::tinylog::logger> logger);

        // `request_service` 템플릿:
        //  - `ContextType`은 `transaction_context`를 상속해야 합니다.
        //  - `context_factory`는 `std::unique_ptr<ContextType> factory(uint64_t tx_id)` 형태로 컨텍스트를 생성해야 합니다.
        //  - `commit_action`은 `void commit(ContextType&)` 형태로 성공 시 최종 커밋 작업을 수행합니다.
        //  - 추가 `Args`는 서비스 함수에 전달될 인자들입니다(내부적으로 `std::any`로 변환되어 전달됨).
        template <typename ContextType, typename FactoryFunc, typename CommitFunc, typename... Args>
        auto request_service(const std::string& service_name,
            FactoryFunc&& context_factory,
            CommitFunc&& commit_action,
            Args&&... args)
            -> std::future<bool>;

    private:
        // 워커 루프: 큐에서 작업을 꺼내 실행하는 함수입니다.
        void worker_loop();

        std::vector<std::thread> workers_;
        // 요청 큐: 각 요청은 실행 가능한 래퍼(무인자 함수)로 큐에 저장됩니다.
        std::queue<std::function<void()>> request_queue_;

        // 서비스 라우팅 맵
        std::unordered_map<std::string, service_function> services_;

        // 💡 서비스별 개별 로거를 매핑 관리할 컨테이너
        // std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> ctx_loggers_;
        std::unordered_map<std::string, std::shared_ptr<mino::core::log::tinylog::logger>> ctx_loggers_;

        // 동기화 객체들
        std::mutex queue_mutex_;
        std::shared_mutex service_mutex_;
        std::condition_variable cv_;

        bool stop_;
        std::atomic<uint64_t> tx_counter_;

        // 모니터 공통 로거: 서비스별 로거가 없을 때 사용됩니다.
        // std::shared_ptr<spdlog::logger> logger_;
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;

    };

    // request_service 템플릿 구현
    template <typename ContextType, typename FactoryFunc, typename CommitFunc, typename... Args>
    auto tp_monitor::request_service(const std::string& service_name,
        FactoryFunc&& context_factory,
        CommitFunc&& commit_action,
        Args&&... args)
        -> std::future<bool>
    {
        // 타입 제약: ContextType은 transaction_context를 상속해야 합니다.
        static_assert(std::is_base_of<transaction_context, ContextType>::value,
            "ContextType must derive from transaction_context");

        // 트랜잭션 ID 발급(원자)
        auto tx_id = ++tx_counter_;

        // 가변 인자를 튜플로 만들고, std::any 벡터로 변환하여 서비스에 전달합니다.
        auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
        auto args_vector = std::make_shared<std::vector<std::any>>();

        std::apply([&args_vector](auto&&... unpacked_args) {
            (args_vector->push_back(std::any(std::forward<decltype(unpacked_args)>(unpacked_args))), ...);
            }, args_tuple);

        // 패키지드 태스크 생성: 실제 서비스 호출 및 커밋/예외 처리를 수행
        auto task = std::make_shared<std::packaged_task<bool()>>(
            [this, tx_id, service_name, args_vector,
            factory = std::forward<FactoryFunc>(context_factory),
            commit = std::forward<CommitFunc>(commit_action)]() mutable -> bool
            {
                // 컨텍스트 생성: factory는 unique_ptr을 반환해야 합니다.
                std::unique_ptr<ContextType> ctx = factory(tx_id);

                bool svc_exist = false;
                service_function svc;
                std::shared_ptr<mino::core::log::tinylog::logger> target_logger = nullptr;

                {
                    // 💡 공유 락을 통해 서비스와 전용 로거 맵을 안전하게 동시 획득 (Read 락)
                    std::shared_lock<std::shared_mutex> lock(service_mutex_);

                    auto it_svc = services_.find(service_name);
                    if (it_svc != services_.end()) {
                        svc_exist = true;
                        svc = it_svc->second;
                    }

                    auto it_log = ctx_loggers_.find(service_name);
                    if (it_log != ctx_loggers_.end()) {
                        target_logger = it_log->second;
                    }
                }

                // 💡 생성된 컨텍스트 객체에 전용 로거 주입 (없을 시 모니터 공통 로거 적용)
                if (ctx) {
                    if (target_logger) {
                        ctx->set_logger(target_logger);
                    }
                    else if (this->logger_) {
                        ctx->set_logger(this->logger_);
                    }
                }

                if (!svc_exist) {
                    if (logger_) logger_->error("[TX-{}] Error: Service routing failed for [{}]", tx_id, service_name);
                    return false;
                }

                bool success = false;
                try {
                    // 서비스 함수 호출
                    success = svc(*ctx, *args_vector);
                }
                catch (const std::exception& e) {
                    if (logger_) logger_->error("[TX-{}] Exception caught during service execution: {}", tx_id, e.what());
                    ctx->abort();
                }
                catch (...) {
                    if (logger_) logger_->error("[TX-{}] Unknown exception caught during service execution", tx_id);
                    ctx->abort();
                }

                // 서비스가 성공하고 트랜잭션이 중단되지 않았을 경우에만 커밋 실행
                if (success && !ctx->is_aborted) {
                    commit(*ctx);
                    return true;
                }
                else {
                    return false;
                }
            }
        );

        // future 획득 및 큐에 작업 등록
        std::future<bool> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            request_queue_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();

        return res;
    }

} 
