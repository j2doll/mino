#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <mutex>

#include "mino/core/log/tinylog/logger.hpp"

namespace mino::network::log::manager {

    // tinylog 싱크별 레벨 필터링 및 동적 교체/On-Off를 지원하는 공통 래퍼 싱크
    class filter_sink : public mino::core::log::tinylog::sink {
    public:
        filter_sink(const std::string& name,
            std::shared_ptr<mino::core::log::tinylog::sink> target,
            mino::core::log::tinylog::log_level min_level,
            bool enabled = true)
            : mino::core::log::tinylog::sink(name),
            target_(std::move(target)),
            min_level_(min_level),
            enabled_(enabled) {
        }

        void set_target(std::shared_ptr<mino::core::log::tinylog::sink> target) {
            std::lock_guard<std::mutex> lock(mu_);
            target_ = std::move(target);
        }

        std::shared_ptr<mino::core::log::tinylog::sink> get_target() const {
            std::lock_guard<std::mutex> lock(mu_);
            return target_;
        }

        void set_level(mino::core::log::tinylog::log_level level) {
            std::lock_guard<std::mutex> lock(mu_);
            min_level_ = level;
        }

        mino::core::log::tinylog::log_level level() const {
            std::lock_guard<std::mutex> lock(mu_);
            return min_level_;
        }

        void set_enabled(bool enabled) {
            std::lock_guard<std::mutex> lock(mu_);
            enabled_ = enabled;
        }

        bool is_enabled() const {
            std::lock_guard<std::mutex> lock(mu_);
            return enabled_;
        }

        void log(mino::core::log::tinylog::log_level level, std::string_view msg) override {
            std::lock_guard<std::mutex> lock(mu_);
            if (!enabled_ || level < min_level_) return;
            if (target_) {
                target_->log(level, msg);
            }
        }

    private:
        mutable std::mutex mu_;
        std::shared_ptr<mino::core::log::tinylog::sink> target_;
        mino::core::log::tinylog::log_level min_level_{ mino::core::log::tinylog::log_level::trace };
        bool enabled_{ true };
    };

} // namespace mino::network::log::manager
