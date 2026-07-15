#include <fstream>
#include <algorithm>

#include "mino/core/csv/csv.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

namespace mino::core::csv {

    csv_handler::csv_handler(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger) {
        logger_ = custom_logger;
    }

    void csv_handler::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger) {
        logger_ = custom_logger;
    }

    std::vector<std::vector<std::string>> csv_handler::read_csv(const std::string& file_path, bool trim_empty_tails) {
        std::vector<std::vector<std::string>> data;

        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            if (logger_) logger_->error("Failed to open file for reading: {}", file_path);
            return data;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        data = parse_all_records(content, trim_empty_tails);

        if (logger_) logger_->info("Successfully read {} rows from file: {}", data.size(), file_path);
        return data;
    }

    std::vector<std::vector<std::string>> csv_handler::read_csv_string(const std::string& content, bool trim_empty_tails) {
        std::vector<std::vector<std::string>> data = parse_all_records(content, trim_empty_tails);
        if (logger_) logger_->info("Successfully read {} rows from string input", data.size());
        return data;
    }

    std::vector<std::vector<std::string>> csv_handler::parse_all_records(std::string_view content_view, bool trim_empty_tails) {
        std::vector<std::vector<std::string>> data;

        if (content_view.substr(0, 3) == "\xEF\xBB\xBF") {
            content_view.remove_prefix(3);
        }

        size_t pos = 0;
        size_t length = content_view.length();

        while (pos < length) {
            size_t current_line_end = length;
            size_t next_line_pos = length;
            std::string accum;

            while (pos < length) {
                size_t next_cr = content_view.find('\r', pos);
                size_t next_lf = content_view.find('\n', pos);
                size_t first_nl = std::min(next_cr, next_lf);

                if (first_nl != std::string_view::npos) {
                    current_line_end = first_nl;
                    if (content_view[first_nl] == '\r') {
                        next_line_pos = (first_nl + 1 < length && content_view[first_nl + 1] == '\n') ? first_nl + 2 : first_nl + 1;
                    }
                    else {
                        next_line_pos = first_nl + 1;
                    }
                }
                else {
                    current_line_end = length;
                    next_line_pos = length;
                }

                accum += std::string(content_view.substr(pos, next_line_pos - pos));
                pos = next_line_pos;

                size_t quote_count = std::count(accum.begin(), accum.end(), '"');
                if (quote_count % 2 == 0) {
                    size_t strip_len = next_line_pos - current_line_end;
                    accum.erase(accum.length() - strip_len, strip_len);
                    break;
                }
            }

            if (accum.empty() && pos >= length) {
                continue;
            }

            std::vector<std::string> row = parse_line(accum);

            if (trim_empty_tails) {
                while (!row.empty() && row.back().empty()) {
                    row.pop_back();
                }
            }

            if (!row.empty()) {
                data.push_back(row);
            }
        }

        return data;
    }

    bool csv_handler::write_csv(const std::string& file_path,
        const std::vector<std::vector<std::string>>& data,
        line_break_type lb_type,
        bool include_final_newline) {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            if (logger_) logger_->error("Failed to create file for writing: {}", file_path);
            return false;
        }

        // Write UTF-8 BOM
        file << "\xEF\xBB\xBF";

        // 1. 옵션에 따른 줄바꿈 문자열 결정
        std::string newline;
        switch (lb_type) {
        case line_break_type::lf:   newline = "\n";   break;
        case line_break_type::cr:   newline = "\r";   break;
        case line_break_type::crlf:
        default:                    newline = "\r\n"; break; // RFC 4180 Default
        }

        for (size_t i = 0; i < data.size(); ++i) {
            const auto& row = data[i];
            for (size_t j = 0; j < row.size(); ++j) {
                file << escape_field(row[j]);
                if (j < row.size() - 1) {
                    file << ",";
                }
            }

            // 2. 마지막 행 줄바꿈 처리 옵션 분기
            if (i < data.size() - 1) {
                // 마지막 행이 아니면 무조건 줄바꿈 수행
                file << newline;
            }
            else if (include_final_newline) {
                // 마지막 행일 때, 옵션이 true인 경우에만 줄바꿈 수행 (RFC 4180 권장)
                file << newline;
            }
        }

        if (logger_) logger_->info("Successfully wrote {} rows to file: {}", data.size(), file_path);
        return true;
    }

    std::vector<std::string> csv_handler::parse_line(std::string_view line) {
        std::vector<std::string> row;
        std::string current_field;
        bool inside_quotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (c == '"') {
                if (inside_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                    current_field += '"';
                    ++i;
                }
                else {
                    inside_quotes = !inside_quotes;
                }
            }
            else if (c == ',' && !inside_quotes) {
                row.push_back(current_field);
                current_field.clear();
            }
            else {
                current_field += c;
            }
        }
        row.push_back(current_field);

        return row;
    }

    std::string csv_handler::escape_field(std::string_view field) {
        bool require_quotes = false;

        if (field.find(',') != std::string_view::npos ||
            field.find('"') != std::string_view::npos ||
            field.find('\n') != std::string_view::npos ||
            field.find('\r') != std::string_view::npos) {
            require_quotes = true;
        }

        if (!require_quotes) {
            return std::string(field);
        }

        std::string escaped;
        escaped.reserve(field.size() + 2);
        escaped += '"';
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\"";
            }
            else {
                escaped += c;
            }
        }
        escaped += '"';

        return escaped;
    }

} // namespace mino::core::csv