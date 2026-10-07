#include "mino/core/subsystem/subsystem_master_channel.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace mino::core::subsystem {

    master_channel::master_channel() = default;

    master_channel::~master_channel() {
        close();
    }

    void master_channel::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        logger_ = std::move(logger);
    }

    void master_channel::on_event(event_callback cb) {
        event_callback_ = std::move(cb);
    }

    void master_channel::on_diagnostic_log(log_callback cb) {
        log_callback_ = std::move(cb);
    }

#if defined(_WIN32)
    bool master_channel::spawn(const std::string& executable_path) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE h_in_read = NULL, h_in_write = NULL;
        HANDLE h_out_read = NULL, h_out_write = NULL;
        HANDLE h_err_read = NULL, h_err_write = NULL;

        if (!CreatePipe(&h_in_read, &h_in_write, &sa, 0) ||
            !CreatePipe(&h_out_read, &h_out_write, &sa, 0) ||
            !CreatePipe(&h_err_read, &h_err_write, &sa, 0)) {
            return false;
        }

        SetHandleInformation(h_in_write, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(h_out_read, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(h_err_read, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = h_in_read;
        si.hStdOutput = h_out_write;
        si.hStdError = h_err_write;

        ZeroMemory(&proc_info_, sizeof(proc_info_));
        std::string cmd = executable_path;

        BOOL ok = CreateProcessA(
            nullptr, cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &proc_info_
        );

        CloseHandle(h_in_read);
        CloseHandle(h_out_write);
        CloseHandle(h_err_write);

        if (!ok) {
            CloseHandle(h_in_write);
            CloseHandle(h_out_read);
            CloseHandle(h_err_read);
            return false;
        }

        h_stdin_write_ = h_in_write;
        h_stdout_read_ = h_out_read;
        h_stderr_read_ = h_err_read;
        is_running_.store(true);

        start_stdout_monitor();
        start_stderr_monitor();
        return true;
    }

    bool master_channel::send_packet(const std::string& command, const std::string& payload) {
        if (!h_stdin_write_) return false;
        std::string raw_line = protocol::serialize(command, payload) + "\n";
        DWORD written = 0;
        return WriteFile(h_stdin_write_, raw_line.data(), static_cast<DWORD>(raw_line.size()), &written, nullptr) &&
            written == raw_line.size();
    }

    void master_channel::start_stdout_monitor() {
        stdout_thread_ = std::thread([this]() {
            char buf[512];
            DWORD bytes_read = 0;
            std::string acc;

            while (ReadFile(h_stdout_read_, buf, sizeof(buf) - 1, &bytes_read, nullptr) && bytes_read > 0) {
                buf[bytes_read] = '\0';
                acc += buf;

                size_t pos = 0;
                while ((pos = acc.find('\n')) != std::string::npos) {
                    std::string line = acc.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    acc.erase(0, pos + 1);

                    packet pkt;
                    if (protocol::deserialize(line, pkt)) {
                        // 워커가 직접 생성하여 보낸 비동기 이벤트인지 확인
                        if (pkt.command.rfind("EVENT:", 0) == 0) {
                            std::string event_name = pkt.command.substr(6);
                            if (event_callback_) {
                                event_callback_(event_name, pkt.payload);
                            }
                        }
                        else {
                            // 일반 RPC 응답인 경우 대기 큐에 삽입
                            std::lock_guard<std::mutex> lk(rpc_resp_mutex_);
                            rpc_responses_.push_back(pkt);
                            rpc_resp_cv_.notify_one();
                        }
                    }
                }
            }
            rpc_resp_cv_.notify_all();
            });
    }

    void master_channel::start_stderr_monitor() {
        stderr_thread_ = std::thread([this]() {
            char buf[512];
            DWORD bytes_read = 0;
            std::string err_acc;

            while (ReadFile(h_stderr_read_, buf, sizeof(buf) - 1, &bytes_read, nullptr) && bytes_read > 0) {
                buf[bytes_read] = '\0';
                err_acc += buf;

                size_t pos = 0;
                while ((pos = err_acc.find('\n')) != std::string::npos) {
                    std::string line = err_acc.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();

                    if (logger_) {
                        logger_->warn("<yellow>[WORKER-DIAGNOSTIC]</yellow> {}", line);
                    }
                    if (log_callback_) {
                        log_callback_(line);
                    }
                    err_acc.erase(0, pos + 1);
                }
            }
            });
    }

    void master_channel::close() {
        if (!is_running_.exchange(false)) return;

        if (h_stdin_write_) { CloseHandle(h_stdin_write_); h_stdin_write_ = NULL; }
        if (proc_info_.hProcess) {
            WaitForSingleObject(proc_info_.hProcess, 3000);
            CloseHandle(proc_info_.hProcess);
            CloseHandle(proc_info_.hThread);
            proc_info_.hProcess = NULL;
        }
        if (h_stdout_read_) { CloseHandle(h_stdout_read_); h_stdout_read_ = NULL; }
        if (h_stderr_read_) { CloseHandle(h_stderr_read_); h_stderr_read_ = NULL; }

        rpc_resp_cv_.notify_all();
        if (stdout_thread_.joinable()) stdout_thread_.join();
        if (stderr_thread_.joinable()) stderr_thread_.join();
    }

#else // POSIX (Linux)

    bool master_channel::spawn(const std::string& executable_path) {
        int in_pipe[2], out_pipe[2], err_pipe[2];

        if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
            return false;
        }

        pid_t pid = fork();
        if (pid < 0) return false;

        if (pid == 0) {
            dup2(in_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(err_pipe[1], STDERR_FILENO);

            ::close(in_pipe[0]);  ::close(in_pipe[1]);
            ::close(out_pipe[0]); ::close(out_pipe[1]);
            ::close(err_pipe[0]); ::close(err_pipe[1]);

            execl(executable_path.c_str(), executable_path.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }

        child_pid_ = pid;
        ::close(in_pipe[0]);
        ::close(out_pipe[1]);
        ::close(err_pipe[1]);

        fd_stdin_write_ = in_pipe[1];
        fd_stdout_read_ = out_pipe[0];
        fd_stderr_read_ = err_pipe[0];
        is_running_.store(true);

        start_stdout_monitor();
        start_stderr_monitor();
        return true;
    }

    bool master_channel::send_packet(const std::string& command, const std::string& payload) {
        if (fd_stdin_write_ < 0) return false;
        std::string raw_line = protocol::serialize(command, payload) + "\n";
        ssize_t written = write(fd_stdin_write_, raw_line.data(), raw_line.size());
        return written == static_cast<ssize_t>(raw_line.size());
    }

    void master_channel::start_stdout_monitor() {
        stdout_thread_ = std::thread([this]() {
            char buf[512];
            ssize_t bytes_read = 0;
            std::string acc;

            while ((bytes_read = read(fd_stdout_read_, buf, sizeof(buf) - 1)) > 0) {
                buf[bytes_read] = '\0';
                acc += buf;

                size_t pos = 0;
                while ((pos = acc.find('\n')) != std::string::npos) {
                    std::string line = acc.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    acc.erase(0, pos + 1);

                    packet pkt;
                    if (protocol::deserialize(line, pkt)) {
                        if (pkt.command.rfind("EVENT:", 0) == 0) {
                            std::string event_name = pkt.command.substr(6);
                            if (event_callback_) {
                                event_callback_(event_name, pkt.payload);
                            }
                        }
                        else {
                            std::lock_guard<std::mutex> lk(rpc_resp_mutex_);
                            rpc_responses_.push_back(pkt);
                            rpc_resp_cv_.notify_one();
                        }
                    }
                }
            }
            rpc_resp_cv_.notify_all();
            });
    }

    void master_channel::start_stderr_monitor() {
        stderr_thread_ = std::thread([this]() {
            char buf[512];
            ssize_t bytes_read = 0;
            std::string err_acc;

            while ((bytes_read = read(fd_stderr_read_, buf, sizeof(buf) - 1)) > 0) {
                buf[bytes_read] = '\0';
                err_acc += buf;

                size_t pos = 0;
                while ((pos = err_acc.find('\n')) != std::string::npos) {
                    std::string line = err_acc.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();

                    if (logger_) {
                        logger_->warn("<yellow>[WORKER-DIAGNOSTIC]</yellow> {}", line);
                    }
                    if (log_callback_) {
                        log_callback_(line);
                    }
                    err_acc.erase(0, pos + 1);
                }
            }
            });
    }

    void master_channel::close() {
        if (!is_running_.exchange(false)) return;

        if (fd_stdin_write_ >= 0) { ::close(fd_stdin_write_); fd_stdin_write_ = -1; }
        if (child_pid_ > 0) {
            int status = 0;
            waitpid(child_pid_, &status, 0);
            child_pid_ = -1;
        }
        if (fd_stdout_read_ >= 0) { ::close(fd_stdout_read_); fd_stdout_read_ = -1; }
        if (fd_stderr_read_ >= 0) { ::close(fd_stderr_read_); fd_stderr_read_ = -1; }

        rpc_resp_cv_.notify_all();
        if (stdout_thread_.joinable()) stdout_thread_.join();
        if (stderr_thread_.joinable()) stderr_thread_.join();
    }
#endif

    bool master_channel::call(const std::string& command, const std::string& request_payload,
        std::string& out_status, std::string& out_response_payload,
        std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> call_lock(rpc_call_mutex_);

        {
            std::lock_guard<std::mutex> resp_lock(rpc_resp_mutex_);
            rpc_responses_.clear();
        }

        if (!send_packet(command, request_payload)) {
            return false;
        }

        std::unique_lock<std::mutex> resp_lock(rpc_resp_mutex_);
        bool ok = rpc_resp_cv_.wait_for(resp_lock, timeout, [this] {
            return !rpc_responses_.empty() || !is_running_.load();
            });

        if (!ok || rpc_responses_.empty()) {
            if (logger_) {
                logger_->error("RPC call timed out or channel closed. Command: {}", command);
            }
            return false;
        }

        packet res_pkt = rpc_responses_.front();
        rpc_responses_.pop_front();

        out_status = res_pkt.command;
        out_response_payload = res_pkt.payload;
        return true;
    }

} // namespace mino::core::subsystem
