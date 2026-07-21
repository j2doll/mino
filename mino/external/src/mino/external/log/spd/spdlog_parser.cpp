#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "mino/external/log/spd/spdlog_parser.hpp"

namespace mino::external::log::spd {

    spdlog_parser::spdlog_parser(spdlog_parser_config config)
        : config_(std::move(config)) {
        base_pattern_ = std::regex(R"(\[([^\]]+)\]\s+(.*?)\s+\[(trace|debug|info|warn|error|critical|TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL)\]\s+(.*))");
    }

    // 개별 한 줄을 파싱하는 공통 핵심 비즈니스 로직
    bool spdlog_parser::parse_single_line(std::string line, spdlog_parser_record& out_record) {
        trim_line_ending(line);
        std::string processed_line = handle_encoding(line);

        std::smatch matches;
        if (std::regex_match(processed_line, matches, base_pattern_)) {
            out_record.timestamp = parse_and_normalize_time(matches[1].str());
            out_record.logger_name = matches[2].str();
            out_record.log_level = matches[3].str();
            out_record.message = matches[4].str();
            return true;
        }
        return false;
    }

    std::string spdlog_parser::parse_and_normalize_time(const std::string& raw_time) {
        std::string main_time = raw_time;
        std::string ms_part = "000";

        size_t sep_pos = raw_time.find_last_of(config_.subsecond_separator);
        if (sep_pos != std::string::npos && sep_pos > raw_time.find_last_of(':')) {
            main_time = raw_time.substr(0, sep_pos);
            ms_part = raw_time.substr(sep_pos + 1);
            if (ms_part.length() > 3) ms_part = ms_part.substr(0, 3);
        }

        std::tm tm_info = {};
        std::istringstream ss(main_time);
        ss >> std::get_time(&tm_info, config_.time_format.c_str());

        if (ss.fail()) {
            return raw_time;
        }

        std::ostringstream oss;
        oss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S") << "." << ms_part;
        return oss.str();
    }

    void spdlog_parser::trim_line_ending(std::string& line) {
        if (line.empty()) return;

        switch (config_.line_ending) {
        case line_ending_type::crlf:
            if (line.back() == '\r') line.pop_back();
            break;
        case line_ending_type::lf:
            break;
        case line_ending_type::auto_detect:
        default:
            if (line.back() == '\r') line.pop_back();
            break;
        }
    }

    std::string spdlog_parser::handle_encoding(const std::string& raw_line) {
        if (config_.encoding == encoding_type::cp949_ansi) {
            return raw_line;
        }
        return raw_line;
    }

    std::vector<spdlog_parser_record> spdlog_parser::parse_log_file(const std::string& file_path) {
        std::vector<spdlog_parser_record> records;
        std::ifstream file(file_path, std::ios::in | std::ios::binary);

        if (!file.is_open()) {
            return records;
        }

        std::string line;
        while (std::getline(file, line)) {
            spdlog_parser_record record;
            if (parse_single_line(line, record)) {
                records.push_back(record);
            }
        }
        return records;
    }

    // (1) 신규 문자열 기반 스트림 파싱 구현
    std::vector<spdlog_parser_record> spdlog_parser::parse_log_string(const std::string& log_string) {
        std::vector<spdlog_parser_record> records;
        std::istringstream stream(log_string);
        std::string line;

        while (std::getline(stream, line)) {
            spdlog_parser_record record;
            if (parse_single_line(line, record)) {
                records.push_back(record);
            }
        }
        return records;
    }

}
