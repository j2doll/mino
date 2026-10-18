#include "mino/external/log/sqlite/sqlite_logger.hpp"

#include "sqlite3/sqlite3.h"

#include <filesystem>
#include <iostream>

namespace mino::external::log::sqlite {

    constexpr uint64_t kMaxPossibleDbBytes = 281ULL * 1024 * 1024 * 1024 * 1024;
    constexpr size_t kMaxSqlLengthBytes = 1'000'000'000;

    sqlite_logger::~sqlite_logger() {
        close();
    }

    bool sqlite_logger::execute_sql(const std::string& sql) {
        char* error_message = nullptr;
        int rc = ::sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK) {
            if (error_message) {
                ::sqlite3_free(error_message);
            }
            return false;
        }
        return true;
    }

    void sqlite_logger::purge_old_logs(const std::string& table_name) {
        std::string sql = "DELETE FROM " + table_name +
            " WHERE id IN (SELECT id FROM " + table_name + " ORDER BY id ASC LIMIT 100);";
        execute_sql(sql);
    }

    void sqlite_logger::enforce_capacity() {
        if (max_capacity_bytes_ == 0) {
            return;
        }

        try {
            if (std::filesystem::exists(db_path_)) {
                uintmax_t current_size = std::filesystem::file_size(db_path_);

                while (current_size > max_capacity_bytes_ && !table_names_.empty()) {
                    for (const auto& table_name : table_names_) {
                        purge_old_logs(table_name);
                    }

                    uintmax_t new_size = std::filesystem::file_size(db_path_);
                    if (new_size >= current_size) {
                        break;
                    }
                    current_size = new_size;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Filesystem error during capacity enforcement: " << e.what() << "\n";
        }
    }

    std::string sqlite_logger::level_to_string(log_level level) {
        switch (level) {
        case log_level::trace:   return "TRACE";
        case log_level::debug:   return "DEBUG";
        case log_level::info:    return "INFO";
        case log_level::warning: return "WARNING";
        case log_level::error:   return "ERROR";
        case log_level::fatal:   return "FATAL";
        }
        return "UNKNOWN";
    }

    std::optional<std::string> sqlite_logger::initialize(const logger_config& config) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (config.max_capacity_bytes > kMaxPossibleDbBytes) {
            return "Error: max_capacity_bytes exceeds SQLite theoretical maximum limit (281 TB).";
        }

        try {
            db_path_ = config.db_path;
            max_capacity_bytes_ = config.max_capacity_bytes;
            table_names_ = config.table_names;  // table_names_ = config.table_names_;
            is_closed_ = false;

            int rc = ::sqlite3_open(db_path_.string().c_str(), &db_);
            if (rc != SQLITE_OK) {
                std::string err = "Can't open database: " + std::string(::sqlite3_errmsg(db_));
                if (db_) {
                    ::sqlite3_close(db_);
                    db_ = nullptr;
                }
                return err;
            }

            char* error_message = nullptr;
            int rc_exec = ::sqlite3_exec(db_, "PRAGMA auto_vacuum = FULL; PRAGMA journal_mode = WAL;", nullptr, nullptr, &error_message);
            if (rc_exec != SQLITE_OK) {
                std::string err = "Failed to set pragmas: " + (error_message ? std::string(error_message) : "unknown");
                if (error_message)
                    ::sqlite3_free(error_message);
                ::sqlite3_close(db_);
                db_ = nullptr;
                return err;
            }

            for (const auto& table_name : table_names_) {
                std::string create_table_sql =
                    "CREATE TABLE IF NOT EXISTS " + table_name + " ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "timestamp TEXT DEFAULT CURRENT_TIMESTAMP, "
                    "level TEXT, "
                    "message TEXT);";

                rc_exec = ::sqlite3_exec(db_, create_table_sql.c_str(), nullptr, nullptr, &error_message);
                if (rc_exec != SQLITE_OK) {
                    std::string err = "Failed to create table '" + table_name + "': " + (error_message ? std::string(error_message) : "unknown");
                    if (error_message)
                        ::sqlite3_free(error_message);
                    ::sqlite3_close(db_);
                    db_ = nullptr;
                    return err;
                }
            }

            return std::nullopt;
        }
        catch (const std::exception& e) {
            if (db_) {
                ::sqlite3_close(db_);
                db_ = nullptr;
            }
            return std::string("Initialization exception: ") + e.what();
        }
    }

    void sqlite_logger::close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (db_) {
            ::sqlite3_close(db_);
            db_ = nullptr;
        }
        is_closed_ = true;
    }

    std::vector<std::string> sqlite_logger::get_table_names() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return table_names_;
    }

    std::filesystem::path sqlite_logger::get_db_path() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return db_path_;
    }

    uint64_t sqlite_logger::get_max_capacity_bytes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return max_capacity_bytes_;
    }

    std::optional<std::string> sqlite_logger::write_log(const std::string& table_name, log_level level, const std::string& message) {
        if (message.size() > kMaxSqlLengthBytes) {
            return "Error: Message length exceeds SQLite SQL/parameter length limit (1,000,000,000 bytes).";
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (is_closed_ || !db_) {
            return "Error: Logger is closed or not initialized.";
        }

        bool valid_table = false;
        for (const auto& name : table_names_) {
            if (name == table_name) {
                valid_table = true;
                break;
            }
        }
        if (!valid_table) {
            return "Error: Table '" + table_name + "' is not registered in the logger configuration.";
        }

        enforce_capacity();

        std::string sql = "INSERT INTO " + table_name + " (level, message) VALUES (?, ?);";
        ::sqlite3_stmt* stmt = nullptr;
        int rc = ::sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return "Failed to prepare statement: " + std::string(::sqlite3_errmsg(db_));
        }

        std::string level_str = level_to_string(level);
        ::sqlite3_bind_text(stmt, 1, level_str.c_str(), -1, SQLITE_TRANSIENT);
        ::sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);

        rc = ::sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::string err_msg = "Execution failed: " + std::string(::sqlite3_errmsg(db_));
            ::sqlite3_finalize(stmt);
            return err_msg;
        }

        ::sqlite3_finalize(stmt);
        return std::nullopt;
    }

    std::optional<std::string> sqlite_logger::write_log(size_t table_index, log_level level, const std::string& message) {
        std::string target_table;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (table_index >= table_names_.size()) {
                return "Error: Table index " + std::to_string(table_index) + " is out of range.";
            }
            target_table = table_names_[table_index];
        }
        return write_log(target_table, level, message);
    }

    std::optional<std::string> sqlite_logger::write_log_trace(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::trace, message);
    }
    std::optional<std::string> sqlite_logger::write_log_debug(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::debug, message);
    }
    std::optional<std::string> sqlite_logger::write_log_info(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::info, message);
    }
    std::optional<std::string> sqlite_logger::write_log_warning(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::warning, message);
    }
    std::optional<std::string> sqlite_logger::write_log_error(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::error, message);
    }
    std::optional<std::string> sqlite_logger::write_log_fatal(const std::string& table_name, const std::string& message) {
        return write_log(table_name, log_level::fatal, message);
    }

    std::optional<std::string> sqlite_logger::write_log_trace(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::trace, message);
    }
    std::optional<std::string> sqlite_logger::write_log_debug(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::debug, message);
    }
    std::optional<std::string> sqlite_logger::write_log_info(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::info, message);
    }
    std::optional<std::string> sqlite_logger::write_log_warning(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::warning, message);
    }
    std::optional<std::string> sqlite_logger::write_log_error(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::error, message);
    }
    std::optional<std::string> sqlite_logger::write_log_fatal(size_t table_index, const std::string& message) {
        return write_log(table_index, log_level::fatal, message);
    }

} // namespace mino::external::log::sqlite
