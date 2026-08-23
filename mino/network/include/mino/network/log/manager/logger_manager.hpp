#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <string_view>
#include <cstdlib>
#include <cctype>
#include <cmath>

#include "mino/core/ini/ini_parser.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/udp/udp_sender.hpp"
#include "mino/network/log/manager/filter_sink.hpp"

// INI 기반 tinylog 구성/리로드/디스크 감시/UDP 알림을 제공하는 기본 로거 매니저
namespace mino::network::log::manager {

    class logger_manager {
    public:
        logger_manager();
        ~logger_manager();

        bool init(
            const std::string& defaultConfigPath,
            const std::string& sectionName,
            const std::string& loggerName,
            const std::string& envName = "");

        std::shared_ptr<mino::core::log::tinylog::logger> getLogger() const;

        bool reloadIfChanged();

        bool startAutoReload(unsigned interval_sec = 60);

        void stopAutoReload();

    protected:
        bool loadConfig(bool readAutoReload);
        void applySoftSettings();
        void applyHardSettingsIfNeeded(
            bool old_enableConsole,
            bool old_enableFileAll,
            bool old_enableFileAlerts,
            const std::string& old_allPath,
            const std::string& old_alertsPath,
            std::size_t old_allMaxSize,
            std::size_t old_allMaxFiles,
            std::size_t old_alertMaxSize,
            std::size_t old_alertMaxFiles);
        static void ensureParentDir(const std::string& path);
        bool toBool(const std::string& val, bool default_val) const;
        std::string toLower(const std::string& s) const;
        std::size_t parseSizeBytes(const std::string& s, std::size_t default_val) const;
        mino::core::log::tinylog::log_level parseLevel(const std::string& s,
            mino::core::log::tinylog::log_level def) const;

        void checkDiskAndAct();
        bool sendUdpAlert(const std::string& msg);
        std::string buildUdpMessage(const std::string& tmpl,
            const std::string& path,
            unsigned long long availBytes,
            long double ratioPercent) const;
        static void replaceAll(std::string& s, const std::string& from, const std::string& to);

    protected:
        std::string iniPath_;
        std::string logSection_ = "Log";
        std::string loggerName_;
        mino::core::ini::ini_parser ini_;

        bool utcMode_ = false;

        bool enableConsole_ = true;
        bool enableFileAll_ = true;
        bool enableFileAlerts_ = true;

        mino::core::log::tinylog::log_level consoleMin_;
        mino::core::log::tinylog::log_level allFileMin_;
        mino::core::log::tinylog::log_level alertsMin_;
        mino::core::log::tinylog::log_level loggerMin_;

        std::string allPath_ = "logs/all.log";
        std::string alertsPath_ = "logs/alerts.log";

        std::size_t allMaxSize_ = 100 * 1024 * 1024;
        std::size_t allMaxFiles_ = 5;
        std::size_t alertMaxSize_ = 100 * 1024 * 1024;
        std::size_t alertMaxFiles_ = 10;

        // 디스크 감시
        bool        diskGuardEnable_ = true;
        std::string diskRoot_;
        double      diskMinFreeRatio_ = 5.0;

        // UDP 알림
        std::string udpIp_;
        std::uint16_t udpPort_ = 0;
        unsigned    udpIntervalSec_ = 60;
        std::string udpMessageTmpl_ = "DISK LOW: path={path} free={avail_bytes}B ({ratio}%)";
        std::chrono::steady_clock::time_point lastUdpSent_{};

        bool fileSinksDetachedForDisk_ = false;

        // 로거 및 래퍼 싱크
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;
        std::shared_ptr<filter_sink> consoleSinkWrapper_;
        std::shared_ptr<filter_sink> allSinkWrapper_;
        std::shared_ptr<filter_sink> alertsSinkWrapper_;

        std::filesystem::file_time_type lastWriteTime_{};
        std::atomic<bool> autoReloadRunning_{ false };
        std::thread autoReloadThread_;
        unsigned autoReloadIntervalSec_{ 60 };
        mutable std::mutex mu_;
    };

} // namespace mino::network::log::manager
