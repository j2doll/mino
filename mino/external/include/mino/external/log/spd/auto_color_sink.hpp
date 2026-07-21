#pragma once

#include <regex>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <mutex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/pattern_formatter.h>

#include <nlohmann/json_fwd.hpp>

namespace mino::external::log::spd {

    enum class log_color {
        red = 31,
        green = 32,
        yellow = 33,
        blue = 34,
        magenta = 35,
        cyan = 36,
        white = 37,
        orange = 208,
        pink = 205,
        purple = 93,
        light_green = 118,
        light_blue = 81,
        gold = 220,
        grey = 244,
        dark_red = 88
    };

    struct  text_style {
        bool bold = false;
        bool italic = false;
    };

    // strip_tags_formatter는 헤더에 구현을 유지합니다 (비템플릿, 간단한 포맷터).
    class strip_tags_formatter : public ::spdlog::formatter {
    public:
        void format(const ::spdlog::details::log_msg& msg, ::spdlog::memory_buf_t& dest) override;

        std::unique_ptr<::spdlog::formatter> clone() const override;
    };

    // 템플릿 싱크 선언부
    template<typename Mutex>
    class auto_color_sink : public ::spdlog::sinks::base_sink<Mutex> {
    protected:
        using json = nlohmann::json;

        struct keyword_rule {
            std::regex reg;
            std::string color_code;
            std::string keyword; // 원본 패턴 문자열을 저장
        };

        std::vector<keyword_rule> rules_;
        const std::string ansi_reset_ = "\033[0m";

        // 전달된 이름을 저장
        std::string name_;

        // 내부 헬퍼 (구현은 .cpp)
        std::string make_ansi_code(log_color color, text_style style);
        log_color string_to_color(std::string str);
        void write_to_console(const std::string& str);
        std::string parse_tags_to_ansi(const std::string& tagged_text);

    protected:
        // spdlog 인터페이스
        void sink_it_(const ::spdlog::details::log_msg& msg) override;
        void flush_() override;

    public:
        auto_color_sink(std::string name = "");

        void add_keyword(const std::string& keyword, log_color color, bool bold = false, bool italic = false);
        void remove_keyword(const std::string& keyword); 
        void load_keywords_from_json(const json& json_data);

        // 외부 사용 API
        void print_rich_text(const std::string& tagged_text, bool newline = true);

        // 생성자에 전달된 이름을 조회하는 getter
        const std::string& name() const noexcept;
    };

} 
