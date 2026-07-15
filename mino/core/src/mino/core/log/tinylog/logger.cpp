#include <regex>

#if defined(_WIN32) || defined(_WIN64)
#   include <windows.h>
#else
#   include <iconv.h>
#endif

#include "mino/core/log/tinylog/logger.hpp"

namespace fs = std::filesystem;

namespace mino::core::log::tinylog {

    std::unordered_map<std::string, std::shared_ptr<logger>> logger::registry_;
    std::mutex logger::registry_mutex_;

    std::string_view to_string(log_level level) {
        switch (level) {
        case log_level::trace:    return "TRC";
        case log_level::debug:    return "DBG";
        case log_level::info:     return "INF";
        case log_level::warn:     return "WRN";
        case log_level::err:      return "ERR";
        case log_level::critical: return "CRT";
        }
        return "UNK";
    }

    std::string_view to_full_string(log_level level) {
        switch (level) {
        case log_level::trace:    return "TRACE";
        case log_level::debug:    return "DEBUG";
        case log_level::info:     return "INFO";
        case log_level::warn:     return "WARN";
        case log_level::err:      return "ERROR";
        case log_level::critical: return "CRITICAL";
        }
        return "UNKNOWN";
    }

    std::string to_korean_string(log_level level, encoding_type et) {
        std::string ret = "미정";
        switch (level) {
        case log_level::trace:    ret = "추적"; break;
        case log_level::debug:    ret = "디벅"; break;
        case log_level::info:     ret = "정보"; break;
        case log_level::warn:     ret = "경고"; break;
        case log_level::err:      ret = "오류"; break;
        case log_level::critical: ret = "치명"; break;
        }
        return convert_encoding(ret, et);
    }

    std::string_view to_string(eol_type eol) {
        switch (eol) {
        case eol_type::lf:   return "\n";
        case eol_type::crlf: return "\r\n";
        case eol_type::cr:   return "\r";
        }
        return "\n";
    }

    std::string convert_encoding(std::string_view src_utf8, encoding_type target_enc) {
        if (target_enc == encoding_type::utf8) {
            return std::string(src_utf8);
        }

#if defined(_WIN32) || defined(_WIN64)
        int wchar_len = MultiByteToWideChar(CP_UTF8, 0, src_utf8.data(), static_cast<int>(src_utf8.size()), nullptr, 0);
        if (wchar_len <= 0) return "";
        std::wstring wstr(wchar_len, 0);
        MultiByteToWideChar(CP_UTF8, 0, src_utf8.data(), static_cast<int>(src_utf8.size()), &wstr[0], wchar_len);

        int ansi_len = WideCharToMultiByte(949, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (ansi_len <= 0) return "";
        std::string converted(ansi_len, 0);
        WideCharToMultiByte(949, 0, wstr.data(), static_cast<int>(wstr.size()), &converted[0], ansi_len, nullptr, nullptr);
        return converted;
#else
        iconv_t cd = iconv_open("EUC-KR", "UTF-8");
        if (cd == (iconv_t)-1) return std::string(src_utf8);

        char* in_buf = const_cast<char*>(src_utf8.data());
        size_t in_bytes = src_utf8.size();
        size_t out_bytes = in_bytes * 2 + 1;
        std::string converted(out_bytes, 0);
        char* out_buf = &converted[0];

        size_t res = iconv(cd, &in_buf, &in_bytes, &out_buf, &out_bytes);
        iconv_close(cd);

        if (res == (size_t)-1) return std::string(src_utf8);
        converted.resize(converted.size() - out_bytes);
        return converted;
#endif
    }

    //--- console_sink 구현 ---
    console_sink::console_sink(std::string sink_name, console_sink_config config)
        : sink(std::move(sink_name)), config_(std::move(config)) {

#if defined(_WIN32) || defined(_WIN64)
        if (config_.encoding == encoding_type::utf8) {
            ::SetConsoleOutputCP(CP_UTF8);
            ::SetConsoleCP(CP_UTF8);
        }
        else if (config_.encoding == encoding_type::cp949) {
            ::SetConsoleOutputCP(949);
            ::SetConsoleCP(949);
        }
#endif
    }

    void console_sink::log(log_level /*level*/, std::string_view msg) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string converted_msg = convert_encoding(msg, config_.encoding);
        std::string ansi_reset = "\033[0m";

        // 정적 멤버 변수를 직접 순회하며 태그 치환
        for (const auto& item : style_maps) {
            std::string tag_open = convert_encoding("<" + item.tag + ">", config_.encoding);
            std::string tag_close = convert_encoding("</" + item.tag + ">", config_.encoding);

            std::size_t pos = 0;
            while ((pos = converted_msg.find(tag_open, pos)) != std::string::npos) {
                converted_msg.replace(pos, tag_open.length(), item.ansi_code);
                pos += item.ansi_code.length();
            }

            pos = 0;
            while ((pos = converted_msg.find(tag_close, pos)) != std::string::npos) {
                converted_msg.replace(pos, tag_close.length(), ansi_reset);
                pos += ansi_reset.length();
            }
        }

        std::cout << converted_msg << std::endl;
    }

    //--- rolling_file_sink 구현 ---
    std::shared_ptr<rolling_file_sink> rolling_file_sink::create(std::string sink_name, rolling_file_sink_config config) {
        if (sink_name.empty()) return nullptr;
        if (config.filename.empty()) return nullptr;
        if (config.max_size == 0) return nullptr;
        if (config.max_files == 0) return nullptr;

        return std::make_shared<rolling_file_sink>(private_token{ 0 }, std::move(sink_name), std::move(config));
    }

    rolling_file_sink::rolling_file_sink(private_token, std::string sink_name, rolling_file_sink_config config)
        : sink(std::move(sink_name)), config_(std::move(config)) {
        open_file();
    }

    void rolling_file_sink::open_file() {
        file_.open(config_.filename, std::ios::out | std::ios::app | std::ios::binary);
        if (file_.is_open()) {
            current_size_ = fs::file_size(config_.filename);
        }
    }

    void rolling_file_sink::rotate_files() {
        file_.close();

        for (auto i = config_.max_files - 1; i > 0; --i) {
            std::string old_name = config_.filename + "." + std::to_string(i);
            std::string new_name = config_.filename + "." + std::to_string(i + 1);
            if (fs::exists(old_name)) {
                if (fs::exists(new_name)) {
                    fs::remove(new_name);
                }
                fs::rename(old_name, new_name);
            }
        }

        if (fs::exists(config_.filename)) {
            std::string backup_name = config_.filename + ".1";
            if (fs::exists(backup_name)) {
                fs::remove(backup_name);
            }
            fs::rename(config_.filename, backup_name);
        }

        open_file();
    }

    void rolling_file_sink::log(log_level /*level*/, std::string_view msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) return;

        std::string converted_msg = convert_encoding(msg, config_.encoding);

        // 파일 싱크에서는 정규식을 이용하여 서식 태그들을 완전히 지우고 기록합니다.
        static const std::regex tag_regex(R"(<\/?([^>]+)>)");
        converted_msg = std::regex_replace(converted_msg, tag_regex, "");

        std::string_view eol_str = to_string(config_.eol);
        std::size_t total_len = converted_msg.size() + eol_str.size();

        if (current_size_ + total_len > config_.max_size) {
            rotate_files();
        }

        file_ << converted_msg << eol_str;
        file_.flush();
        current_size_ = fs::file_size(config_.filename);
    }

    //--- logger 구현 ---
    logger::logger(std::string name) : name_(std::move(name)) {}

    void logger::add_sink(std::shared_ptr<sink> s) {
        if (!s) return;
        std::lock_guard<std::mutex> lock(logger_mutex_);
        sinks_.push_back(std::move(s));
    }

    std::string logger::format_message(log_level level, std::string_view msg) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch() % std::chrono::seconds(1)
        ).count();

        std::tm time_info;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&time_info, &time_t_now);
#else
        localtime_r(&time_t_now, &time_info);
#endif 

        std::stringstream ss;
        ss << "[" << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms << "] "
            << "[" << name_ << "] "
            << "[" << to_string(level) << "] "
            << msg;
        return ss.str();
    }  

} // namespace mino::core::log::tinylog