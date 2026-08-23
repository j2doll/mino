
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/fmt/fmt.h>

#include "mino/external/log/adapter/logger_adapter.hpp"

namespace mino::external::log::adapter {

    logger_adapter::logger_adapter(std::shared_ptr<::spdlog::logger> l)
        : logger_(std::move(l)) {
    }

    void logger_adapter::set_logger(std::shared_ptr<::spdlog::logger> l) noexcept {
        std::lock_guard<std::mutex> lk(mutex_);
        logger_ = std::move(l);
        // flush buffer
        if (logger_) {
            for (auto& p : buffer_) {
                dispatch_unlocked(p.first, p.second);
            }
            buffer_.clear();
        }
    }

    void logger_adapter::dispatch_unlocked(level lvl, const std::string& msg) {
        // mutex는 호출자가 소유(또는 이미 잠금 해제된 상태)라고 가정
        switch (lvl) {
        case level::trace:    logger_->trace(msg); break;
        case level::debug:    logger_->debug(msg); break;
        case level::info:     logger_->info(msg); break;
        case level::warn:     logger_->warn(msg); break;
        case level::error:    logger_->error(msg); break;
        case level::critical: logger_->critical(msg); break;
        }
    }

    // 전역 어댑터 정의
    logger_adapter& global_logger_adapter() noexcept {
        static logger_adapter instance;
        return instance;
    }

} 
