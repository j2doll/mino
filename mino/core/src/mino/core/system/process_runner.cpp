#include <thread>

#include "mino/core/system/process_runner.hpp"

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <signal.h>
#endif

namespace mino::core::system {

#ifdef _WIN32
    process_result process_runner::run_windows(const std::string& command, std::chrono::milliseconds timeout_ms) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        std::string mutable_command = command;

        if (!CreateProcessA(nullptr, mutable_command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            return { process_status::execution_failed, -1 };
        }

        DWORD wait_time = (timeout_ms == std::chrono::milliseconds(0)) ? INFINITE : static_cast<DWORD>(timeout_ms.count());
        DWORD wait_result = WaitForSingleObject(pi.hProcess, wait_time);

        process_result result;
        if (wait_result == WAIT_OBJECT_0) {
            DWORD exit_code_win;
            GetExitCodeProcess(pi.hProcess, &exit_code_win);
            result = { (exit_code_win == 0 ? process_status::success : process_status::abnormal_exit),
                       static_cast<int>(exit_code_win) };
        }
        else {
            TerminateProcess(pi.hProcess, 1);
            result = { process_status::timeout, -1 };
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return result;
    }
#else
    process_result process_runner::run_linux(const std::string& command, std::chrono::milliseconds timeout_ms) {
        pid_t pid = fork();
        if (pid == -1) return { process_status::execution_failed, -1 };

        if (pid == 0) {
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            _exit(127);
        }

        auto start_time = std::chrono::steady_clock::now();
        int status;

        while (true) {
            pid_t wait_res = waitpid(pid, &status, WNOHANG);

            if (wait_res == pid) {
                if (WIFEXITED(status)) {
                    int code = WEXITSTATUS(status);
                    return { (code == 0 ? process_status::success : process_status::abnormal_exit), code };
                }
                return { process_status::abnormal_exit, -1 };
            }

            if (timeout_ms > std::chrono::milliseconds(0)) {
                auto current_time = std::chrono::steady_clock::now();
                if ((current_time - start_time) > timeout_ms) {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    return { process_status::timeout, -1 };
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
#endif

    process_result process_runner::run_process(const std::string& command, std::chrono::milliseconds timeout_ms) {
#ifdef _WIN32
        return run_windows(command, timeout_ms);
#else
        return run_linux(command, timeout_ms);
#endif
    }

} 