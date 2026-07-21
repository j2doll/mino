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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/dist_sink.h>

#if __has_include(<spdlog/pattern_formatter.h>)
#   include <spdlog/pattern_formatter.h>
#elif __has_include(<spdlog/details/pattern_formatter.h>)
#   include <spdlog/details/pattern_formatter.h>
#else
#   error "spdlog pattern_formatter header not found. Check your spdlog installation."
#endif

#include <nlohmann/json.hpp>

#include "mino/core/ini/ini_parser.hpp"

#include "mino/external/log/spd/auto_color_sink.hpp"
#include "mino/external/log/spd/encoding_file_logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/win_net_shim.hpp"
#include "mino/network/udp/udp_sender.hpp"

namespace mino::network::log::manager {

    class  hybrid_logger_manager {
    public:
        hybrid_logger_manager();
        ~hybrid_logger_manager();

        bool init(
            const std::string& defaultConfigPath,
            const std::string& sectionName,
            const std::string& loggerName,
            const std::string& envName = "");

        std::shared_ptr<::spdlog::logger> getLogger() const;

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
        ::spdlog::level::level_enum parseLevel(const std::string& s,
            ::spdlog::level::level_enum def) const;

        void checkDiskAndAct();
        bool sendUdpAlert(const std::string& msg);
        std::string buildUdpMessage(const std::string& tmpl,
            const std::string& path,
            unsigned long long availBytes,
            long double ratioPercent) const;
        static void replaceAll(std::string& s, const std::string& from, const std::string& to);

    protected:
        // INI/상태
        std::string iniPath_;
        std::string logSection_ = "Log";
        std::string loggerName_;
        mino::core::ini::ini_parser ini_;

        bool utcMode_ = false;

        bool enableConsole_ = true;
        bool enableFileAll_ = true;
        bool enableFileAlerts_ = true;

        ::spdlog::level::level_enum consoleMin_;
        ::spdlog::level::level_enum allFileMin_;
        ::spdlog::level::level_enum alertsMin_;
        ::spdlog::level::level_enum loggerMin_;
        ::spdlog::level::level_enum flushOn_;

        std::size_t flushEverySec_ = 1;

        std::string patternConsole_ = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v";
        std::string patternFile_ = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v";

        std::string allPath_ = "logs/all.log";
        std::string alertsPath_ = "logs/alerts.log";

        std::size_t allMaxSize_ = 100 * 1024 * 1024;
        std::size_t allMaxFiles_ = 5;
        std::size_t alertMaxSize_ = 100 * 1024 * 1024;
        std::size_t alertMaxFiles_ = 10;

        // rotating-zipping 추가 파라미터
        bool allDeleteOnFailure_ = false;
        int allCompressionLevel_ = 1;
        std::size_t allMaxZipCount_ = 5;
        mino::external::log::spd::time_zone_type allTimezone_ = mino::external::log::spd::time_zone_type::local_time;

        bool alertsDeleteOnFailure_ = false;
        int alertsCompressionLevel_ = 1;
        std::size_t alertsMaxZipCount_ = 5;
        mino::external::log::spd::time_zone_type alertsTimezone_ = mino::external::log::spd::time_zone_type::local_time;

        // disk guard
        bool        diskGuardEnable_ = true;
        std::string diskRoot_;
        double      diskMinFreeRatio_ = 5.0;

        // UDP alert
        std::string udpIp_;
        std::uint16_t udpPort_ = 0;
        unsigned    udpIntervalSec_ = 60;
        std::string udpMessageTmpl_ = "DISK LOW: path={path} free={avail_bytes}B ({ratio}%)";
        std::chrono::steady_clock::time_point lastUdpSent_{};

        bool fileSinksDetachedForDisk_ = false;

        // 파일 인코딩/라인엔딩/BOM 및 auto_color 키워드(JSON)
        mino::external::log::spd::log_encoding allEncoding_     = mino::external::log::spd::log_encoding::utf8;
        mino::external::log::spd::log_encoding alertsEncoding_  = mino::external::log::spd::log_encoding::utf8;
        mino::external::log::spd::line_ending allLineEnding_    = mino::external::log::spd::line_ending::lf;
        mino::external::log::spd::line_ending alertsLineEnding_ = mino::external::log::spd::line_ending::lf;
        bool allWriteBom_ = false;
        bool alertsWriteBom_ = false;

        std::string autoColorKeywordsJson_;

        // logger / sinks
        std::shared_ptr<::spdlog::logger> logger_;
        std::shared_ptr<mino::external::log::spd::auto_color_sink<std::mutex>> consoleSink_;
        std::shared_ptr<mino::external::log::spd::encoding_rotating_zipping_sink_mt> allSink_;
        std::shared_ptr<mino::external::log::spd::encoding_rotating_zipping_sink_mt> alertsSink_;
        std::shared_ptr<::spdlog::sinks::dist_sink_mt> distSink_;

        std::filesystem::file_time_type lastWriteTime_{};
        std::atomic<bool> autoReloadRunning_{ false };
        std::thread autoReloadThread_;
        unsigned autoReloadIntervalSec_{ 60 };
        mutable std::mutex mu_;
    };

}
