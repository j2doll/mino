#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>  
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/dist_sink.h>

// #include <spdlog/pattern_formatter.h>
#if __has_include(<spdlog/pattern_formatter.h>)
// 최신 spdlog (공개 헤더)
#   include <spdlog/pattern_formatter.h>
#   elif __has_include(<spdlog/details/pattern_formatter.h>)
// 구버전 spdlog (details 아래에만 존재)
#   include <spdlog/details/pattern_formatter.h>
#else
#   error "spdlog pattern_formatter header not found. Check your spdlog installation."
#endif

#include "mino/network/udp/udp_sender.hpp"

#include "mino/network/log/manager/logger_manager.hpp"

namespace mino::network::log::manager {

// spdlog 버전에 따라 custom_flag_formatter / add_flag 지원 여부를 판별
// Rocky 8의 구버전 spdlog(1.5.x)는 이 기능이 없으므로 컴파일 시 분기 처리
#ifndef SPDLOG_VER_MAJOR
#  define MINO_SPDLOG_NO_CUSTOM_FLAG 1
#elif (SPDLOG_VER_MAJOR * 10000 + SPDLOG_VER_MINOR * 100 + SPDLOG_VER_PATCH) < 10800
#  define MINO_SPDLOG_NO_CUSTOM_FLAG 1
#else
#  define MINO_SPDLOG_HAS_CUSTOM_FLAG 1
#endif

// %Z 자리(패턴 문자열 내)를 "utc"/"local" 텍스트로 치환하는 헬퍼
// 구버전 spdlog에서는 custom_flag_formatter를 사용할 수 없으므로
// 패턴 문자열 자체에서 "%Z"를 고정 문자열로 바꿔서 사용한다.
    inline std::string replace_Z_placeholder(const std::string& pattern, bool utc)
    {
        std::string result = pattern;
        const std::string tz = utc ? "utc" : "local";
        std::string::size_type pos = 0;
        while ((pos = result.find("%Z", pos)) != std::string::npos) {
            result.replace(pos, 2, tz);
            pos += tz.size();
        }
        return result;
    }

    // %Z 플래그: TIME_MODE에 따라 "utc" 또는 "local"을 고정폭(기본 5)으로 출력
    namespace {
#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
        class TzFlag : public spdlog::custom_flag_formatter {
        public:
            explicit TzFlag(bool utc, std::size_t width = 5) : utc_(utc), width_(width) {}

            void format(const spdlog::details::log_msg&,
                const std::tm&,
                spdlog::memory_buf_t& dest) override
            {
                const char* s = utc_ ? "utc" : "local";
                fmt::format_to(fmt::appender(dest), "{:<{}}", s, width_);
            }

            std::unique_ptr<spdlog::custom_flag_formatter> clone() const override {
                return spdlog::details::make_unique<TzFlag>(utc_, width_);
            }

        private:
            bool utc_{ false };
            std::size_t width_{ 5 };
        };
#endif 
    } // anonymous namespace

    logger_manager::logger_manager() {
        consoleMin_ = spdlog::level::trace;
        allFileMin_ = spdlog::level::trace;
        alertsMin_  = spdlog::level::warn;
        loggerMin_  = spdlog::level::trace;
        flushOn_    = spdlog::level::warn;
    }

    logger_manager::~logger_manager() {
        stopAutoReload();
    }

    // init에서 락을 해제한 뒤 start/stopAutoReload를 호출하여 교착 방지
    bool logger_manager::init(const std::string& defaultConfigPath,
        const std::string& sectionName,
        const std::string& loggerName,
        const std::string& envName)
    {
        unsigned interval_to_start = 0;
        bool need_start = false;

        {
            std::lock_guard<std::mutex> lk(mu_);

            loggerName_ = loggerName;
            logSection_ = sectionName;

            if (!envName.empty()) {
                if (const char* envPath = std::getenv(envName.c_str())) {
                    iniPath_ = envPath;
                    std::cout << "[logger_manager] Using config from " << envName << ": " << iniPath_ << "\n";
                }
                else {
                    iniPath_ = defaultConfigPath;
                    std::cout << "[logger_manager] Env var " << envName << " not set, using default: " << iniPath_ << "\n";
                }
            }
            else {
                iniPath_ = defaultConfigPath;
                std::cout << "[logger_manager] Using default config path: " << iniPath_ << "\n";
            }

            // ini_.SetUnicode();
            // ini_.SetMultiKey(false);

            if (!loadConfig(true)) {
                std::cerr << "[logger_manager] Failed to load config.\n";
                return false;
            }

            try {
                lastWriteTime_ = std::filesystem::last_write_time(iniPath_);
            }
            catch (...) {
                lastWriteTime_ = std::filesystem::file_time_type{};
            }

            auto time_type = utcMode_ ? spdlog::pattern_time_type::utc
                : spdlog::pattern_time_type::local;

#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
            auto console_fmt = std::make_unique<spdlog::pattern_formatter>(patternConsole_, time_type);
            auto file_fmt = std::make_unique<spdlog::pattern_formatter>(patternFile_, time_type);

            // %Z 플래그 등록(utc/local 고정폭 출력)
            console_fmt->add_flag<TzFlag>('Z', utcMode_);
            file_fmt->add_flag<TzFlag>('Z', utcMode_);
#else
            // 구버전 spdlog(예: Rocky 8 기본 패키지)에서는 custom_flag_formatter를
            // 지원하지 않으므로, 패턴 문자열 내 "%Z"를 "utc"/"local" 텍스트로 치환하여 사용
            auto console_pattern = replace_Z_placeholder(patternConsole_, utcMode_);
            auto file_pattern = replace_Z_placeholder(patternFile_, utcMode_);
            auto console_fmt = std::make_unique<spdlog::pattern_formatter>(console_pattern, time_type);
            auto file_fmt = std::make_unique<spdlog::pattern_formatter>(file_pattern, time_type);
#endif

            distSink_ = std::make_shared<spdlog::sinks::dist_sink_mt>();

            if (enableConsole_) {
                consoleSink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                consoleSink_->set_level(consoleMin_);
                consoleSink_->set_formatter(console_fmt->clone());
                distSink_->add_sink(consoleSink_);
            }

            if (enableFileAll_) {
                ensureParentDir(allPath_);
                allSink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    allPath_, allMaxSize_, allMaxFiles_, false);
                allSink_->set_level(allFileMin_);
                allSink_->set_formatter(file_fmt->clone());
                distSink_->add_sink(allSink_);
            }

            if (enableFileAlerts_) {
                ensureParentDir(alertsPath_);
                alertsSink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    alertsPath_, alertMaxSize_, alertMaxFiles_, false);
                alertsSink_->set_level(alertsMin_);
                alertsSink_->set_formatter(file_fmt->clone());
                distSink_->add_sink(alertsSink_);
            }

            if (distSink_->sinks().empty()) {
                auto fallback = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                fallback->set_level(spdlog::level::trace);
#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
                auto fallback_fmt = std::make_unique<spdlog::pattern_formatter>(patternConsole_, time_type);
                fallback_fmt->add_flag<TzFlag>('Z', utcMode_);
#else
                auto fallback_pattern = replace_Z_placeholder(patternConsole_, utcMode_);
                auto fallback_fmt = std::make_unique<spdlog::pattern_formatter>(fallback_pattern, time_type);
#endif
                fallback->set_formatter(std::move(fallback_fmt));
                distSink_->add_sink(fallback);
                consoleSink_ = fallback;
                std::cerr << "[logger_manager] No sinks enabled, fallback to console.\n";
            }

            logger_ = std::make_shared<spdlog::logger>(loggerName_, distSink_);
            spdlog::register_logger(logger_);

            applySoftSettings();

            if (flushEverySec_ > 0) {
                spdlog::flush_every(std::chrono::seconds(flushEverySec_));
            }

            // 초기 1회 디스크 확인
            checkDiskAndAct();

            need_start = (autoReloadIntervalSec_ > 0);
            interval_to_start = autoReloadIntervalSec_;
        } // 락 해제

        if (need_start) {
            startAutoReload(interval_to_start);
        }
        else {
            stopAutoReload();
        }

        return true;
    }

    std::shared_ptr<spdlog::logger> logger_manager::getLogger() const {
        std::lock_guard<std::mutex> lk(mu_);
        return logger_;
    }

    void logger_manager::applySoftSettings() {
        auto time_type = utcMode_ ? spdlog::pattern_time_type::utc
            : spdlog::pattern_time_type::local;
#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
        auto console_fmt = std::make_unique<spdlog::pattern_formatter>(patternConsole_, time_type);
        auto file_fmt = std::make_unique<spdlog::pattern_formatter>(patternFile_, time_type);

        // soft-reload 시에도 %Z 재등록(utc/local 변경 반영)
        console_fmt->add_flag<TzFlag>('Z', utcMode_);
        file_fmt->add_flag<TzFlag>('Z', utcMode_);
#else
        auto console_pattern = replace_Z_placeholder(patternConsole_, utcMode_);
        auto file_pattern = replace_Z_placeholder(patternFile_, utcMode_);
        auto console_fmt = std::make_unique<spdlog::pattern_formatter>(console_pattern, time_type);
        auto file_fmt = std::make_unique<spdlog::pattern_formatter>(file_pattern, time_type);
#endif

        if (consoleSink_) {
            consoleSink_->set_level(consoleMin_);
            consoleSink_->set_formatter(console_fmt->clone());
        }
        if (allSink_) {
            allSink_->set_level(allFileMin_);
            allSink_->set_formatter(file_fmt->clone());
        }
        if (alertsSink_) {
            alertsSink_->set_level(alertsMin_);
            alertsSink_->set_formatter(file_fmt->clone());
        }

        if (logger_) {
            logger_->set_level(loggerMin_);
            logger_->flush_on(flushOn_);
        }
    }

    void logger_manager::applyHardSettingsIfNeeded(
        bool , // old_enableConsole,
        bool old_enableFileAll,
        bool old_enableFileAlerts,
        const std::string& old_allPath,
        const std::string& old_alertsPath,
        std::size_t old_allMaxSize,
        std::size_t old_allMaxFiles,
        std::size_t old_alertMaxSize,
        std::size_t old_alertMaxFiles)
    {

        auto time_type = utcMode_ ? spdlog::pattern_time_type::utc
            : spdlog::pattern_time_type::local;
#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
        auto console_fmt = std::make_unique<spdlog::pattern_formatter>(patternConsole_, time_type);
        auto file_fmt = std::make_unique<spdlog::pattern_formatter>(patternFile_, time_type);

        // hard-reload 시에도 %Z 재등록
        console_fmt->add_flag<TzFlag>('Z', utcMode_);
        file_fmt->add_flag<TzFlag>('Z', utcMode_);
#else
        auto console_pattern = replace_Z_placeholder(patternConsole_, utcMode_);
        auto file_pattern = replace_Z_placeholder(patternFile_, utcMode_);
        auto console_fmt = std::make_unique<spdlog::pattern_formatter>(console_pattern, time_type);
        auto file_fmt = std::make_unique<spdlog::pattern_formatter>(file_pattern, time_type);
#endif

        bool console_add = enableConsole_ && !consoleSink_;
        bool console_remove = !enableConsole_ && consoleSink_;
        if (console_add) {
            auto s = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            s->set_level(consoleMin_);
            s->set_formatter(console_fmt->clone());
            distSink_->add_sink(s);
            consoleSink_ = s;
        }
        else if (console_remove) {
            consoleSink_->flush();
            distSink_->remove_sink(consoleSink_);
            consoleSink_.reset();
        }

        bool need_new_all =
            (enableFileAll_ && !allSink_) ||
            (!old_enableFileAll && enableFileAll_) ||
            (allSink_ && (allPath_ != old_allPath ||
                allMaxSize_ != old_allMaxSize ||
                allMaxFiles_ != old_allMaxFiles));

        if (enableFileAll_) {
            if (need_new_all) {
                ensureParentDir(allPath_);
                auto new_all = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    allPath_, allMaxSize_, allMaxFiles_, false);
                new_all->set_level(allFileMin_);
                new_all->set_formatter(file_fmt->clone());
                distSink_->add_sink(new_all);
                if (allSink_) {
                    allSink_->flush();
                    distSink_->remove_sink(allSink_);
                }
                allSink_.swap(new_all);
            }
        }
        else {
            if (allSink_) {
                allSink_->flush();
                distSink_->remove_sink(allSink_);
                allSink_.reset();
            }
        }

        bool need_new_alerts =
            (enableFileAlerts_ && !alertsSink_) ||
            (!old_enableFileAlerts && enableFileAlerts_) ||
            (alertsSink_ && (alertsPath_ != old_alertsPath ||
                alertMaxSize_ != old_alertMaxSize ||
                alertMaxFiles_ != old_alertMaxFiles));

        if (enableFileAlerts_) {
            if (need_new_alerts) {
                ensureParentDir(alertsPath_);
                auto new_alerts = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    alertsPath_, alertMaxSize_, alertMaxFiles_, false);
                new_alerts->set_level(alertsMin_);
                new_alerts->set_formatter(file_fmt->clone());
                distSink_->add_sink(new_alerts);
                if (alertsSink_) {
                    alertsSink_->flush();
                    distSink_->remove_sink(alertsSink_);
                }
                alertsSink_.swap(new_alerts);
            }
        }
        else {
            if (alertsSink_) {
                alertsSink_->flush();
                distSink_->remove_sink(alertsSink_);
                alertsSink_.reset();
            }
        }

        if (distSink_->sinks().empty()) {
            auto fallback = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            fallback->set_level(spdlog::level::trace);
#if defined(MINO_SPDLOG_HAS_CUSTOM_FLAG)
            auto fallback_fmt = std::make_unique<spdlog::pattern_formatter>(patternConsole_, time_type);
            fallback_fmt->add_flag<TzFlag>('Z', utcMode_);
#else
            auto fallback_pattern = replace_Z_placeholder(patternConsole_, utcMode_);
            auto fallback_fmt = std::make_unique<spdlog::pattern_formatter>(fallback_pattern, time_type);
#endif
            fallback->set_formatter(std::move(fallback_fmt));
            distSink_->add_sink(fallback);
            consoleSink_ = fallback;
            if (logger_) {
                logger_->warn("No sinks enabled after hard-reload. Fallback to console sink.");
            }
        }
    }

    bool logger_manager::reloadIfChanged() {
        std::lock_guard<std::mutex> lk(mu_);

        std::filesystem::file_time_type now;
        try {
            now = std::filesystem::last_write_time(iniPath_);
        }
        catch (...) {
            checkDiskAndAct();
            return false;
        }
        if (now == lastWriteTime_) {
            checkDiskAndAct();
            return false;
        }
        lastWriteTime_ = now;

        bool old_enableConsole = enableConsole_;
        bool old_enableFileAll = enableFileAll_;
        bool old_enableFileAlerts = enableFileAlerts_;
        std::string old_allPath = allPath_;
        std::string old_alertsPath = alertsPath_;
        std::size_t old_allMaxSize = allMaxSize_;
        std::size_t old_allMaxFiles = allMaxFiles_;
        std::size_t old_alertMaxSize = alertMaxSize_;
        std::size_t old_alertMaxFiles = alertMaxFiles_;

        bool ok = loadConfig(false);
        if (!ok) {
            checkDiskAndAct();
            return false;
        }

        applyHardSettingsIfNeeded(
            old_enableConsole, old_enableFileAll, old_enableFileAlerts,
            old_allPath, old_alertsPath,
            old_allMaxSize, old_allMaxFiles, old_alertMaxSize, old_alertMaxFiles);

        applySoftSettings();

        if (flushEverySec_ > 0) {
            spdlog::flush_every(std::chrono::seconds(flushEverySec_));
        }
        else {
            if (logger_) {
                logger_->warn("FLUSH_EVERY_SEC=0 detected. Disabling periodic flush at runtime is limited. Restart recommended.");
            }
        }

        checkDiskAndAct();
        return true;
    }

    bool logger_manager::startAutoReload(unsigned interval_sec) {
        std::lock_guard<std::mutex> lk(mu_);
        if (autoReloadRunning_) {
            autoReloadIntervalSec_ = interval_sec ? interval_sec : 60;
            return true;
        }
        if (interval_sec == 0) interval_sec = 60;
        autoReloadIntervalSec_ = interval_sec;

        autoReloadRunning_ = true;
        autoReloadThread_ = std::thread([this]() {
            while (autoReloadRunning_) {
                try {
                    this->reloadIfChanged();
                }
                catch (...) {
                }
                std::this_thread::sleep_for(std::chrono::seconds(this->autoReloadIntervalSec_));
            }
            });
        return true;
    }

    void logger_manager::stopAutoReload() {
        if (!autoReloadRunning_) return;
        autoReloadRunning_ = false;
        if (autoReloadThread_.joinable()) {
            autoReloadThread_.join();
        }
    }

    bool logger_manager::loadConfig(bool readAutoReload) {
        if (!ini_.load(iniPath_)) {
            return false;
        }

        // SimpleIni의 GetValue / GetLongValue / GetDoubleValue 를 흉내 내는 람다들
        auto get_str = [&](std::string_view key, const std::string& def) -> std::string {
            if (auto v = ini_.get_string(logSection_, std::string(key))) {
                return *v;
            }
            return def;
            };

        auto get_ll = [&](std::string_view key, long long def) -> long long {
            if (auto v = ini_.get_int(logSection_, std::string(key))) {
                return *v;
            }
            if (auto v = ini_.get_double(logSection_, std::string(key))) {
                return static_cast<long long>(*v);
            }
            return def;
            };

        auto get_d = [&](std::string_view key, double def) -> double {
            if (auto v = ini_.get_double(logSection_, std::string(key))) {
                return *v;
            }
            if (auto v = ini_.get_int(logSection_, std::string(key))) {
                return static_cast<double>(*v);
            }
            return def;
            };

        // TIME_MODE
        std::string time_mode = toLower(get_str("TIME_MODE", "local"));
        utcMode_ = (time_mode == "utc");

        // ENABLE_* 플래그
        enableConsole_ = toBool(get_str("ENABLE_CONSOLE_LOG", "true"), true);
        enableFileAll_ = toBool(get_str("ENABLE_FILE_LOG_ALL", "true"), true);
        enableFileAlerts_ = toBool(get_str("ENABLE_FILE_LOG_ALERTS", "true"), true);

        // 레벨들
        consoleMin_ = parseLevel(get_str("CONSOLE_LEVEL", "trace"), spdlog::level::trace);
        allFileMin_ = parseLevel(get_str("ALL_FILE_LEVEL", "trace"), spdlog::level::trace);
        alertsMin_ = parseLevel(get_str("ALERTS_FILE_LEVEL", "warn"), spdlog::level::warn);
        loggerMin_ = parseLevel(get_str("LOGGER_LEVEL", "trace"), spdlog::level::trace);
        flushOn_ = parseLevel(get_str("FLUSH_ON_LEVEL", "warn"), spdlog::level::warn);

        // flush 주기
        flushEverySec_ = static_cast<std::size_t>(
            get_ll("FLUSH_EVERY_SEC", 1));

        // 패턴
        patternConsole_ = get_str("PATTERN_CONSOLE",
            "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        patternFile_ = get_str("PATTERN_FILE",
            "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");

        // 파일 경로
        allPath_ = get_str("ALL_PATH", "logs/all.log");
        alertsPath_ = get_str("ALERTS_PATH", "logs/alerts.log");

        // 파일 롤링 파라미터
        allMaxSize_ = parseSizeBytes(
            get_str("ALL_MAX_SIZE", "104857600"),
            100ull * 1024ull * 1024ull);
        allMaxFiles_ = static_cast<std::size_t>(
            get_ll("ALL_MAX_FILES", 5));

        alertMaxSize_ = parseSizeBytes(
            get_str("ALERT_MAX_SIZE", "104857600"),
            100ull * 1024ull * 1024ull);
        alertMaxFiles_ = static_cast<std::size_t>(
            get_ll("ALERT_MAX_FILES", 10));

        // 디스크 감시 ON/OFF 및 파라미터
        diskGuardEnable_ = toBool(get_str("DISK_GUARD_ENABLE", "true"), true);
        diskRoot_ = get_str("DISK_ROOT", "");
        diskMinFreeRatio_ = get_d("DISK_MIN_FREE_RATIO", 5.0);

        // UDP 알림
        udpIp_ = get_str("UDP_ALERT_IP", "");
        udpPort_ = static_cast<std::uint16_t>(
            get_ll("UDP_ALERT_PORT", 0));
        udpIntervalSec_ = static_cast<unsigned>(
            get_ll("UDP_ALERT_INTERVAL_SEC", 60));
        udpMessageTmpl_ = get_str("UDP_ALERT_MESSAGE",
            "DISK LOW: path={path} free={avail_bytes}B ({ratio}%)");

        // AUTO_RELOAD_SEC
        if (readAutoReload) {
            autoReloadIntervalSec_ = static_cast<unsigned>(
                get_ll("AUTO_RELOAD_SEC", 60));
        }

        return true;
    }



    void logger_manager::ensureParentDir(const std::string& path) {
        try {
            std::filesystem::path p(path);
            if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
        }
        catch (...) {
        }
    }

    bool logger_manager::toBool(const std::string& val, bool default_val) const {
        std::string v = toLower(val);
        if (v == "true" || v == "1" || v == "yes" || v == "on")  return true;
        if (v == "false" || v == "0" || v == "no" || v == "off") return false;
        return default_val;
    }

    std::string logger_manager::toLower(const std::string& s) const {
        std::string res = s;
        std::transform(res.begin(), res.end(), res.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return res;
    }

    std::size_t logger_manager::parseSizeBytes(const std::string& s, std::size_t default_val) const {
        if (s.empty()) return default_val;
        std::string v = s;
        v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c); }), v.end());
        std::string lower = toLower(v);

        std::size_t i = 0;
        while (i < lower.size() && (std::isdigit(static_cast<unsigned char>(lower[i])) || lower[i] == '.')) ++i;
        if (i == 0) return default_val;

        std::string num_str = lower.substr(0, i);
        std::string unit_str = lower.substr(i);

        double num = 0.0;
        try { num = std::stod(num_str); }
        catch (...) { return default_val; }

        long double mul = 1.0L;
        if (unit_str.empty() || unit_str == "b") mul = 1.0L;
        else if (unit_str == "k" || unit_str == "kb") mul = 1024.0L;
        else if (unit_str == "m" || unit_str == "mb") mul = 1024.0L * 1024.0L;
        else if (unit_str == "g" || unit_str == "gb") mul = 1024.0L * 1024.0L * 1024.0L;
        else if (unit_str == "t" || unit_str == "tb") mul = 1024.0L * 1024.0L * 1024.0L * 1024.0L;
        else return default_val;

        unsigned long long bytes = static_cast<unsigned long long>(std::llround(num * mul));
        return static_cast<std::size_t>(bytes);
    }

    spdlog::level::level_enum logger_manager::parseLevel(
        const std::string& s, spdlog::level::level_enum def) const {
        std::string v = toLower(s);
        if (v == "trace")                  return spdlog::level::trace;
        if (v == "debug")                  return spdlog::level::debug;
        if (v == "info")                   return spdlog::level::info;
        if (v == "warn" || v == "warning") return spdlog::level::warn;
        if (v == "error" || v == "err")    return spdlog::level::err;
        if (v == "critical" || v == "crit")return spdlog::level::critical;
        if (v == "off")                    return spdlog::level::off;
        return def;
    }

    void logger_manager::checkDiskAndAct() {
        if (!diskGuardEnable_) {
            if (fileSinksDetachedForDisk_) {
                if (enableFileAll_ && allSink_)    distSink_->add_sink(allSink_);
                if (enableFileAlerts_ && alertsSink_) distSink_->add_sink(alertsSink_);
                applySoftSettings();
                fileSinksDetachedForDisk_ = false;
                if (logger_) logger_->info("Disk guard disabled by config. File logging resumed.");
            }
            return;
        }

        if (diskRoot_.empty()) return;

        std::filesystem::space_info info{};
        try {
            info = std::filesystem::space(std::filesystem::path(diskRoot_));
        }
        catch (...) {
            if (logger_) logger_->warn("DISK_ROOT='{}' space() failed. Skip this round.", diskRoot_);
            return;
        }

        unsigned long long avail = static_cast<unsigned long long>(info.available);
        unsigned long long cap = static_cast<unsigned long long>(info.capacity);
        long double ratio = cap > 0 ? (static_cast<long double>(avail) * 100.0L / static_cast<long double>(cap)) : 100.0L;

        bool low = (ratio < static_cast<long double>(diskMinFreeRatio_));

        if (low) {
            if (!fileSinksDetachedForDisk_) {
                if (allSink_) { allSink_->flush();    distSink_->remove_sink(allSink_); }
                if (alertsSink_) { alertsSink_->flush(); distSink_->remove_sink(alertsSink_); }
                fileSinksDetachedForDisk_ = true;
                if (logger_) logger_->warn("Low disk space on '{}': {:.2f}% free. File logging suspended, console only.", diskRoot_, static_cast<double>(ratio));
            }

            auto now = std::chrono::steady_clock::now();
            bool due = (lastUdpSent_.time_since_epoch().count() == 0) ||
                (now - lastUdpSent_ >= std::chrono::seconds(udpIntervalSec_));
            if (due && !udpIp_.empty() && udpPort_ > 0) {
                std::string payload = buildUdpMessage(udpMessageTmpl_, diskRoot_, avail, ratio);
                if (sendUdpAlert(payload)) {
                    lastUdpSent_ = now;
                }
            }
        }
        else {
            if (fileSinksDetachedForDisk_) {
                if (enableFileAll_ && allSink_)    distSink_->add_sink(allSink_);
                if (enableFileAlerts_ && alertsSink_) distSink_->add_sink(alertsSink_);
                applySoftSettings();
                fileSinksDetachedForDisk_ = false;
                if (logger_) logger_->info("Disk space recovered on '{}': {:.2f}% free. File logging resumed.", diskRoot_, static_cast<double>(ratio));
            }
        }
    }

    std::string logger_manager::buildUdpMessage(const std::string& tmpl,
        const std::string& path,
        unsigned long long availBytes,
        long double ratioPercent) const {
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss << std::setprecision(2) << static_cast<double>(ratioPercent);
        std::string ratio2 = oss.str();

        std::string msg = tmpl;
        replaceAll(msg, "{path}", path);
        replaceAll(msg, "{avail_bytes}", std::to_string(static_cast<long long>(availBytes)));
        replaceAll(msg, "{ratio}", ratio2);
        return msg;
    }

    void logger_manager::replaceAll(std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    bool logger_manager::sendUdpAlert(const std::string& msg) {

        // check validity
        if (udpIp_.empty()) return false;
        if (udpPort_ == 0) return false;
        if (msg.empty()) return false;

        // Boost.Asio 사용 코드
        //try {
        //    net::io_context io;
        //    net::ip::udp::endpoint ep(net::ip::make_address(udpIp_), udpPort_);
        //    net::ip::udp::socket sock(io);
        //    sock.open(net::ip::udp::v4());
        //    sock.send_to(net::buffer(msg), ep);
        //    sock.close();
        //    return true;
        //}
        //catch (...) {
        //    return false;
        // }

        mino::network::udp::udp_sender sender;
        if (sender.send_data_to(msg, udpIp_, udpPort_) < 0) {
            return false;
        }
        return true;

    }

}  
