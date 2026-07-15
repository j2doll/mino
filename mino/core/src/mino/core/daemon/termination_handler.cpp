#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#   include <windows.h>
#else
#   include <csignal>
#   include <unistd.h>
#endif

#include "mino/core/daemon/daemon.hpp"

namespace mino::core::daemon {

    termination_handler& termination_handler::get_instance() {
        static termination_handler instance;
        return instance;
    }

    void termination_handler::set_callback(callback_t callback) {
        user_callback_ = std::move(callback);
    }

    void termination_handler::execute_callback() {
        auto& instance = get_instance();
        if (instance.user_callback_) {
            instance.user_callback_();
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    // Windows Console Control Handler
    int __stdcall termination_handler::windows_handler(unsigned long ctrl_type) {
        switch (ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
            execute_callback();
            return 1; // Signal handled successfully
        default:
            return 0;
        }
    }
#else
    // Linux POSIX Signal Handler
    void termination_handler::linux_handler(int signum) {
        if (signum == SIGINT || signum == SIGTERM) {
            execute_callback();
            std::exit(signum); // Explicit exit required in Linux signal handlers
        }
    }
#endif

    void termination_handler::initialize() {
#if defined(_WIN32) || defined(_WIN64)
        // Register Windows console handler
        SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(windows_handler), 1);
#else
        // Register Linux signal handler using sigaction
        struct sigaction action {};
        action.sa_handler = linux_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGINT, &action, nullptr);  // Ctrl+C
        sigaction(SIGTERM, &action, nullptr); // Termination request (e.g., kill)
#endif
    }

} // namespace mino::core::daemon