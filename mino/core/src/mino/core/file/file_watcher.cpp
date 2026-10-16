#include "mino/core/file/file_watcher.hpp"
#include <system_error>

namespace mino::core::file {

    // file_metadata 비교 연산자 정의
    bool file_metadata::operator==(const file_metadata& rhs) const noexcept {
        return last_write_time == rhs.last_write_time && file_size == rhs.file_size;
    }

    bool file_metadata::operator!=(const file_metadata& rhs) const noexcept {
        return !(*this == rhs);
    }

    file_watcher::file_watcher()
        : interval_(std::chrono::milliseconds(1000)),
        is_running_(false) {
    }

    file_watcher::~file_watcher() {
        stop();
    }

    bool file_watcher::set_target_path(const std::filesystem::path& target_path) {
        namespace fs = std::filesystem;

        if (is_running_) {
            return false;
        }

        std::error_code ec;
        auto status = fs::status(target_path, ec);
        if (ec || !fs::exists(status)) {
            return false;
        }

        if (fs::is_regular_file(status)) {
            [[maybe_unused]] auto last_write = fs::last_write_time(target_path, ec);
            if (ec) {
                return false;
            }
        }
        else if (fs::is_directory(status)) {
            using directory_iterator = fs::directory_iterator;
            using directory_options = fs::directory_options;
            auto skip_permission_denied = directory_options::skip_permission_denied;

            directory_iterator test_iter(target_path, skip_permission_denied, ec);
            if (ec) {
                return false;
            }
        }
        else {
            return false;
        }

        target_path_ = target_path;
        return true;
    }

    bool file_watcher::set_interval(std::chrono::milliseconds interval) {
        if (is_running_ || interval.count() <= 0) {
            return false;
        }
        interval_ = interval;
        return true;
    }

    std::optional<std::string> file_watcher::start(callback_type callback) {
        namespace fs = std::filesystem;

        if (is_running_) {
            return "File watcher is already running.";
        }

        if (target_path_.empty()) {
            return "Target path has not been set.";
        }

        if (!callback) {
            return "Callback function is empty or invalid.";
        }

        std::error_code ec;
        if (!fs::exists(target_path_, ec)) {
            if (ec) {
                return "Failed to access target path: " + ec.message();
            }
            return "Target path does not exist: " + target_path_.string();
        }

        paths_cache_.clear();
        build_snapshot(paths_cache_);

        is_running_ = true;
        worker_thread_ = std::thread(&file_watcher::monitor_loop, this, std::move(callback));
        return std::nullopt;
    }

    void file_watcher::stop() {
        if (!is_running_) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(cv_mutex_);
            is_running_ = false;
        }
        cv_.notify_all();

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    bool file_watcher::is_running() const noexcept {
        return is_running_;
    }

    const std::filesystem::path& file_watcher::get_target_path() const noexcept {
        return target_path_;
    }

    std::chrono::milliseconds file_watcher::get_interval() const noexcept {
        return interval_;
    }

    void file_watcher::scan_directory(const std::filesystem::path& dir_path,
        std::unordered_set<std::string>& visited_canonical_dirs,
        std::unordered_map<std::string, file_metadata>& out_cache) {
        namespace fs = std::filesystem;

        std::error_code ec;

        fs::path canonical_dir = fs::canonical(dir_path, ec);
        if (ec) {
            return;
        }

        if (!visited_canonical_dirs.insert(canonical_dir.string()).second) {
            return;
        }

        using directory_options = fs::directory_options;
        auto skip_permission_denied = directory_options::skip_permission_denied;
        fs::directory_iterator iter(dir_path, skip_permission_denied, ec);
        if (ec) {
            return;
        }

        for (const auto& entry : iter) {
            auto status = entry.status(ec);
            if (ec) {
                continue;
            }

            if (fs::is_directory(status)) {
                scan_directory(entry.path(), visited_canonical_dirs, out_cache);
            }
            else if (fs::is_regular_file(status)) {
                auto lwt = entry.last_write_time(ec);
                if (ec) {
                    continue;
                }

                auto sz = entry.file_size(ec);
                if (ec) {
                    sz = 0;
                }

                out_cache[entry.path().string()] = file_metadata{ lwt, sz };
            }
        }
    }

    void file_watcher::build_snapshot(std::unordered_map<std::string, file_metadata>& out_cache) {
        namespace fs = std::filesystem;

        std::error_code ec;
        auto status = fs::status(target_path_, ec);
        if (ec || !fs::exists(status)) {
            return;
        }

        if (fs::is_regular_file(status)) {
            auto lwt = fs::last_write_time(target_path_, ec);
            if (!ec) {
                auto sz = fs::file_size(target_path_, ec);
                if (ec) {
                    sz = 0;
                }
                out_cache[target_path_.string()] = file_metadata{ lwt, sz };
            }
        }
        else if (fs::is_directory(status)) {
            std::unordered_set<std::string> visited_canonical_dirs;
            scan_directory(target_path_, visited_canonical_dirs, out_cache);
        }
    }

    void file_watcher::monitor_loop(callback_type callback) {
        while (is_running_) {
            std::unique_lock<std::mutex> lock(cv_mutex_);
            if (cv_.wait_for(lock, interval_, [this] { return !is_running_; })) {
                break;
            }

            check_for_changes(callback);
        }
    }

    void file_watcher::check_for_changes(const callback_type& callback) {
        namespace fs = std::filesystem;

        std::unordered_map<std::string, file_metadata> current_cache;
        build_snapshot(current_cache);

        // 1. 삭제 감지
        for (auto it = paths_cache_.begin(); it != paths_cache_.end();) {
            if (current_cache.find(it->first) == current_cache.end()) {
                callback(std::filesystem::path(it->first), file_status::erased);
                it = paths_cache_.erase(it);
            }
            else {
                ++it;
            }
        }

        // 2. 생성 및 수정 감지
        for (const auto& [file_path_str, meta] : current_cache) {
            auto it = paths_cache_.find(file_path_str);
            if (it == paths_cache_.end()) {
                paths_cache_[file_path_str] = meta;
                callback(fs::path(file_path_str), file_status::created);
            }
            else if (it->second != meta) {
                it->second = meta;
                callback(fs::path(file_path_str), file_status::modified);
            }
        }
    }

} // namespace mino::core::file
