#pragma once

#include <memory>
#include <type_traits>
#include <string>
#include <mutex>
#include <deque>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/fmt/fmt.h>

namespace mino::external::log::adapter {

    enum class level { trace, debug, info, warn, error, critical };

    class logger_adapter {
    public: 
        logger_adapter() = default;
        explicit logger_adapter(std::shared_ptr<::spdlog::logger> l);

        void set_logger(std::shared_ptr<::spdlog::logger> l) noexcept;

        // 직접 로깅: logger가 없으면 문자열로 포맷하여 버퍼에 저장
        template<typename Fmt, typename... Args>
        void log(level lvl, Fmt&& fmt_arg, Args&&... args) {
            // 안전하게 문자열로 포맷한다. 문자열 리터럴은 fmt::runtime으로 감싼다.
            std::string msg;
            if constexpr (std::is_convertible_v<std::decay_t<Fmt>, const char*>) {
#if defined(FMT_VERSION) && FMT_VERSION >= 80000
                // fmt v8.0.0 이상: fmt::runtime 사용 필수
                msg = fmt::format(fmt::runtime(fmt_arg), std::forward<Args>(args)...);
#else
                // fmt v7.x 이하: fmt::runtime이 없으므로 바로 전달
                msg = fmt::format(fmt_arg, std::forward<Args>(args)...);
#endif
            }
            else {
                msg = fmt::format(std::forward<Fmt>(fmt_arg), std::forward<Args>(args)...);
            }

            std::lock_guard<std::mutex> lk(mutex_);
            if (logger_) {
                dispatch_unlocked(lvl, msg);
            }
            else {
                buffer_.emplace_back(lvl, std::move(msg));
            }
        }

    private:
        void dispatch_unlocked(level lvl, const std::string& msg);

        std::shared_ptr<::spdlog::logger> logger_;
        std::mutex mutex_;
        std::deque<std::pair<level, std::string>> buffer_;
    };

    // 전역 어댑터 접근자 선언
    logger_adapter& global_logger_adapter() noexcept;

    // alias_fun: 전역 어댑터 또는 소유 어댑터에 바인딩 가능
    class alias_fun {
    public:
        // 전역 어댑터에 바인딩
        explicit alias_fun(level lvl) noexcept
            : lvl_(lvl), adapter_ptr_(&global_logger_adapter()) {
        }

        // 특정 logger에 바인딩된 소유 어댑터 생성
        alias_fun(level lvl, std::shared_ptr<::spdlog::logger> logger)
            : lvl_(lvl),
            owned_adapter_(std::make_shared<logger_adapter>(std::move(logger))),
            adapter_ptr_(owned_adapter_.get()) {
        }

        template<typename... Args>
        void operator()(Args&&... args) const {
            // 첫 인자가 포맷일 것이라 가정: forwarding 그대로 전달하면 adapter가 포맷 후 버퍼링/로그 처리
            adapter_ptr_->log(lvl_, std::forward<Args>(args)...);
        }

    private:
        level lvl_;
        std::shared_ptr<logger_adapter> owned_adapter_;
        logger_adapter* adapter_ptr_;
    };

    // 팩토리 및 헬퍼
    inline alias_fun make_alias(level lvl) noexcept { return alias_fun(lvl); }
    inline alias_fun make_alias_for_logger(level lvl, std::shared_ptr<::spdlog::logger> logger) {
        return alias_fun(lvl, std::move(logger));
    }

    // 전역 로거 설정 편의 함수
    inline void set_global_logger(std::shared_ptr<::spdlog::logger> l) noexcept {
        global_logger_adapter().set_logger(std::move(l));
    }
    inline void set_global_logger_by_name(const std::string& name) noexcept {
        set_global_logger(::spdlog::get(name));
    }

    // 매크로: 사용자 코드에서 간단하게 이름 정의 가능
#define MINO_DEFINE_LOG_ALIAS(name, lvl) \
    inline ::mino::external::log::alias_fun name{ ::mino::external::log::level::lvl }

#define MINO_DEFINE_LOG_ALIAS_FOR_LOGGER(name, lvl, logger_expr) \
    inline ::mino::external::log::alias_fun name = ::mino::external::log::make_alias_for_logger(::mino::external::log::level::lvl, (logger_expr))

}  
