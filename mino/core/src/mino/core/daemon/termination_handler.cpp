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

    bool termination_handler::initialize() {
#if defined(_WIN32) || defined(_WIN64)
        // Windows: 성공 시 TRUE(0이 아닌 값), 실패 시 FALSE(0) 반환
        if (!SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(windows_handler), TRUE)) {
            return false;
        }
        return true;
#else
        // POSIX: sigaction 성공 시 0, 실패 시 -1 반환
        struct sigaction action {};
        action.sa_handler = linux_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (sigaction(SIGINT, &action, nullptr) != 0) {
            return false;
        }
        if (sigaction(SIGTERM, &action, nullptr) != 0) {
            // 필요에 따라 앞서 등록한 SIGINT를 SIG_DFL로 롤백할 수도 있습니다.
            return false;
        }
        return true;
#endif
    }

} // namespace mino::core::daemon
