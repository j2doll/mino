#include "mino/external/log/adapter/logger_adapter_wrapper.hpp"
#include "mino/external/log/adapter/logger_adapter.hpp" // 사용자 작성 어댑터 헤더

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h> // fmt 헤더는 오직 .cpp 안에서만 include 됩니다.

namespace mino::external::log::adapter {

    // Wrapper 레벨 -> 원본 level 변환
    static level to_adapter_level(log_level lvl) {
        switch (lvl) {
        case log_level::trace:    return level::trace;
        case log_level::debug:    return level::debug;
        case log_level::info:     return level::info;
        case log_level::warn:     return level::warn;
        case log_level::error:    return level::error;
        case log_level::critical: return level::critical;
        }
        return level::info;
    }
     
    // Pimpl 구현
    class logger_adapter_wrapper::impl {
    public:
        impl() : adapter_() {}

        explicit impl(const std::string& logger_name) {
            auto spd_logger = ::spdlog::get(logger_name);
            if (spd_logger) {
                adapter_.set_logger(spd_logger);
            }
        }

        // fmt::format 및 원래 어댑터의 log 함수를 호출하는 실제 구현부
        template <typename Fmt, typename... Args>
        void log(log_level lvl, Fmt&& fmt_arg, Args&&... args) {
            adapter_.log(to_adapter_level(lvl), std::forward<Fmt>(fmt_arg), std::forward<Args>(args)...);
        }

        void log_string(log_level lvl, const std::string& msg) {
            adapter_.log(to_adapter_level(lvl), "{}", msg);
        }

    private:
        logger_adapter adapter_;
    };

    // logger_adapter_wrapper 기본 멤버 구현
    logger_adapter_wrapper::logger_adapter_wrapper()
        : impl_(std::make_unique<impl>()) {
    }

    logger_adapter_wrapper::logger_adapter_wrapper(const std::string& logger_name)
        : impl_(std::make_unique<impl>(logger_name)) {
    }

    logger_adapter_wrapper::~logger_adapter_wrapper() = default;

    logger_adapter_wrapper::logger_adapter_wrapper(logger_adapter_wrapper&&) noexcept = default;
    logger_adapter_wrapper& logger_adapter_wrapper::operator=(logger_adapter_wrapper&&) noexcept = default;

    void logger_adapter_wrapper::log_string(log_level lvl, const std::string& msg) {
        if (impl_) {
            impl_->log_string(lvl, msg);
        }
    }

    void logger_adapter_wrapper::set_global_logger_by_name(const std::string& name) {
        ::mino::external::log::adapter::set_global_logger_by_name(name);
    }

} // namespace mino::external::log::adapter
