#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/types.h>
#endif

#include "mino/core/log/tinylog/tinylog_fwd.hpp"
#include "mino/core/subsystem/subsystem_protocol.hpp"

namespace mino::core::log::tinylog {
    class logger;
}

namespace mino::core::subsystem {

    class master_channel {
    public:
        using log_callback = std::function<void(const std::string& log_line)>;
        // 워커가 직접 생성하여 보낸 비동기 이벤트를 수신하는 콜백
        using event_callback = std::function<void(const std::string& event_name, const std::string& payload)>;

        master_channel();
        ~master_channel();

        master_channel(const master_channel&) = delete;
        master_channel& operator=(const master_channel&) = delete;

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger);
        bool spawn(const std::string& executable_path);

        // 동기 RPC 요청/응답 (워커의 비동기 이벤트와 간섭 없이 독립 동작)
        bool call(const std::string& command, const std::string& request_payload,
            std::string& out_status, std::string& out_response_payload,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

        bool send_packet(const std::string& command, const std::string& payload);

        // 워커가 생성한 자율 이벤트 수신 핸들러 등록
        void on_event(event_callback cb);

        // 워커 진단 로그(stderr) 수신 핸들러 등록
        void on_diagnostic_log(log_callback cb);

        void close();

    private:
        void start_stdout_monitor();
        void start_stderr_monitor();

#if defined(_WIN32)
        HANDLE h_stdin_write_{ NULL };
        HANDLE h_stdout_read_{ NULL };
        HANDLE h_stderr_read_{ NULL };
        PROCESS_INFORMATION proc_info_{};
#else
        int fd_stdin_write_{ -1 };
        int fd_stdout_read_{ -1 };
        int fd_stderr_read_{ -1 };
        pid_t child_pid_{ -1 };
#endif

        std::thread stdout_thread_;
        std::thread stderr_thread_;
        std::atomic<bool> is_running_{ false };

        // RPC 응답 대기용 동기화 객체
        std::mutex rpc_call_mutex_;
        std::mutex rpc_resp_mutex_;
        std::condition_variable rpc_resp_cv_;
        std::deque<packet> rpc_responses_;

        event_callback event_callback_{ nullptr };
        log_callback log_callback_{ nullptr };
        std::shared_ptr<mino::core::log::tinylog::logger> logger_{ nullptr };
    };

} // namespace mino::core::subsystem
