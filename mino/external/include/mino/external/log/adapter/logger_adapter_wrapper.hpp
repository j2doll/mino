#pragma once

#include <memory>
#include <string>
#include <utility>

namespace mino::external::log::adapter {

    enum class log_level { trace, debug, info, warn, error, critical };

    class logger_adapter_wrapper {
    public:
        logger_adapter_wrapper();
        explicit logger_adapter_wrapper(const std::string& logger_name);
        ~logger_adapter_wrapper();

        // 복사 불가능, 이동 가능
        logger_adapter_wrapper(const logger_adapter_wrapper&) = delete;
        logger_adapter_wrapper& operator=(const logger_adapter_wrapper&) = delete;
        logger_adapter_wrapper(logger_adapter_wrapper&&) noexcept;
        logger_adapter_wrapper& operator=(logger_adapter_wrapper&&) noexcept;

        // 템플릿 인라인 구현부에서 impl로 인자를 그대로 전달
        template <typename Fmt, typename... Args>
        void log(log_level lvl, Fmt&& fmt_arg, Args&&... args);

        // 단순 문자열 출력용 (포맷팅이 필요 없는 경우)
        void log_string(log_level lvl, const std::string& msg);

        // 전역 로거 설정 래퍼
        static void set_global_logger_by_name(const std::string& name);

    private:
        class impl; // Pimpl 패턴 (전방 선언)
        std::unique_ptr<impl> impl_;
    };

} // namespace mino::external::log::adapter

// 헤더 외부 구현 (fmt 헤더 없이 pimpl로 전달만 수행)
namespace mino::external::log::adapter {

    template <typename Fmt, typename... Args>
    inline void logger_adapter_wrapper::log(log_level lvl, Fmt&& fmt_arg, Args&&... args) {
        if (impl_) {
            // 실제 fmt::format 및 logger_adapter 호출은 .cpp 내부의 impl이 담당합니다.
            impl_log_bridge(lvl, std::forward<Fmt>(fmt_arg), std::forward<Args>(args)...);
        }
    }

}
