#pragma once

#include <iostream>
#include <fstream> 
#include <string>
#include <filesystem>
#include <mutex>
#include <memory>
#include <chrono>
#include <atomic> 

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h> 
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if __has_include(<spdlog/pattern_formatter.h>)
#   include <spdlog/pattern_formatter.h>
#elif __has_include(<spdlog/details/pattern_formatter.h>)
#   include <spdlog/details/pattern_formatter.h>
#else
#   error "spdlog pattern_formatter header not found. Check your spdlog installation."
#endif

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

namespace mino::external::log::spd {

    enum class log_encoding {
        utf8, // UTF-8
        cp949 // CP949 (EUC-KR)
    };

    enum class line_ending {
        lf,    // \n
        crlf,  // \r\n
        cr     // \r
    };

    enum class time_zone_type {
        local_time,
        utc
    };

    // UTF-8 -> CP949 변환 함수 선언
     std::string convert_utf8_to_cp949(const std::string& utf8_str);

    // ============================================================================
    template<typename Mutex>
    class encoding_file_sink : public ::spdlog::sinks::base_sink<Mutex> {
    public:
        encoding_file_sink(
            const std::filesystem::path& filename,
            log_encoding encoding,
            line_ending ending,
            bool write_bom);
        virtual ~encoding_file_sink() = default;

    protected:
        void sink_it_(const ::spdlog::details::log_msg& msg) override;
        void flush_() override;

    protected:
        std::ofstream file_stream_;
        log_encoding encoding_;
        line_ending line_ending_;
    };

    // ============================================================================
    template<typename Mutex>
    class encoding_rotating_zipping_sink : public ::spdlog::sinks::base_sink<Mutex> {
    public:
        encoding_rotating_zipping_sink(
            const std::filesystem::path& filename,
            std::size_t max_size,
            std::size_t max_files,
            log_encoding encoding,
            line_ending ending,
            bool write_bom,
            bool delete_on_failure,
            int compression_level = 1,
            std::size_t max_zip_count = 5,
            time_zone_type timezone = time_zone_type::local_time);

        virtual ~encoding_rotating_zipping_sink();

        void set_skip_handler(bool skip);

    protected:
        void sink_it_(const ::spdlog::details::log_msg& msg) override;
        void flush_() override;

    protected:
        std::shared_ptr<std::atomic<bool>> skip_handler_; // changed to shared atomic
        log_encoding encoding_;
        line_ending line_ending_;
        bool write_bom_;
        std::size_t max_files_;
        bool delete_on_failure_;
        int compression_level_;
        std::size_t max_zip_count_;
        time_zone_type timezone_;
        std::shared_ptr<::spdlog::logger> console_logger_;
        std::shared_ptr<::spdlog::sinks::rotating_file_sink<Mutex>> backend_sink_;

    };

    // ============================================================================
    template<typename Mutex, typename FileNameCalc = ::spdlog::sinks::daily_filename_calculator>
    class encoding_daily_zipping_sink : public ::spdlog::sinks::base_sink<Mutex> {
    public:
        encoding_daily_zipping_sink(
            const std::filesystem::path& filename,
            int rotation_hour,
            int rotation_minute,
            log_encoding encoding,
            line_ending ending,
            bool write_bom,
            bool delete_on_failure,
            int compression_level = 1,
            std::size_t max_zip_count = 5,
            uint32_t max_files = 3,
            time_zone_type timezone = time_zone_type::local_time);

        virtual ~encoding_daily_zipping_sink();

        void set_skip_handler(bool skip);

    protected:
        void sink_it_(const ::spdlog::details::log_msg& msg) override;
        void flush_() override;

    protected:
        std::shared_ptr<std::atomic<bool>> skip_handler_; // changed to shared atomic
        log_encoding encoding_;
        line_ending line_ending_;
        bool write_bom_;
        bool delete_on_failure_;
        int compression_level_;
        std::size_t max_zip_count_;
        time_zone_type timezone_;
        std::shared_ptr<::spdlog::logger> console_logger_;
        std::shared_ptr<::spdlog::sinks::daily_file_sink<Mutex, FileNameCalc>> backend_sink_;
    };

    // 타입 별칭 정의
    using encoding_file_sink_mt = encoding_file_sink<std::mutex>;
    using encoding_file_sink_st = encoding_file_sink<::spdlog::details::null_mutex>;

    using encoding_rotating_zipping_sink_mt = encoding_rotating_zipping_sink<std::mutex>;
    using encoding_rotating_zipping_sink_st = encoding_rotating_zipping_sink<::spdlog::details::null_mutex>;

    using encoding_daily_zipping_sink_mt = encoding_daily_zipping_sink<std::mutex>;
    using encoding_daily_zipping_sink_st = encoding_daily_zipping_sink<::spdlog::details::null_mutex>;

} 
