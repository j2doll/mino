#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <mutex>
#include <algorithm>
#include <cctype>

#if defined(_WIN32) || defined(_WIN64)
#   include <windows.h>
#else
#   include <iconv.h>
#   include <errno.h>
#endif

#include "mino/core/csv/excel_csv_writer.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

namespace mino::core::csv {

    excel_csv_writer::excel_csv_writer()
        : file_path_("output.csv"),
        max_file_size_(5 * 1024 * 1024),
        max_files_(3),
        charset_("CP949"),
        header_count_(0),
        is_initialized_(false),
        is_header_written_(false),
        logger_name_("excel_csv_logger_" + std::to_string(reinterpret_cast<uintptr_t>(this))) {
    }

    excel_csv_writer::~excel_csv_writer() {
        // nothing special; rolling_file_sink will close its file on destruction
    }

    void excel_csv_writer::set_file_path(const std::string& path) noexcept { file_path_ = path; }
    void excel_csv_writer::set_max_file_size(size_t size) noexcept { max_file_size_ = size; }

    bool excel_csv_writer::set_max_file_size(const std::string& size_str) noexcept {
        try {
            max_file_size_ = parse_size_string(size_str);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void excel_csv_writer::set_max_files(size_t count) noexcept { max_files_ = count; }
    void excel_csv_writer::set_charset(const std::string& charset) noexcept { charset_ = charset; }
    size_t excel_csv_writer::get_header_count() const noexcept { return header_count_; }

    size_t excel_csv_writer::parse_size_string(const std::string& size_str) {
        std::string s = size_str;
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);

        if (s.empty()) throw std::invalid_argument("Empty size string");

        size_t i = 0;
        while (i < s.size() && (std::isdigit(s[i]) || s[i] == '.')) {
            i++;
        }

        double value = std::stod(s.substr(0, i));
        std::string unit = s.substr(i);

        size_t multiplier = 1;
        if (unit == "KB" || unit == "K") multiplier = 1024;
        else if (unit == "MB" || unit == "M") multiplier = 1024 * 1024;
        else if (unit == "GB" || unit == "G") multiplier = 1024 * 1024 * 1024;
        else if (!unit.empty() && unit != "B" && unit != "BYTE" && unit != "BYTES") {
            throw std::invalid_argument("Unknown unit");
        }

        return static_cast<size_t>(value * multiplier);
    }

    bool excel_csv_writer::initialize() noexcept {
        if (is_initialized_) return true;
        return init_logger();
    }

    bool excel_csv_writer::init_logger() noexcept {
        try {
            std::filesystem::path p(file_path_);
            if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
                std::filesystem::create_directories(p.parent_path());
            }
            // configure tinylog rolling_file_sink
            mino::core::log::tinylog::rolling_file_sink_config cfg;
            cfg.filename = file_path_;
            cfg.max_size = max_file_size_;
            cfg.max_files = max_files_;
            // encoding
            if (charset_ == "CP949" || charset_ == "cp949") cfg.encoding = mino::core::log::tinylog::encoding_type::cp949;
            else cfg.encoding = mino::core::log::tinylog::encoding_type::utf8;
            // ensure CRLF for Excel
            cfg.eol = mino::core::log::tinylog::eol_type::crlf;

            sink_ = mino::core::log::tinylog::rolling_file_sink::create(logger_name_, std::move(cfg));
            if (!sink_) {
                is_initialized_ = false;
                return false;
            }

            is_initialized_ = true;
            return true;
        }
        catch (...) {
            is_initialized_ = false;
            return false;
        }
    }

    bool excel_csv_writer::write_header(const std::vector<std::string>& headers) noexcept {
        if (is_header_written_) {
            return false;
        }

        if (!is_initialized_ && !initialize()) return false;
        header_count_ = headers.size();

        if (std::filesystem::exists(file_path_) && std::filesystem::file_size(file_path_) > 0) {
            is_header_written_ = true;
            return true;
        }

        if (charset_ == "UTF-8" || charset_ == "utf-8") {
            // write BOM directly without the sink's EOL
            try {
                std::ofstream ofs(file_path_, std::ios::out | std::ios::binary | std::ios::app);
                if (!ofs.is_open()) return false;
                const char bom[] = "\xEF\xBB\xBF";
                ofs.write(bom, 3);
                ofs.flush();
            }
            catch (...) { return false; }
        }

        std::vector<std::string> modified_headers = headers;
        if (!modified_headers.empty() && modified_headers[0] == "ID") {
            is_header_written_ = true;
            return write_row_internal(modified_headers, true);
        }

        is_header_written_ = true;
        return write_row_internal(modified_headers, false);
    }

    bool excel_csv_writer::write_row(const std::vector<std::string>& row_data) noexcept {
        if (header_count_ > 0 && row_data.size() != header_count_) return false;
        if (!is_initialized_ && !initialize()) return false;

        return write_row_internal(row_data, false);
    }

    bool excel_csv_writer::write_row_pure_text(const std::vector<std::string>& row_data) noexcept {
        if (header_count_ > 0 && row_data.size() != header_count_) return false;
        if (!is_initialized_ && !initialize()) return false;

        return write_row_internal(row_data, true);
    }

    bool excel_csv_writer::write_row_internal(const std::vector<std::string>& row_data, bool force_text) noexcept {
        try {
            std::stringstream ss;
            for (size_t i = 0; i < row_data.size(); ++i) {
                if (force_text) ss << "=\"" << escape_csv_field(row_data[i]) << "\"";
                else ss << escape_csv_field(row_data[i]);
                if (i < row_data.size() - 1) ss << ",";
            }
                // Let the rolling_file_sink handle encoding and append the configured EOL.
                return log_safe(ss.str());
        }
        catch (...) { return false; }
    }

    bool excel_csv_writer::log_safe(const std::string& str) noexcept {
        if (!sink_) return false;
        try { sink_->log(mino::core::log::tinylog::log_level::info, str); return true; }
        catch (...) { return false; }
    }

    std::string excel_csv_writer::escape_csv_field(const std::string& field) {
        bool needs_quotes = false;
        std::string escaped = "";
        for (char c : field) {
            if (c == '"') { escaped += "\"\""; needs_quotes = true; }
            else { escaped += c; if (c == ',' || c == '\n' || c == '\r') needs_quotes = true; }
        }
        return needs_quotes ? "\"" + escaped + "\"" : field;
    }

    std::string excel_csv_writer::convert_encoding(const std::string& input, const std::string& target_charset) {
        if (target_charset == "UTF-8" || target_charset == "utf-8" ||
            target_charset == "UTF8" || target_charset == "utf8" ) {
            return input;
        }

        if (target_charset == "CP949" || target_charset == "cp949") {
#if defined(_WIN32) || defined(_WIN64)
            try {
                // UTF-8 -> wide
                int wlen = ::MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    input.data(),
                    static_cast<int>(input.size()),
                    nullptr,
                    0);
                if (wlen <= 0) {
                    // DWORD err = ::GetLastError();
                    return input;
                }

                std::wstring wstr(static_cast<size_t>(wlen), L'\0');
                ::MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    input.data(),
                    static_cast<int>(input.size()),
                    &wstr[0],
                    wlen);

                // wide -> CP949 (code page 949)
                int len = ::WideCharToMultiByte(
                    949,
                    0,
                    wstr.data(),
                    static_cast<int>(wstr.size()),
                    nullptr,
                    0,
                    nullptr,
                    nullptr);
                if (len <= 0) {
                    // DWORD err = ::GetLastError();
                    return input;
                }

                std::string out(static_cast<size_t>(len), '\0');
                int needed = ::WideCharToMultiByte(
                    949,
                    0,
                    wstr.data(),
                    static_cast<int>(wstr.size()),
                    &out[0],
                    len,
                    nullptr,
                    nullptr);
                (void)needed; // warning C4189

                return out;
            }
            catch (...) {
                return input;
            }
#else
            // iconv 를 사용한 UTF-8 -> CP949 변환
            iconv_t cd = iconv_open("CP949", "UTF-8");
            if (cd == (iconv_t)-1) {
                // 일부 시스템에서는 "CP949" 대신 "EUC-KR"을 지원할 수 있음
                cd = iconv_open("EUC-KR", "UTF-8");
                if (cd == (iconv_t)-1) {
                    return input;
                }
            }

            size_t inbytes = input.size();
            // CP949는 최대 2바이트 문자이므로 여유있게 2배 버퍼 할당
            size_t outbytes_left = inbytes * 2 + 16;
            std::vector<char> outbuf(outbytes_left);
            char* inbuf = const_cast<char*>(input.data());
            char* outptr = outbuf.data();
            size_t inleft = inbytes;
            size_t outleft = outbytes_left;

            // iconv은 char**을 변경하므로 포인터를 전달
            size_t res = iconv(cd, &inbuf, &inleft, &outptr, &outleft);
            if (res == (size_t)-1) {
                iconv_close(cd);
                return input;
            }

            std::string result(outbuf.data(), outptr - outbuf.data());
            iconv_close(cd);
            return result;
#endif
        }
        return input;
    }


}