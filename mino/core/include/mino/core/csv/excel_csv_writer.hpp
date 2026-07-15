#pragma once

#include <string>
#include <vector>
#include <memory>

#include "mino/core/log/tinylog/tinylog_fwd.hpp"

namespace mino::core::csv {

    class excel_csv_writer {
    public:
        excel_csv_writer();
        ~excel_csv_writer();

        void set_file_path(const std::string& path) noexcept;
        void set_max_file_size(size_t size) noexcept;
        bool set_max_file_size(const std::string& size_str) noexcept;
        void set_max_files(size_t count) noexcept;
        void set_charset(const std::string& charset) noexcept;

        bool initialize() noexcept;
        size_t get_header_count() const noexcept;

        bool write_header(const std::vector<std::string>& headers) noexcept;
        bool write_row(const std::vector<std::string>& row_data) noexcept;
        bool write_row_pure_text(const std::vector<std::string>& row_data) noexcept;

    protected:
        bool init_logger() noexcept;
        bool write_row_internal(const std::vector<std::string>& row_data, bool force_text) noexcept;
        bool log_safe(const std::string& str) noexcept;
        std::string escape_csv_field(const std::string& field);
        std::string convert_encoding(const std::string& input, const std::string& target_charset);
        size_t parse_size_string(const std::string& size_str);

    protected:
        std::string file_path_;
        size_t max_file_size_;
        size_t max_files_;
        std::string charset_;
        size_t header_count_;   
        bool is_initialized_;
        bool is_header_written_;
        std::string logger_name_;
        std::shared_ptr<mino::core::log::tinylog::rolling_file_sink> sink_;
    };

} // namespace mino::core::csv