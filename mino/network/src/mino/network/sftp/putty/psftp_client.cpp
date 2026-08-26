#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#endif

#include "mino/network/sftp/putty/psftp_client.hpp"

namespace mino::network::sftp::putty {

    // OS별 종속 데이터 정의
    struct psftp_client::platform_context {
#ifdef _WIN32
        HANDLE child_stdin_write = NULL;
        HANDLE child_stdout_read = NULL;
        PROCESS_INFORMATION proc_info;

        platform_context() {
            ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
        }
#else
        int pipe_stdin[2] = { -1, -1 };
        int pipe_stdout[2] = { -1, -1 };
        pid_t pid = -1;
#endif
    };

    psftp_client::psftp_client()
        : context_(std::make_unique<platform_context>()),
        error_patterns_({
            "unable to open",
            "cannot open",
            "no such file or directory",
            "permission denied",
            "fatal:",
            "error:",
            "connection lost",
            "access denied",
            "command not recognized"
            }) {
    }

    psftp_client::~psftp_client() {
        disconnect();
    }

    std::string psftp_client::to_lower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::string psftp_client::get_executable_dir() {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string path(buffer);
        size_t pos = path.find_last_of("\\/");
        return (pos != std::string::npos) ? path.substr(0, pos) : "";
#else
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
        if (count != -1) {
            std::string path(buffer, count);
            size_t pos = path.find_last_of("/");
            return (pos != std::string::npos) ? path.substr(0, pos) : "";
        }
        return ".";
#endif
    }

    std::vector<std::string> psftp_client::split_path(const std::string& path) {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string item;
        while (std::getline(ss, item, '/')) {
            if (!item.empty()) {
                parts.push_back(item);
            }
        }
        return parts;
    }

    bool psftp_client::connect(const std::string& host, int port, const std::string& user, const std::string& password_or_key_path, bool is_key, const std::string& hostkey_fingerprint) {
        std::string exe_dir = get_executable_dir();

#ifdef _WIN32
        SECURITY_ATTRIBUTES sa_attr;
        sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa_attr.bInheritHandle = TRUE;
        sa_attr.lpSecurityDescriptor = NULL;

        HANDLE child_stdin_read = NULL;
        HANDLE child_stdout_write = NULL;

        if (!CreatePipe(&context_->child_stdout_read, &child_stdout_write, &sa_attr, 0)) return false;
        SetHandleInformation(context_->child_stdout_read, HANDLE_FLAG_INHERIT, 0);

        if (!CreatePipe(&child_stdin_read, &context_->child_stdin_write, &sa_attr, 0)) return false;
        SetHandleInformation(context_->child_stdin_write, HANDLE_FLAG_INHERIT, 0);

        std::string psftp_path = exe_dir.empty() ? "psftp.exe" : (exe_dir + "\\psftp.exe");
        std::string cmd = "\"" + psftp_path + "\" " + host + " -P " + std::to_string(port) + " -l " + user + " -batch";

        // 호스트 키 핑거프린트 지정 시 옵션 추가
        if (hostkey_fingerprint.empty()) {
        } else {
            cmd += " -hostkey \"" + hostkey_fingerprint + "\"";
        }

        if (is_key) {
            cmd += " -i \"" + password_or_key_path + "\"";
        }
        else {
            cmd += " -pw \"" + password_or_key_path + "\"";
        }

        STARTUPINFOA start_info;
        ZeroMemory(&start_info, sizeof(STARTUPINFOA));
        start_info.cb = sizeof(STARTUPINFOA);
        start_info.hStdError = child_stdout_write;
        start_info.hStdOutput = child_stdout_write;
        start_info.hStdInput = child_stdin_read;
        start_info.dwFlags |= STARTF_USESTDHANDLES;

        std::vector<char> cmd_buffer(cmd.begin(), cmd.end());
        cmd_buffer.push_back('\0');

        BOOL success = CreateProcessA(NULL, cmd_buffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &start_info, &context_->proc_info);

        CloseHandle(child_stdout_write);
        CloseHandle(child_stdin_read);
        if (!success) {
            std::cerr << "[Error] Failed to execute psftp.exe at: " << psftp_path << std::endl;
            return false;
        }
#else
        if (pipe(context_->pipe_stdin) < 0 || pipe(context_->pipe_stdout) < 0) return false;

        context_->pid = fork();
        if (context_->pid < 0) return false;

        if (context_->pid == 0) {
            close(context_->pipe_stdin[1]);
            close(context_->pipe_stdout[0]);
            dup2(context_->pipe_stdin[0], STDIN_FILENO);
            dup2(context_->pipe_stdout[1], STDOUT_FILENO);
            dup2(context_->pipe_stdout[1], STDERR_FILENO);
            close(context_->pipe_stdin[0]);
            close(context_->pipe_stdout[1]);

            std::string psftp_path = exe_dir + "/psftp";
            std::string port_str = std::to_string(port);
            std::vector<const char*> args = { psftp_path.c_str(), host.c_str(), "-P", port_str.c_str(), "-l", user.c_str(), "-batch" };

            if (!hostkey_fingerprint.empty()) {
                args.push_back("-hostkey");
                args.push_back(hostkey_fingerprint.c_str());
            }

            if (is_key) {
                args.push_back("-i");
                args.push_back(password_or_key_path.c_str());
            }
            else {
                args.push_back("-pw");
                args.push_back(password_or_key_path.c_str());
            }
            args.push_back(nullptr);

            execv(psftp_path.c_str(), const_cast<char* const*>(args.data()));
            _exit(1);
        }

        close(context_->pipe_stdin[0]);
        close(context_->pipe_stdout[1]);
        fcntl(context_->pipe_stdout[0], F_SETFL, O_NONBLOCK);
#endif

        std::string init_response;
        return wait_for_prompt(init_response, nullptr, 15);
    }

    bool psftp_client::send_command(const std::string& command) {
        std::string full_cmd = command + "\n";
#ifdef _WIN32
        if (!context_->child_stdin_write) return false;
        DWORD bytes_written;
        return WriteFile(context_->child_stdin_write, full_cmd.c_str(), (DWORD)full_cmd.length(), &bytes_written, NULL);
#else
        if (context_->pipe_stdin[1] == -1) return false;
        return write(context_->pipe_stdin[1], full_cmd.c_str(), full_cmd.length()) > 0;
#endif
    }

    std::string psftp_client::read_output() {
        std::string result = "";
        char buffer[4096];
#ifdef _WIN32
        DWORD bytes_avail = 0, bytes_read = 0;
        while (PeekNamedPipe(context_->child_stdout_read, NULL, 0, NULL, &bytes_avail, NULL) && bytes_avail > 0) {
            if (ReadFile(context_->child_stdout_read, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0) {
                buffer[bytes_read] = '\0';
                result += buffer;
            }
        }
#else
        ssize_t bytes_read;
        while ((bytes_read = read(context_->pipe_stdout[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            result += buffer;
        }
#endif
        return result;
    }

    bool psftp_client::wait_for_prompt(std::string& out_response, progress_callback_t on_progress, int idle_timeout_seconds) {
        out_response.clear();
        auto last_active_time = std::chrono::steady_clock::now();
        std::regex percent_regex(R"((\d{1,3})%)");
        int last_percent = -1;

        while (true) {
            std::string chunk = read_output();

            // std::cout << chunk; // DEBUG

            if (!chunk.empty()) {

                // std::cout << chunk; // 실시간 터미널 출력 // DEBUG

                out_response += chunk;
                last_active_time = std::chrono::steady_clock::now();

                if (on_progress) {
                    std::sregex_iterator next(chunk.begin(), chunk.end(), percent_regex);
                    std::sregex_iterator end;
                    while (next != end) {
                        std::smatch match = *next;
                        int percent = std::stoi(match[1].str());
                        if (percent != last_percent && percent >= 0 && percent <= 100) {
                            last_percent = percent;
                            on_progress(percent);
                        }
                        next++;
                    }
                }

                if (out_response.find("psftp>") != std::string::npos) {
                    if (on_progress && last_percent != -1 && last_percent < 100) {
                        on_progress(100);
                    }
                    return true;
                }
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_active_time).count() > idle_timeout_seconds) {
                out_response += "\n[ERROR: Operation timed out due to inactivity]";
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    bool psftp_client::execute(
        const std::string& cmd,
        std::string& out_response,
        int idle_timeout_seconds)
    {
        return execute_with_progress(cmd, out_response, nullptr, idle_timeout_seconds);
    }

    bool psftp_client::execute_with_progress(
        const std::string& cmd,
        std::string& out_response,
        progress_callback_t on_progress,
        int idle_timeout_seconds)
    {
        read_output();

        if (!send_command(cmd)) {
            out_response = "[ERROR: Failed to write to process pipe]";
            return false;
        }

        if (!wait_for_prompt(out_response, on_progress, idle_timeout_seconds)) {
            return false;
        }

        std::string lower_resp = to_lower(out_response);

        // std::cout << lower_resp; // DEBUG

        for (const auto& pattern : error_patterns_) {
            if (lower_resp.find(pattern) != std::string::npos) {
                return false;
            }
        }

        return true;
    }

    bool psftp_client::ensure_remote_directory(const std::string& remote_path, int timeout_sec) {
        std::string response;

        auto mk_dir_string = "mkdir " + remote_path;
        if (!execute(mk_dir_string, response, timeout_sec)) {
            // return false; 
        }

        auto cd_dir_string = "cd " + remote_path;
        if (!execute(cd_dir_string, response, timeout_sec)) {
            return false;
        }

        return true; 
    }

    bool psftp_client::download_file(const std::string& remote_file, const std::string& local_file, progress_callback_t on_progress, bool resume, int idle_timeout) {
        std::string response;
        std::string cmd = (resume ? "reget \"" : "get \"") + remote_file + "\" \"" + local_file + "\"";

        bool success = execute_with_progress(cmd, response, on_progress, idle_timeout);
        if (!success && !resume) {
            std::remove(local_file.c_str());
        }
        return success;
    }

    void psftp_client::disconnect() {
        send_command("quit");
#ifdef _WIN32
        if (context_->child_stdin_write) { CloseHandle(context_->child_stdin_write); context_->child_stdin_write = NULL; }
        if (context_->child_stdout_read) { CloseHandle(context_->child_stdout_read); context_->child_stdout_read = NULL; }
        if (context_->proc_info.hProcess) {
            WaitForSingleObject(context_->proc_info.hProcess, 3000);
            CloseHandle(context_->proc_info.hProcess);
            CloseHandle(context_->proc_info.hThread);
            context_->proc_info.hProcess = NULL;
        }
#else
        if (context_->pipe_stdin[1] != -1) { close(context_->pipe_stdin[1]); context_->pipe_stdin[1] = -1; }
        if (context_->pipe_stdout[0] != -1) { close(context_->pipe_stdout[0]); context_->pipe_stdout[0] = -1; }
        if (context_->pid > 0) {
            int status;
            waitpid(context_->pid, &status, WNOHANG);
            context_->pid = -1;
        }
#endif
    }

} // namespace mino::network::sftp::putty
