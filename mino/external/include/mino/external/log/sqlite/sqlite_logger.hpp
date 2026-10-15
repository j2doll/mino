#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <filesystem>
#include <optional>
#include <cstdint>

struct sqlite3; // Forward declaration for SQLite3

namespace mino::external::log::sqlite {

    enum class log_level {
        trace,
        debug,
        info,
        warning,
        error,
        fatal
    };

    struct logger_config {
        std::filesystem::path db_path = "application.db"; // 기본 데이터베이스 파일 경로
        uint64_t max_capacity_bytes = 10 * 1024 * 1024; // 기본 10MB (0으로 설정 시 무제한)
        std::vector<std::string> table_names = { "system_logs" }; // 기본 테이블 이름
    };

    class sqlite_logger {
    private:
        struct ::sqlite3* db_ = nullptr;
        std::filesystem::path db_path_;
        uint64_t max_capacity_bytes_ = 0;
        std::vector<std::string> table_names_;
        mutable std::mutex mutex_;
        bool is_closed_ = false;

        sqlite_logger() = default;
        ~sqlite_logger();

        bool execute_sql(const std::string& sql);
        void purge_old_logs(const std::string& table_name);
        void enforce_capacity();
        std::string level_to_string(log_level level);

    public:
        static sqlite_logger& get_instance() {
            static sqlite_logger instance;
            return instance;
        }

        sqlite_logger(const sqlite_logger&) = delete;
        sqlite_logger& operator=(const sqlite_logger&) = delete;
        sqlite_logger(sqlite_logger&&) = delete;
        sqlite_logger& operator=(sqlite_logger&&) = delete;

        std::optional<std::string> initialize(const logger_config& config);
        void close();

        std::vector<std::string> get_table_names() const;
        std::filesystem::path get_db_path() const;
        uint64_t get_max_capacity_bytes() const;

        // 핵심 로깅 함수
        std::optional<std::string> write_log(const std::string& table_name, log_level level, const std::string& message);
        std::optional<std::string> write_log(size_t table_index, log_level level, const std::string& message);

        // 레벨별 헬퍼 함수들 (테이블 이름 지정)
        std::optional<std::string> write_log_trace(const std::string& table_name, const std::string& message);
        std::optional<std::string> write_log_debug(const std::string& table_name, const std::string& message);
        std::optional<std::string> write_log_info(const std::string& table_name, const std::string& message);
        std::optional<std::string> write_log_warning(const std::string& table_name, const std::string& message);
        std::optional<std::string> write_log_error(const std::string& table_name, const std::string& message);
        std::optional<std::string> write_log_fatal(const std::string& table_name, const std::string& message);

        // 레벨별 헬퍼 함수들 (테이블 인덱스 지정)
        std::optional<std::string> write_log_trace(size_t table_index, const std::string& message);
        std::optional<std::string> write_log_debug(size_t table_index, const std::string& message);
        std::optional<std::string> write_log_info(size_t table_index, const std::string& message);
        std::optional<std::string> write_log_warning(size_t table_index, const std::string& message);
        std::optional<std::string> write_log_error(size_t table_index, const std::string& message);
        std::optional<std::string> write_log_fatal(size_t table_index, const std::string& message);
    };

} // namespace mino::external::log::sqlite
