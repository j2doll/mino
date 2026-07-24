#include <iostream>
#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>

#include "mino/external/service/cross_platform_service.hpp"

#ifdef _WIN32
    #include <windows.h>
    SERVICE_STATUS g_service_status = { 0 };
    SERVICE_STATUS_HANDLE g_status_handle = nullptr;
    HANDLE g_service_stop_event = INVALID_HANDLE_VALUE;
#else
    #include <csignal>
#endif

namespace mino::external::service {

    cross_platform_service::cross_platform_service(std::wstring_view service_name_w, std::string_view service_name_a)
        : service_name_w_(service_name_w), service_name_a_(service_name_a) {
        instance_ = this;
    }

    void cross_platform_service::set_logger(std::shared_ptr<spdlog::logger> logger) {
        logger_ = logger;
    }

    void cross_platform_service::log_trace(std::string_view message) {
        if (logger_) {
            logger_->trace(message);
        }
    }

    void cross_platform_service::log_debug(std::string_view message) {
        if (logger_) {
            logger_->debug(message);
        }
    }

    void cross_platform_service::log_info(std::string_view message) {
        if (logger_) {
            logger_->info(message);
        }
    }

    void cross_platform_service::log_error(std::string_view message) {
        if (logger_) {
            logger_->error(message);
        }
    }

    void cross_platform_service::log_warn(std::string_view message) {
        if (logger_) {
            logger_->warn(message);
        }
    }

    void cross_platform_service::log_critical(std::string_view message) {
        if (logger_) {
            logger_->critical(message);
        }
    }

    void cross_platform_service::service_loop_delay(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    bool cross_platform_service::run() {
#ifdef _WIN32
        SERVICE_TABLE_ENTRYW service_table[] = {
            { const_cast<LPWSTR>(service_name_w_.c_str()), (LPSERVICE_MAIN_FUNCTIONW)service_main },
            { nullptr, nullptr }
        };
        return StartServiceCtrlDispatcherW(service_table) != 0;
#else
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGCONT, signal_handler);

        is_running_ = true;
        on_start();
        run_linux_loop();
        return true;
#endif
    }

    // ==========================================
    // WINDOWS (NT 서비스) 구현부
    // ==========================================
#ifdef _WIN32
    DWORD WINAPI cross_platform_service::ctrl_handler(
        DWORD request,
        DWORD , // event_type,
        LPVOID , // event_data,
        LPVOID ) // context)
    {
        if (!instance_)
            return ERROR_CALL_NOT_IMPLEMENTED;

        switch (request) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            instance_->is_running_ = false;
            g_service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_status_handle, &g_service_status);
            instance_->on_stop();
            SetEvent(g_service_stop_event);
            return NO_ERROR;

        case SERVICE_CONTROL_PAUSE:
            instance_->is_paused_ = true;
            g_service_status.dwCurrentState = SERVICE_PAUSED;
            SetServiceStatus(g_status_handle, &g_service_status);
            instance_->on_pause();
            return NO_ERROR;

        case SERVICE_CONTROL_CONTINUE:
            instance_->is_paused_ = false;
            g_service_status.dwCurrentState = SERVICE_RUNNING;
            SetServiceStatus(g_status_handle, &g_service_status);
            instance_->on_continue();
            return NO_ERROR;

        default:
            break;
        }
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    void WINAPI cross_platform_service::service_main(
        DWORD , // argc,
        LPTSTR* // argv
    )
    {
        if (!instance_) return;

        g_status_handle = RegisterServiceCtrlHandlerExW(instance_->service_name_w_.c_str(), ctrl_handler, nullptr);
        if (!g_status_handle) return;

        g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_service_status.dwCurrentState = SERVICE_START_PENDING;
        g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;

        g_service_status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus(g_status_handle, &g_service_status);

        instance_->is_running_ = true;
        instance_->on_start();

        g_service_stop_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        WaitForSingleObject(g_service_stop_event, INFINITE);

        CloseHandle(g_service_stop_event);
        g_service_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_status_handle, &g_service_status);
    }

    // ==========================================
    // LINUX (Rocky, Ubuntu) 구현부
    // ==========================================
#else
    void cross_platform_service::signal_handler(int signum) {
        if (!instance_) return;

        switch (signum) {
        case SIGTERM:
            instance_->is_running_ = false;
            instance_->on_stop();
            break;
        case SIGCONT:
            instance_->is_paused_ = false;
            instance_->on_continue();
            break;
        default:
            break;
        }
    }

    void cross_platform_service::run_linux_loop() {
        while (is_running_) {
            if (!is_paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
#endif

}