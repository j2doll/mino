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

#include "mino/core/ini/ini.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/udp/udp_sender.hpp"

namespace mino::network::log::manager {

    // tinylog 싱크별 레벨 필터링 및 동적 교체/On-Off를 지원하는 래퍼 싱크
    class filter_sink : public mino::core::log::tinylog::sink {
    public:
        filter_sink(const std::string& name,
            std::shared_ptr<mino::core::log::tinylog::sink> target,
            mino::core::log::tinylog::log_level min_level,
            bool enabled = true)
            : mino::core::log::tinylog::sink(name),
            target_(std::move(target)),
            min_level_(min_level),
            enabled_(enabled) {
        }

        void set_target(std::shared_ptr<mino::core::log::tinylog::sink> target) {
            std::lock_guard<std::mutex> lock(mu_);
            target_ = std::move(target);
        }

        std::shared_ptr<mino::core::log::tinylog::sink> get_target() const {
            std::lock_guard<std::mutex> lock(mu_);
            return target_;
        }

        void set_level(mino::core::log::tinylog::log_level level) {
            std::lock_guard<std::mutex> lock(mu_);
            min_level_ = level;
        }

        mino::core::log::tinylog::log_level level() const {
            std::lock_guard<std::mutex> lock(mu_);
            return min_level_;
        }

        void set_enabled(bool enabled) {
            std::lock_guard<std::mutex> lock(mu_);
            enabled_ = enabled;
        }

        bool is_enabled() const {
            std::lock_guard<std::mutex> lock(mu_);
            return enabled_;
        }

        void log(mino::core::log::tinylog::log_level level, std::string_view msg) override {
            std::lock_guard<std::mutex> lock(mu_);
            if (!enabled_ || level < min_level_) return;
            if (target_) {
                target_->log(level, msg);
            }
        }

    private:
        mutable std::mutex mu_;
        std::shared_ptr<mino::core::log::tinylog::sink> target_;
        mino::core::log::tinylog::log_level min_level_{ mino::core::log::tinylog::log_level::trace };
        bool enabled_{ true };
    };

    class hybrid_logger_manager {
    public:
        hybrid_logger_manager();
        ~hybrid_logger_manager();

        bool init(
            const std::string& defaultConfigPath,
            const std::string& sectionName,
            const std::string& loggerName,
            const std::string& envName = "");

        // tinylog 로거 인스턴스 반환
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
            std::size_t old_alertMaxFiles,
            mino::core::log::tinylog::encoding_type old_consoleEncoding,
            mino::core::log::tinylog::encoding_type old_allEncoding,
            mino::core::log::tinylog::encoding_type old_alertsEncoding,
            mino::core::log::tinylog::eol_type old_allLineEnding,
            mino::core::log::tinylog::eol_type old_alertsLineEnding);

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
        // INI 및 상태
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

        // Disk Guard
        bool        diskGuardEnable_ = true;
        std::string diskRoot_;
        double      diskMinFreeRatio_ = 5.0;

        // UDP Alert
        std::string udpIp_;
        std::uint16_t udpPort_ = 0;
        unsigned    udpIntervalSec_ = 60;
        std::string udpMessageTmpl_ = "DISK LOW: path={path} free={avail_bytes}B ({ratio}%)";
        std::chrono::steady_clock::time_point lastUdpSent_{};

        bool fileSinksDetachedForDisk_ = false;

        // 인코딩 및 개행 설정
        mino::core::log::tinylog::encoding_type consoleEncoding_ = mino::core::log::tinylog::encoding_type::utf8;
        mino::core::log::tinylog::encoding_type allEncoding_ = mino::core::log::tinylog::encoding_type::utf8;
        mino::core::log::tinylog::encoding_type alertsEncoding_ = mino::core::log::tinylog::encoding_type::utf8;
        mino::core::log::tinylog::eol_type      allLineEnding_ = mino::core::log::tinylog::eol_type::lf;
        mino::core::log::tinylog::eol_type      alertsLineEnding_ = mino::core::log::tinylog::eol_type::lf;

        // tinylog 로거 및 래퍼 싱크
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
