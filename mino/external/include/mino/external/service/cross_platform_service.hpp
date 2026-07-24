#pragma once

#include <string>
#include <string_view>
#include <atomic>
#include <memory>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <csignal>
#endif

#include <spdlog/spdlog.h>

namespace mino::external::service {

    class  cross_platform_service {
    public:
        explicit cross_platform_service(std::wstring_view service_name_w, std::string_view service_name_a);
        virtual ~cross_platform_service() = default;

        void set_logger(std::shared_ptr<spdlog::logger> logger);
        bool run();

    protected:
        virtual void on_start() = 0;
        virtual void on_stop() = 0;
        virtual void on_pause() {}
        virtual void on_continue() {}

        void log_trace(std::string_view message);
        void log_debug(std::string_view message);
        void log_info(std::string_view message);
        void log_error(std::string_view message);
        void log_warn(std::string_view message);
        void log_critical(std::string_view message);

        bool is_running() const { return is_running_; }
        bool is_paused() const { return is_paused_; }
        void service_loop_delay(int milliseconds);

    protected:
        std::wstring service_name_w_;
        std::string service_name_a_;
        std::atomic<bool> is_running_{ false };
        std::atomic<bool> is_paused_{ false };

        std::shared_ptr<spdlog::logger> logger_{ nullptr };

    protected:
#ifdef _WIN32
        static void WINAPI service_main(DWORD argc, LPTSTR* argv);
        static DWORD WINAPI ctrl_handler(DWORD request, DWORD event_type, LPVOID event_data, LPVOID context);
        static inline cross_platform_service* instance_ = nullptr;
#else
        static void signal_handler(int signum);
        static inline cross_platform_service* instance_ = nullptr;
        void run_linux_loop();
#endif

    };

}