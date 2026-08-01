#include <iostream>
#include <csignal>

#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/core/server/server_application.hpp"

#if defined(_WIN32) || defined(_WIN64)
#   define PLATFORM_WINDOWS
#   include <windows.h>
#   include <shellapi.h>
#else
#define PLATFORM_LINUX
#   include <unistd.h>
#endif

namespace mino::core::server {

    server_application::server_application()
        : is_requested_to_stop_(false), is_paused_(false), logger_(nullptr) {
        instance_ = this;
    }

    server_application::~server_application() {
        instance_ = nullptr;
    }

    void server_application::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        logger_ = logger;
    }

    int server_application::start(int argc, char** argv) {
        parse_arguments(argc, argv);

        try {
            pre_run();
            setup_signal_handlers();

            if (logger_) {
                logger_->info("Application has successfully completed pre_run and is starting up...");
            }

            return run(arguments_);
        }
        catch (const std::exception& e) {
            if (logger_) logger_->critical("Unhandled exception: {}", e.what());
            else std::cerr << "Unhandled exception: " << e.what() << std::endl;
            return 1;
        }
        catch (...) {
            if (logger_) logger_->critical("Unknown exception caught");
            else std::cerr << "Unknown exception caught" << std::endl;
            return 1;
        }
    }

    void server_application::terminate() {
        if (!is_requested_to_stop_.exchange(true)) {
            if (logger_) logger_->warn("Termination requested.");
            on_terminate();
            resume();
        }
    }

    void server_application::pause() {
        if (!is_paused_.exchange(true)) {
            if (logger_) logger_->warn("Application pausing...");
            on_pause();
        }
    }

    void server_application::resume() {
        if (is_paused_.exchange(false)) {
            if (logger_) logger_->info("Application resuming...");
            on_resume();
            pause_cv_.notify_all();
        }
    }

    bool server_application::is_cancelled() const {
        return is_requested_to_stop_;
    }

    bool server_application::is_paused() const {
        return is_paused_;
    }

    void server_application::pre_run() {}
    void server_application::on_pause() {}
    void server_application::on_resume() {}
    void server_application::on_terminate() {}

    void server_application::check_pause_status() {
        if (is_paused_) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this]() {
                return !is_paused_ || is_requested_to_stop_;
                });
        }
    }

    std::shared_ptr<mino::core::log::tinylog::logger> server_application::logger() {
        return logger_;
    }

    void server_application::parse_arguments(
#if defined(PLATFORM_WINDOWS)
        int,
        char** 
#else
        int argc,
        char** argv
#endif
        )
    {
        arguments_.clear();

#if defined(PLATFORM_WINDOWS)
        int win_argc = 0;
        LPWSTR* win_argv = ::CommandLineToArgvW(::GetCommandLineW(), &win_argc);

        if (win_argv != nullptr) {
            for (int i = 0; i < win_argc; ++i) {
                arguments_.push_back(wstring_to_utf8(win_argv[i]));
            }
            ::LocalFree(win_argv);
        }
#elif defined(PLATFORM_LINUX)
        for (int i = 0; i < argc; ++i) {
            arguments_.emplace_back(argv[i]);
        }
#endif
    }

#if defined(PLATFORM_WINDOWS)
    std::string server_application::wstring_to_utf8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = ::WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str_to(size_needed, 0);
        ::WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str_to[0], size_needed, NULL, NULL);
        return str_to;
    }
#endif

    void server_application::setup_signal_handlers() {
#if defined(PLATFORM_WINDOWS)
        ::SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(win32_ctrl_handler), TRUE);
#elif defined(PLATFORM_LINUX)
        struct sigaction sa;
        sa.sa_handler = posix_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGHUP, &sa, nullptr);
#endif
    }

#if defined(PLATFORM_WINDOWS)
    int server_application::win32_ctrl_handler(unsigned long ctrl_type) {
        if (instance_) {
            if (ctrl_type == 0 || ctrl_type == 2) { // CTRL_C_EVENT = 0, CTRL_CLOSE_EVENT = 2
                instance_->terminate();
                return 1;
            }
        }
        return 0;
    }
#elif defined(PLATFORM_LINUX)
    void server_application::posix_signal_handler(int signal) {
        if (!instance_)
            return;

        if (signal == SIGINT || signal == SIGTERM) {
            instance_->terminate();
        } else if (signal == SIGHUP) {
            if (instance_->is_paused()) {
                instance_->resume();
            } else {
                instance_->pause();
            }
        }
    }
#endif

} 
