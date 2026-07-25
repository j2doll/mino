#include "mino/core/tpm/tp_monitor.hpp"

namespace mino::core::tpm {

    transaction_context::transaction_context(uint64_t tx_id) : id(tx_id), logger_(nullptr) {}

    void transaction_context::abort() {
        is_aborted = true;
    }

    // void transaction_context::set_logger(std::shared_ptr<spdlog::logger> logger) {
    void transaction_context::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        logger_ = logger;
    }

    // std::shared_ptr<spdlog::logger> transaction_context::get_logger() const {
    std::shared_ptr<mino::core::log::tinylog::logger> transaction_context::get_logger() const {
        return logger_;
    }

    tp_monitor::tp_monitor() : stop_(false), tx_counter_(1000) {
        logger_ = nullptr;
    }

    tp_monitor::~tp_monitor() {
        if (logger_) logger_->info("Shutting down TP Monitor system cleanly...");
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true; // 💡 오타 수정 완료
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void tp_monitor::start_workers(size_t num_workers) {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (!workers_.empty()) {
            if (logger_) logger_->warn("Workers are already running. Ignore start_workers({}) request.", num_workers);
            return;
        }

        if (logger_) logger_->info("Initializing TP Monitor with {} workers via start_workers().", num_workers);

        for (size_t i = 0; i < num_workers; ++i) {
            workers_.emplace_back(&tp_monitor::worker_loop, this);
        }
    }

    void tp_monitor::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        if (logger) {
            logger_ = logger;
        }
    }

    std::shared_ptr<mino::core::log::tinylog::logger> tp_monitor::get_logger() const {
        return logger_;
    }

    void tp_monitor::register_service(const std::string& service_name, service_function func) {
        std::unique_lock<std::shared_mutex> lock(service_mutex_); // 💡 컴파일 에러 수정 완료
        services_[service_name] = func;
        if (logger_) logger_->info("Service [{}] registered successfully.", service_name);
    }

    // 💡 서비스별 개별 로거 추가 세팅 함수 구현 (Write 락)
    void tp_monitor::set_ctx_logger(std::string service_name, std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        if (!logger) return;

        std::unique_lock<std::shared_mutex> lock(service_mutex_);
        ctx_loggers_[service_name] = logger;
        if (logger_) logger_->info("Custom context logger registered for service [{}].", service_name);
    }

    void tp_monitor::worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this]() { return stop_ || !request_queue_.empty(); });

                if (stop_ && request_queue_.empty()) {
                    return;
                }

                task = std::move(request_queue_.front());
                request_queue_.pop();
            }
            task();
        }
    }

} 