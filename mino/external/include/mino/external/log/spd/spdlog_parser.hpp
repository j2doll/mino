#pragma once

#include <string>
#include <vector>
#include <regex>

namespace mino::external::log::spd {

    struct spdlog_parser_record {
        std::string timestamp;
        std::string logger_name;
        std::string log_level;
        std::string message;
    };

    class spdlog_parser {
    public:
        enum class encoding_type {
            utf8,
            cp949_ansi
        };

        enum class line_ending_type {
            auto_detect,
            lf,
            crlf
        };

        struct spdlog_parser_config {
            std::string time_format = "%Y-%m-%d %H:%M:%S";
            char subsecond_separator = '.';
            encoding_type encoding = encoding_type::utf8;
            line_ending_type line_ending = line_ending_type::auto_detect;
        };

        explicit spdlog_parser(spdlog_parser_config config);
        ~spdlog_parser() = default;

        // 기존 파일 패스 파싱 함수
        std::vector<spdlog_parser_record> parse_log_file(const std::string& file_path);

        // (1) 신규 문자열 파싱 함수 추가
        std::vector<spdlog_parser_record> parse_log_string(const std::string& log_string);

    private:
        spdlog_parser_config config_;
        std::regex base_pattern_;

        // 내부 공통 줄바꿈 파싱용 헬퍼
        bool parse_single_line(std::string line, spdlog_parser_record& out_record);
        std::string parse_and_normalize_time(const std::string& raw_time);
        void trim_line_ending(std::string& line);
        std::string handle_encoding(const std::string& raw_line);
    };

}  


