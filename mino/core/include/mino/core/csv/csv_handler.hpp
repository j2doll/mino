#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <memory>

#include "mino/core/log/tinylog/tinylog_fwd.hpp"

namespace mino::core::csv {

    // 줄바꿈 기호 설정을 위한 열거형
    enum class line_break_type {
        crlf, // \r\n (윈도우 / RFC 4180 표준)
        lf,   // \n   (유닉스 / 리눅스 / 맥OS)
        cr    // \r   (구형 맥OS)
    };

    class csv_handler {
    public:

        explicit csv_handler(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger = nullptr);

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger);

        // Read CSV
        std::vector<std::vector<std::string>> read_csv(const std::string& file_path, bool trim_empty_tails = true);
        std::vector<std::vector<std::string>> read_csv_string(const std::string& content, bool trim_empty_tails = true);

        /**
         * @brief CSV 파일을 작성합니다.
         * @param file_path 저장할 파일 경로
         * @param data 저장할 2차원 문자열 데이터
         * @param lb_type 줄바꿈 기호 종류 (디폴트: CRLF)
         * @param include_final_newline 마지막 행 끝에 줄바꿈을 포함할지 여부 (디폴트: true)
         */
        bool write_csv(const std::string& file_path,
            const std::vector<std::vector<std::string>>& data,
            line_break_type lb_type = line_break_type::crlf,
            bool include_final_newline = true);

    private:
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;

        std::vector<std::vector<std::string>> parse_all_records(std::string_view content_view, bool trim_empty_tails);
        std::vector<std::string> parse_line(std::string_view line);
        std::string escape_field(std::string_view field);
    };

} // namespace mino::core::csv
