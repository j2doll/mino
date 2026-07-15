#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <unordered_map>

namespace mino::core::log::tinylog {

    // 로그 레벨 정의
    enum class log_level {
        trace,
        debug,
        info,
        warn,
        err,
        critical
    };

    // 인코딩 타입 정의
    enum class encoding_type {
        utf8,
        cp949
    };

    // End of Line (EOL) 타입 정의
    enum class eol_type {
        lf,
        crlf,
        cr
    };

    //--- 싱크 공통 설정 구조체 ---
    struct base_sink_config {
        encoding_type encoding{ encoding_type::utf8 };
        eol_type eol{ eol_type::lf };
    };

    //--- 콘솔 싱크 전용 설정 구조체 ---
    struct console_sink_config : public base_sink_config {
    };

    //--- 롤링 파일 싱크 전용 설정 구조체 ---
    struct rolling_file_sink_config : public base_sink_config {
        std::string filename{ "tinylog.txt" };
        std::size_t max_size{ 10 * 1024 * 1024 };
        std::size_t max_files{ 5 };
    };

    // 로깅 레벨 값을 문자열로 반환
    std::string_view to_string(log_level level); // 약어 문자열
    std::string_view to_full_string(log_level level); // 전체 문자열
    std::string to_korean_string(log_level level, encoding_type et = encoding_type::utf8); // 한국어 문자열

    // EOL 타입을 문자열로 반환
    std::string_view to_string(eol_type eol);

    // UTF-8 문자열을 지정된 인코딩으로 변환
    std::string convert_encoding(std::string_view src_utf8, encoding_type target_enc);

    //--- Sinks 인터페이스 ---
    class sink {
    protected:
        std::string sink_name_;

        explicit sink(std::string sink_name)
            : sink_name_(std::move(sink_name)) {
        }

    public:
        virtual ~sink() = default;

        virtual void log(log_level level, std::string_view msg) = 0;

        const std::string& name() const {
            return sink_name_;
        }
    };

    // 1. 콘솔 싱크
    class console_sink : public sink {
    private:
        std::mutex mutex_;
        console_sink_config config_;

    public:
        // 스타일 및 색상 매핑 구조체 정의
        struct ansi_style_map {
            std::string tag;
            std::string ansi_code;
        };

        // 스타일 및 색상(기본 16색) 매핑 정의 
        const std::vector<ansi_style_map> style_maps = {
            // --- 서식 스타일 (Bold, Italic) ---
            {"bold",          "\033[1m"},
            {"italic",        "\033[3m"},

            // --- Standard 8 Colors (기본색) ---
            {"black",         "\033[30m"},
            {"red",           "\033[31m"},
            {"green",         "\033[32m"},
            {"yellow",        "\033[33m"},
            {"blue",          "\033[34m"},
            {"magenta",       "\033[35m"},
            {"cyan",          "\033[36m"},
            {"white",         "\033[37m"},

            // --- Bright 8 Colors (밝은색) ---
            {"gray",          "\033[90m"},
            {"bright_red",    "\033[91m"},
            {"bright_green",  "\033[92m"},
            {"bright_yellow", "\033[93m"},
            {"bright_blue",   "\033[94m"},
            {"pink",          "\033[95m"},
            {"bright_cyan",   "\033[96m"},
            {"bright_white",  "\033[97m"}
        };  

        explicit console_sink(std::string sink_name, console_sink_config config = {});
        void log(log_level level, std::string_view msg) override;
    };

    // 2. 롤링 파일 싱크
    class rolling_file_sink : public sink {
    private:
        std::mutex mutex_;
        rolling_file_sink_config config_;
        std::size_t current_size_{ 0 };
        std::ofstream file_;

        void open_file();
        void rotate_files();

        struct private_token { explicit private_token(int) {} };

    public:
        static std::shared_ptr<rolling_file_sink> create(std::string sink_name, rolling_file_sink_config config);

        rolling_file_sink(private_token, std::string sink_name, rolling_file_sink_config config);
        void log(log_level level, std::string_view msg) override;
    };

    //--- 로거 클래스 ---
    class logger {
    private:
        std::string name_;
        std::vector<std::shared_ptr<sink>> sinks_;
        std::mutex logger_mutex_;
        log_level level_{ log_level::trace };

        static std::unordered_map<std::string, std::shared_ptr<logger>> registry_;
        static std::mutex registry_mutex_;

        std::string format_message(log_level level, std::string_view msg);

        template<typename... args>
        std::string format_string(std::string_view fmt, args&&... format_args) {
            std::vector<std::string> arg_strs;
            arg_strs.reserve(sizeof...(args));

            auto to_string_helper = [&](const auto& arg) {
                std::stringstream ss;
                ss << std::boolalpha;
                ss << arg;
                arg_strs.push_back(ss.str());
                };
            (to_string_helper(format_args), ...);

            std::string result;
            result.reserve(fmt.size() + (sizeof...(args) * 16));

            std::size_t last_pos = 0;
            std::size_t current_pos = 0;
            std::size_t auto_index = 0;

            while ((current_pos = fmt.find('{', last_pos)) != std::string_view::npos) {
                result.append(fmt.data() + last_pos, current_pos - last_pos);

                std::size_t close_pos = fmt.find('}', current_pos);
                if (close_pos == std::string_view::npos) {
                    last_pos = current_pos;
                    break;
                }

                std::string_view index_str = fmt.substr(current_pos + 1, close_pos - current_pos - 1);
                std::size_t target_index = 0;

                if (index_str.empty()) {
                    target_index = auto_index++;
                }
                else {
                    std::stringstream index_ss;
                    index_ss << index_str;
                    if (!(index_ss >> target_index)) {
                        result.append(fmt.data() + current_pos, close_pos - current_pos + 1);
                        last_pos = close_pos + 1;
                        continue;
                    }
                }

                if (target_index < arg_strs.size()) {
                    result.append(arg_strs[target_index]);
                }

                last_pos = close_pos + 1;
            }

            result.append(fmt.data() + last_pos, fmt.size() - last_pos);
            return result;
        }

    public:
        explicit logger(std::string name);

        static bool register_logger(std::shared_ptr<logger> lg) {
            if (!lg) return false;

            std::lock_guard<std::mutex> lock(registry_mutex_);
            if (registry_.find(lg->name()) != registry_.end()) {
                return false;
            }

            registry_[lg->name()] = lg;
            return true;
        }

        static std::shared_ptr<logger> get(const std::string& name) {
            std::lock_guard<std::mutex> lock(registry_mutex_);
            auto it = registry_.find(name);
            if (it != registry_.end()) {
                return it->second;
            }
            return nullptr;
        }

        static void drop_all() {
            std::lock_guard<std::mutex> lock(registry_mutex_);
            registry_.clear();
        }

        const std::string& name() const { return name_; }

        void set_level(log_level level) {
            std::lock_guard<std::mutex> lock(logger_mutex_);
            level_ = level;
        }
        log_level level() const { return level_; }

        void add_sink(std::shared_ptr<sink> s);

        template<typename... args>
        void log(log_level level, std::string_view fmt, args&&... format_args) {
            if (level < level_) return;

            std::string formatted_msg = format_string(fmt, std::forward<args>(format_args)...);
            std::string full_log = format_message(level, formatted_msg);

            std::lock_guard<std::mutex> lock(logger_mutex_);
            for (auto& s : sinks_) {
                if (s) s->log(level, full_log);
            }
        }

        template<typename... args> void trace(std::string_view fmt, args&&... format_args) { log(log_level::trace, fmt, std::forward<args>(format_args)...); }
        template<typename... args> void debug(std::string_view fmt, args&&... format_args) { log(log_level::debug, fmt, std::forward<args>(format_args)...); }
        template<typename... args> void info(std::string_view fmt, args&&... format_args) { log(log_level::info, fmt, std::forward<args>(format_args)...); }
        template<typename... args> void warn(std::string_view fmt, args&&... format_args) { log(log_level::warn, fmt, std::forward<args>(format_args)...); }
        template<typename... args> void error(std::string_view fmt, args&&... format_args) { log(log_level::err, fmt, std::forward<args>(format_args)...); }
        template<typename... args> void critical(std::string_view fmt, args&&... format_args) { log(log_level::critical, fmt, std::forward<args>(format_args)...); }
    };

}  
