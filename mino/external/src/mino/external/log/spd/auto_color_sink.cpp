#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/fmt/fmt.h>

#include <nlohmann/json.hpp>

#if defined(_WIN32) || defined(_WIN64)
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#endif

#include "mino/external/log/spd/auto_color_sink.hpp"

namespace mino::external::log::spd {

    // ---------------------------
    // strip_tags_formatter 구현
    // ---------------------------
    void strip_tags_formatter::format(const ::spdlog::details::log_msg& msg, ::spdlog::memory_buf_t& dest) {
        ::spdlog::pattern_formatter default_formatter;
        ::spdlog::memory_buf_t formatted;
        default_formatter.format(msg, formatted);

        std::string log_str = fmt::to_string(formatted);

        std::regex tag_regex(R"(<[^>]+>)");
        std::string cleaned_str = std::regex_replace(log_str, tag_regex, "");

        dest.append(cleaned_str.data(), cleaned_str.data() + cleaned_str.size());
    }

    std::unique_ptr<::spdlog::formatter> strip_tags_formatter::clone() const {
        return std::make_unique<strip_tags_formatter>();
    }

    // ---------------------------
    // auto_color_sink 구현
    // ---------------------------
    template<typename Mutex>
    std::string auto_color_sink<Mutex>::make_ansi_code(log_color color, text_style style) {
        std::string style_str = "0";
        if (style.bold)   style_str += ";1";
        if (style.italic) style_str += ";3";

        int color_val = static_cast<int>(color);
        if (color_val > 37) {
            return "\033[" + style_str + ";38;5;" + std::to_string(color_val) + "m";
        }
        return "\033[" + style_str + ";" + std::to_string(color_val) + "m";
    }

    template<typename Mutex>
    log_color auto_color_sink<Mutex>::string_to_color(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        if (str == "red")          return log_color::red;
        if (str == "green")        return log_color::green;
        if (str == "yellow")       return log_color::yellow;
        if (str == "blue")         return log_color::blue;
        if (str == "magenta")      return log_color::magenta;
        if (str == "cyan")         return log_color::cyan;
        if (str == "orange")       return log_color::orange;
        if (str == "pink")         return log_color::pink;
        if (str == "purple")       return log_color::purple;
        if (str == "light_green")  return log_color::light_green;
        if (str == "light_blue")   return log_color::light_blue;
        if (str == "gold")         return log_color::gold;
        if (str == "grey")         return log_color::grey;
        if (str == "dark_red")     return log_color::dark_red;
        return log_color::white;
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::write_to_console(const std::string& str) {
#if defined(_WIN32) || defined(_WIN64)
        if (!str.empty()) {
            int w_len = MultiByteToWideChar(
                CP_UTF8,
                0,
                str.c_str(),
                -1,
                nullptr,
                0);
            if (w_len > 0) {
                std::wstring w_str(w_len, L'\0');
                MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    str.c_str(),
                    -1,
                    &w_str[0],
                    w_len);

                HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
                DWORD written;
                WriteConsoleW(h_stdout, w_str.c_str(), static_cast<DWORD>(w_str.length() - 1), &written, nullptr);
            }
        }
#else
        std::fwrite(str.data(), 1, str.size(), stdout);
        std::fflush(stdout);
#endif
    }

    template<typename Mutex>
    std::string auto_color_sink<Mutex>::parse_tags_to_ansi(const std::string& tagged_text) {
        std::regex token_regex(R"(<([^>]+)>|([^<]+))");
        auto words_begin = std::sregex_iterator(tagged_text.begin(), tagged_text.end(), token_regex);
        auto words_end = std::sregex_iterator();

        struct style_state {
            log_color color = log_color::white;
            bool bold = false;
            bool italic = false;
        };

        std::vector<style_state> state_stack;
        state_stack.push_back(style_state{});

        std::string final_output = "";

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;

            if (match[1].matched) {
                std::string tag = match[1].str();
                if (tag[0] == '/') {
                    if (state_stack.size() > 1) state_stack.pop_back();
                }
                else {
                    style_state current_style = state_stack.back();
                    if (tag == "bold")         current_style.bold = true;
                    else if (tag == "italic")  current_style.italic = true;
                    else                       current_style.color = string_to_color(tag);
                    state_stack.push_back(current_style);
                }
            }
            else if (match[2].matched) {
                std::string text = match[2].str();
                style_state active = state_stack.back();
                final_output += make_ansi_code(active.color, { active.bold, active.italic }) + text + ansi_reset_;
            }
        }
        return final_output;
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::sink_it_(const ::spdlog::details::log_msg& msg) {
        ::spdlog::memory_buf_t formatted;
        ::spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string log_str = fmt::to_string(formatted);

        for (const auto& rule : rules_) {
            log_str = std::regex_replace(log_str, rule.reg, rule.color_code + "$&" + ansi_reset_);
        }

        std::string final_log = parse_tags_to_ansi(log_str);

        write_to_console(final_log);
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::flush_() {
        std::fflush(stdout);
    }

    template<typename Mutex>
    auto_color_sink<Mutex>::auto_color_sink(std::string name)
        : name_(std::move(name))
    {

#if defined(_WIN32) || defined(_WIN64)
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(hOut, &mode)) {
                SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
#endif
        add_keyword(R"(\[info\])", log_color::green, true);
        add_keyword(R"(\[warn(?:ing)?\])", log_color::yellow, true);
        add_keyword(R"(\[error\])", log_color::red, true);
        add_keyword(R"(\[critical\])", log_color::dark_red, true);
        add_keyword(R"(\[debug\])", log_color::light_blue, false);


    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::add_keyword(const std::string& keyword, log_color color, bool bold, bool italic) {
        keyword_rule rule;
        rule.reg = std::regex(keyword, std::regex_constants::icase);
        text_style style{ bold, italic };
        rule.color_code = make_ansi_code(color, style);
        rule.keyword = keyword; // 원본 패턴 저장
        rules_.push_back(rule);
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::remove_keyword(const std::string& keyword) {
        // 스레드 안전을 위해 base_sink의 mutex_로 보호
        std::lock_guard<Mutex> lock(::spdlog::sinks::base_sink<Mutex>::mutex_);
        rules_.erase(
            std::remove_if(rules_.begin(), rules_.end(), [&](const keyword_rule& r) {
                return r.keyword == keyword;
            }),
            rules_.end()
        );
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::load_keywords_from_json(const json& json_data) {
        if (!json_data.is_array())
            return;
        for (const auto& item : json_data) {
            if (item.contains("keyword") && item["keyword"].is_string()) {
                std::string keyword = item["keyword"];
                log_color color = log_color::white;
                if (item.contains("color") && item["color"].is_string())
                    color = string_to_color(item["color"]);
                bool bold = item.contains("bold") && item["bold"].is_boolean() ? (bool)item["bold"] : false;
                bool italic = item.contains("italic") && item["italic"].is_boolean() ? (bool)item["italic"] : false;
                add_keyword(keyword, color, bold, italic);
            }
        }
    }

    template<typename Mutex>
    void auto_color_sink<Mutex>::print_rich_text(const std::string& tagged_text, bool newline) {
        std::lock_guard<Mutex> lock(::spdlog::sinks::base_sink<Mutex>::mutex_);
        std::string final_output = parse_tags_to_ansi(tagged_text);
        if (newline) final_output += "\n";
        write_to_console(final_output);
    }

    template<typename Mutex>
    const std::string& auto_color_sink<Mutex>::name() const noexcept {
        return name_;
    }

    // 명시적 인스턴스화: 필요 시 여기에 다른 Mutex 타입 추가
    template class auto_color_sink<std::mutex>;
    template class auto_color_sink<::spdlog::details::null_mutex>;

} // namespace mino::external::log::spd
