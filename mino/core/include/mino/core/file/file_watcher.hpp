#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace mino::core::file {

    // 파일 상태 변경 유형
    enum class file_status {
        created,
        modified,
        erased
    };

    // 파일 변경 감지용 메타데이터 (시간 + 크기)
    struct file_metadata {
        std::filesystem::file_time_type last_write_time{};
        std::uintmax_t file_size{ 0 };

        bool operator==(const file_metadata& rhs) const noexcept;
        bool operator!=(const file_metadata& rhs) const noexcept;
    };

    class file_watcher {
    public:
        using callback_type = std::function<void(const std::filesystem::path&, file_status)>;

        file_watcher();
        ~file_watcher();

        file_watcher(const file_watcher&) = delete;
        file_watcher& operator=(const file_watcher&) = delete;
        file_watcher(file_watcher&&) noexcept = delete;
        file_watcher& operator=(file_watcher&&) noexcept = delete;

        // 감시 대상 경로 설정 (접근 불가, 권한 없음, 미존재 시 false 반환)
        bool set_target_path(const std::filesystem::path& target_path);

        // 폴링 주기 설정 (0 이하이거나 실행 중 변경 시 false 반환)
        bool set_interval(std::chrono::milliseconds interval);

        // 감시 시작: 성공 시 std::nullopt, 실패 시 영문 오류 메시지 반환
        std::optional<std::string> start(callback_type callback);

        // 감시 중지
        void stop();

        bool is_running() const noexcept;
        const std::filesystem::path& get_target_path() const noexcept;
        std::chrono::milliseconds get_interval() const noexcept;

    private:
        void scan_directory(const std::filesystem::path& dir_path,
            std::unordered_set<std::string>& visited_canonical_dirs,
            std::unordered_map<std::string, file_metadata>& out_cache);

        void build_snapshot(std::unordered_map<std::string, file_metadata>& out_cache);

        void monitor_loop(callback_type callback);

        void check_for_changes(const callback_type& callback);

        std::filesystem::path target_path_;
        std::chrono::milliseconds interval_;
        bool is_running_;
        std::thread worker_thread_;
        std::mutex cv_mutex_;
        std::condition_variable cv_;
        std::unordered_map<std::string, file_metadata> paths_cache_;
    };

} // namespace mino::core::file
