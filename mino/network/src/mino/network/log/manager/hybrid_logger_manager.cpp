#include "mino/network/udp/udp_sender.hpp"
#include "mino/network/log/manager/hybrid_logger_manager.hpp"

namespace mino::network::log::manager {

    hybrid_logger_manager::hybrid_logger_manager() {
        consoleMin_ = mino::core::log::tinylog::log_level::trace;
        allFileMin_ = mino::core::log::tinylog::log_level::trace;
        alertsMin_ = mino::core::log::tinylog::log_level::warn;
        loggerMin_ = mino::core::log::tinylog::log_level::trace;
    }

    hybrid_logger_manager::~hybrid_logger_manager() {
        stopAutoReload();
    }

    bool hybrid_logger_manager::init(const std::string& defaultConfigPath,
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
                    std::cout << "[hybrid_logger_manager] Using config from " << envName << ": " << iniPath_ << "\n";
                }
                else {
                    iniPath_ = defaultConfigPath;
                    std::cout << "[hybrid_logger_manager] Env var " << envName << " not set, using default: " << iniPath_ << "\n";
                }
            }
            else {
                iniPath_ = defaultConfigPath;
                std::cout << "[hybrid_logger_manager] Using default config path: " << iniPath_ << "\n";
            }

            if (!loadConfig(true)) {
                std::cerr << "[hybrid_logger_manager] Failed to load config.\n";
                return false;
            }

            try {
                lastWriteTime_ = std::filesystem::last_write_time(iniPath_);
            }
            catch (...) {
                lastWriteTime_ = std::filesystem::file_time_type{};
            }

            // 1. 콘솔 싱크 생성
            mino::core::log::tinylog::console_sink_config c_cfg;
            c_cfg.encoding = consoleEncoding_;
            auto c_target = std::make_shared<mino::core::log::tinylog::console_sink>("console", c_cfg);
            consoleSinkWrapper_ = std::make_shared<filter_sink>("console_filter", c_target, consoleMin_, enableConsole_);

            // 2. 전체 로그 파일 싱크 생성
            ensureParentDir(allPath_);
            mino::core::log::tinylog::rolling_file_sink_config af_cfg;
            af_cfg.filename = allPath_;
            af_cfg.max_size = allMaxSize_;
            af_cfg.max_files = allMaxFiles_;
            af_cfg.encoding = allEncoding_;
            af_cfg.eol = allLineEnding_;
            auto af_target = mino::core::log::tinylog::rolling_file_sink::create("all_file", af_cfg);
            allSinkWrapper_ = std::make_shared<filter_sink>("all_filter", af_target, allFileMin_, enableFileAll_);

            // 3. 경고 로그 파일 싱크 생성
            ensureParentDir(alertsPath_);
            mino::core::log::tinylog::rolling_file_sink_config al_cfg;
            al_cfg.filename = alertsPath_;
            al_cfg.max_size = alertMaxSize_;
            al_cfg.max_files = alertMaxFiles_;
            al_cfg.encoding = alertsEncoding_;
            al_cfg.eol = alertsLineEnding_;
            auto al_target = mino::core::log::tinylog::rolling_file_sink::create("alerts_file", al_cfg);
            alertsSinkWrapper_ = std::make_shared<filter_sink>("alerts_filter", al_target, alertsMin_, enableFileAlerts_);

            // 4. tinylog 로거 생성 및 싱크 등록
            logger_ = std::make_shared<mino::core::log::tinylog::logger>(loggerName_);
            logger_->set_level(loggerMin_);
            logger_->add_sink(consoleSinkWrapper_);
            logger_->add_sink(allSinkWrapper_);
            logger_->add_sink(alertsSinkWrapper_);

            mino::core::log::tinylog::logger::register_logger(logger_);

            applySoftSettings();

            // 초기 1회 디스크 감시
            checkDiskAndAct();

            need_start = (autoReloadIntervalSec_ > 0);
            interval_to_start = autoReloadIntervalSec_;
        }

        if (need_start) {
            startAutoReload(interval_to_start);
        }
        else {
            stopAutoReload();
        }

        return true;
    }

    std::shared_ptr<mino::core::log::tinylog::logger> hybrid_logger_manager::getLogger() const {
        std::lock_guard<std::mutex> lk(mu_);
        return logger_;
    }

    void hybrid_logger_manager::applySoftSettings() {
        if (consoleSinkWrapper_) {
            consoleSinkWrapper_->set_level(consoleMin_);
            consoleSinkWrapper_->set_enabled(enableConsole_);
        }
        if (allSinkWrapper_) {
            allSinkWrapper_->set_level(allFileMin_);
            allSinkWrapper_->set_enabled(enableFileAll_ && !fileSinksDetachedForDisk_);
        }
        if (alertsSinkWrapper_) {
            alertsSinkWrapper_->set_level(alertsMin_);
            alertsSinkWrapper_->set_enabled(enableFileAlerts_ && !fileSinksDetachedForDisk_);
        }
        if (logger_) {
            logger_->set_level(loggerMin_);
        }
    }

    void hybrid_logger_manager::applyHardSettingsIfNeeded(
        bool /*old_enableConsole*/,
        bool /*old_enableFileAll*/,
        bool /*old_enableFileAlerts*/,
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
        mino::core::log::tinylog::eol_type old_alertsLineEnding)
    {
        // 콘솔 싱크 인코딩 변경 체크
        if (consoleEncoding_ != old_consoleEncoding && consoleSinkWrapper_) {
            mino::core::log::tinylog::console_sink_config c_cfg;
            c_cfg.encoding = consoleEncoding_;
            auto c_target = std::make_shared<mino::core::log::tinylog::console_sink>("console", c_cfg);
            consoleSinkWrapper_->set_target(c_target);
        }

        // 전체 파일 싱크 속성 변경 체크
        bool need_new_all = (allPath_ != old_allPath) ||
            (allMaxSize_ != old_allMaxSize) ||
            (allMaxFiles_ != old_allMaxFiles) ||
            (allEncoding_ != old_allEncoding) ||
            (allLineEnding_ != old_allLineEnding);

        if (need_new_all && allSinkWrapper_) {
            ensureParentDir(allPath_);
            mino::core::log::tinylog::rolling_file_sink_config af_cfg;
            af_cfg.filename = allPath_;
            af_cfg.max_size = allMaxSize_;
            af_cfg.max_files = allMaxFiles_;
            af_cfg.encoding = allEncoding_;
            af_cfg.eol = allLineEnding_;
            auto af_target = mino::core::log::tinylog::rolling_file_sink::create("all_file", af_cfg);
            allSinkWrapper_->set_target(af_target);
        }

        // 경고 파일 싱크 속성 변경 체크
        bool need_new_alerts = (alertsPath_ != old_alertsPath) ||
            (alertMaxSize_ != old_alertMaxSize) ||
            (alertMaxFiles_ != old_alertMaxFiles) ||
            (alertsEncoding_ != old_alertsEncoding) ||
            (alertsLineEnding_ != old_alertsLineEnding);

        if (need_new_alerts && alertsSinkWrapper_) {
            ensureParentDir(alertsPath_);
            mino::core::log::tinylog::rolling_file_sink_config al_cfg;
            al_cfg.filename = alertsPath_;
            al_cfg.max_size = alertMaxSize_;
            al_cfg.max_files = alertMaxFiles_;
            al_cfg.encoding = alertsEncoding_;
            al_cfg.eol = alertsLineEnding_;
            auto al_target = mino::core::log::tinylog::rolling_file_sink::create("alerts_file", al_cfg);
            alertsSinkWrapper_->set_target(al_target);
        }
    }

    bool hybrid_logger_manager::reloadIfChanged() {
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
        auto old_consoleEncoding = consoleEncoding_;
        auto old_allEncoding = allEncoding_;
        auto old_alertsEncoding = alertsEncoding_;
        auto old_allLineEnding = allLineEnding_;
        auto old_alertsLineEnding = alertsLineEnding_;

        bool ok = loadConfig(false);
        if (!ok) {
            checkDiskAndAct();
            return false;
        }

        applyHardSettingsIfNeeded(
            old_enableConsole, old_enableFileAll, old_enableFileAlerts,
            old_allPath, old_alertsPath,
            old_allMaxSize, old_allMaxFiles, old_alertMaxSize, old_alertMaxFiles,
            old_consoleEncoding, old_allEncoding, old_alertsEncoding,
            old_allLineEnding, old_alertsLineEnding);

        applySoftSettings();
        checkDiskAndAct();
        return true;
    }

    bool hybrid_logger_manager::startAutoReload(unsigned interval_sec) {
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

    void hybrid_logger_manager::stopAutoReload() {
        if (!autoReloadRunning_) return;
        autoReloadRunning_ = false;
        if (autoReloadThread_.joinable()) {
            autoReloadThread_.join();
        }
    }

    bool hybrid_logger_manager::loadConfig(bool readAutoReload) {
        if (!ini_.load(iniPath_)) {
            return false;
        }

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

        std::string time_mode = toLower(get_str("TIME_MODE", "local"));
        utcMode_ = (time_mode == "utc");

        enableConsole_ = toBool(get_str("ENABLE_CONSOLE_LOG", "true"), true);
        enableFileAll_ = toBool(get_str("ENABLE_FILE_LOG_ALL", "true"), true);
        enableFileAlerts_ = toBool(get_str("ENABLE_FILE_LOG_ALERTS", "true"), true);

        consoleMin_ = parseLevel(get_str("CONSOLE_LEVEL", "trace"), mino::core::log::tinylog::log_level::trace);
        allFileMin_ = parseLevel(get_str("ALL_FILE_LEVEL", "trace"), mino::core::log::tinylog::log_level::trace);
        alertsMin_ = parseLevel(get_str("ALERTS_FILE_LEVEL", "warn"), mino::core::log::tinylog::log_level::warn);
        loggerMin_ = parseLevel(get_str("LOGGER_LEVEL", "trace"), mino::core::log::tinylog::log_level::trace);

        allPath_ = get_str("ALL_PATH", "logs/all.log");
        alertsPath_ = get_str("ALERTS_PATH", "logs/alerts.log");

        allMaxSize_ = parseSizeBytes(get_str("ALL_MAX_SIZE", "104857600"), 100ull * 1024ull * 1024ull);
        allMaxFiles_ = static_cast<std::size_t>(get_ll("ALL_MAX_FILES", 5));

        alertMaxSize_ = parseSizeBytes(get_str("ALERT_MAX_SIZE", "104857600"), 100ull * 1024ull * 1024ull);
        alertMaxFiles_ = static_cast<std::size_t>(get_ll("ALERT_MAX_FILES", 10));

        diskGuardEnable_ = toBool(get_str("DISK_GUARD_ENABLE", "true"), true);
        diskRoot_ = get_str("DISK_ROOT", "");
        diskMinFreeRatio_ = get_d("DISK_MIN_FREE_RATIO", 5.0);

        udpIp_ = get_str("UDP_ALERT_IP", "");
        udpPort_ = static_cast<std::uint16_t>(get_ll("UDP_ALERT_PORT", 0));
        udpIntervalSec_ = static_cast<unsigned>(get_ll("UDP_ALERT_INTERVAL_SEC", 60));
        udpMessageTmpl_ = get_str("UDP_ALERT_MESSAGE", "DISK LOW: path={path} free={avail_bytes}B ({ratio}%)");

        if (readAutoReload) {
            autoReloadIntervalSec_ = static_cast<unsigned>(get_ll("AUTO_RELOAD_SEC", 60));
        }

        auto enc_from_str = [&](const std::string& s) -> mino::core::log::tinylog::encoding_type {
            std::string v = toLower(s);
            if (v == "cp949" || v == "cp-949" || v == "euckr" || v == "euc-kr") {
                return mino::core::log::tinylog::encoding_type::cp949;
            }
            return mino::core::log::tinylog::encoding_type::utf8;
            };

        auto le_from_str = [&](const std::string& s) -> mino::core::log::tinylog::eol_type {
            std::string v = toLower(s);
            if (v == "crlf") return mino::core::log::tinylog::eol_type::crlf;
            if (v == "cr") return mino::core::log::tinylog::eol_type::cr;
            return mino::core::log::tinylog::eol_type::lf;
            };

        consoleEncoding_ = enc_from_str(get_str("CONSOLE_ENCODING", "utf8"));
        allEncoding_ = enc_from_str(get_str("ALL_FILE_ENCODING", "utf8"));
        alertsEncoding_ = enc_from_str(get_str("ALERTS_FILE_ENCODING", "utf8"));
        allLineEnding_ = le_from_str(get_str("ALL_FILE_LINE_ENDING", "lf"));
        alertsLineEnding_ = le_from_str(get_str("ALERTS_FILE_LINE_ENDING", "lf"));

        return true;
    }

    void hybrid_logger_manager::ensureParentDir(const std::string& path) {
        try {
            std::filesystem::path p(path);
            if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
        }
        catch (...) {
        }
    }

    bool hybrid_logger_manager::toBool(const std::string& val, bool default_val) const {
        std::string v = toLower(val);
        if (v == "true" || v == "1" || v == "yes" || v == "on")  return true;
        if (v == "false" || v == "0" || v == "no" || v == "off") return false;
        return default_val;
    }

    std::string hybrid_logger_manager::toLower(const std::string& s) const {
        std::string res = s;
        std::transform(res.begin(), res.end(), res.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return res;
    }

    std::size_t hybrid_logger_manager::parseSizeBytes(const std::string& s, std::size_t default_val) const {
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

    mino::core::log::tinylog::log_level hybrid_logger_manager::parseLevel(
        const std::string& s, mino::core::log::tinylog::log_level def) const {
        std::string v = toLower(s);
        if (v == "trace")                  return mino::core::log::tinylog::log_level::trace;
        if (v == "debug")                  return mino::core::log::tinylog::log_level::debug;
        if (v == "info")                   return mino::core::log::tinylog::log_level::info;
        if (v == "warn" || v == "warning") return mino::core::log::tinylog::log_level::warn;
        if (v == "error" || v == "err")    return mino::core::log::tinylog::log_level::err;
        if (v == "critical" || v == "crit")return mino::core::log::tinylog::log_level::critical;
        return def;
    }

    void hybrid_logger_manager::checkDiskAndAct() {
        if (!diskGuardEnable_) {
            if (fileSinksDetachedForDisk_) {
                fileSinksDetachedForDisk_ = false;
                if (allSinkWrapper_) allSinkWrapper_->set_enabled(enableFileAll_);
                if (alertsSinkWrapper_) alertsSinkWrapper_->set_enabled(enableFileAlerts_);
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
                fileSinksDetachedForDisk_ = true;
                if (allSinkWrapper_) allSinkWrapper_->set_enabled(false);
                if (alertsSinkWrapper_) alertsSinkWrapper_->set_enabled(false);
                if (logger_) logger_->warn("Low disk space on '{}': {}% free. File logging suspended, console only.", diskRoot_, static_cast<double>(ratio));
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
                fileSinksDetachedForDisk_ = false;
                if (allSinkWrapper_) allSinkWrapper_->set_enabled(enableFileAll_);
                if (alertsSinkWrapper_) alertsSinkWrapper_->set_enabled(enableFileAlerts_);
                if (logger_) logger_->info("Disk space recovered on '{}': {}% free. File logging resumed.", diskRoot_, static_cast<double>(ratio));
            }
        }
    }

    std::string hybrid_logger_manager::buildUdpMessage(const std::string& tmpl,
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

    void hybrid_logger_manager::replaceAll(std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    bool hybrid_logger_manager::sendUdpAlert(const std::string& msg) {
        if (udpIp_.empty()) return false;
        if (udpPort_ == 0) return false;
        if (msg.empty()) return false;

        mino::network::udp::udp_sender sender;
        if (sender.send_data_to(msg, udpIp_, udpPort_) < 0) {
            return false;
        }
        return true;
    }

} // namespace mino::network::log::manager
