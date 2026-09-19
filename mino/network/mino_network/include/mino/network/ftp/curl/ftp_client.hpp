#pragma once

#ifdef USE_CURL

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdint>

typedef void CURL;

namespace mino::network::ftp::curl {

    // CURL이 지원하는 프로토콜을 반환하는 함수 (디버깅용)
    std::string get_curl_supported_protocols();

    // 파일 정보 구조체
    struct file_info {
        std::string name; // 파일 또는 디렉토리 이름
        long size; // 파일 크기 (바이트 단위, 디렉토리인 경우 0)
        bool is_directory; // 디렉토리 여부 (true: 디렉토리, false: 파일)
    };

    // Progress listener interface
    class i_progress_listener {
    public:
        virtual ~i_progress_listener() {}

        // Progress update callback
        // dlnow: bytes currently downloaded
        // dltotal: total bytes to download
        // ulnow: bytes currently uploaded
        // ultotal: total bytes to upload
        virtual void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) = 0;
    };

    // Default progress listener
    class default_progress_listener : public i_progress_listener {
    public:
        void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) override;
    };

    // Filtered progress listener (declaration)
    // - Wraps another i_progress_listener and forwards events to the inner listener
    //   only according to a forwarding policy (e.g. 1% change or a minimum time interval).
    class filtered_progress_listener : public i_progress_listener {
    private:
        // Prefer owning inner_shared; if absent, use inner_raw (non-owning).
        std::shared_ptr<i_progress_listener> inner_shared;
        i_progress_listener* inner_raw{ nullptr };

        int min_percent; // Minimum percent increase to forward (e.g. 1)
        int min_ms;      // Minimum time interval (ms) to forward even if percent hasn't changed

        int last_dl_percent{0};
        int last_ul_percent{0};
        std::chrono::steady_clock::time_point last_forward{ std::chrono::steady_clock::now() };

        i_progress_listener* inner() const noexcept {
            return inner_shared ? inner_shared.get() : inner_raw;
        }

    public:
        // default ctor (no target yet)
        filtered_progress_listener() noexcept;

        // non-owning constructor (raw pointer)
        explicit filtered_progress_listener(i_progress_listener* target,
            int min_percent_ = 1,
            int min_ms_ = 100) noexcept;

        // owning constructor (shared_ptr)
        explicit filtered_progress_listener(std::shared_ptr<i_progress_listener> target,
            int min_percent_ = 1,
            int min_ms_ = 100) noexcept;

        // set target later (non-owning)
        void set_target(i_progress_listener* target) noexcept;

        // set target later (owning)
        void set_target(std::shared_ptr<i_progress_listener> target) noexcept;

        // optionally allow changing policy after construction
        void set_policy(int min_percent_, int min_ms_) noexcept;

        // i_progress_listener override
        void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) override;
    };

    // FTP/SFTP base class
    class ftp_client_base {
    protected:
        CURL* curl_handle;
        std::string host_name;
        int port;
        std::string user_name;
        std::string password;
        std::string last_error;
        i_progress_listener* progress_listener;

        static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
        static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream);

        // CURL progress callback function
        static int progress_callback_internal(void* clientp, std::int64_t dltotal, std::int64_t dlnow,
            std::int64_t ultotal, std::int64_t ulnow);

    public:
        ftp_client_base();
        virtual ~ftp_client_base();

        std::string get_last_error() const;

        // Register progress listener
        void set_progress_listener(i_progress_listener* listener);

        // Remove progress listener
        void remove_progress_listener();

        virtual bool connect(const std::string& host, int p,
            const std::string& user, const std::string& pass) = 0;
        virtual bool upload(const std::string& local_file, const std::string& remote_file) = 0;
        virtual bool download(const std::string& remote_file, const std::string& local_file) = 0;
        virtual bool delete_file(const std::string& remote_file) = 0;
        virtual std::vector<file_info> list_directory(const std::string& path) = 0;
        virtual bool create_directory(const std::string& path) = 0;
        virtual bool remove_directory(const std::string& path) = 0;
    };

    // FTP client class
    class ftp_client : public ftp_client_base {
    public:
        ftp_client();

        bool connect(const std::string& host, int p = 21,
            const std::string& user = "anonymous",
            const std::string& pass = "") override;

        bool upload(const std::string& local_file, const std::string& remote_file) override;
        bool download(const std::string& remote_file, const std::string& local_file) override;
        bool delete_file(const std::string& remote_file) override;
        std::vector<file_info> list_directory(const std::string& path) override;
        bool create_directory(const std::string& path) override;
        bool remove_directory(const std::string& path) override;
    };

    // SFTP client class
    class sftp_client : public ftp_client_base {
    public:
        sftp_client();

        bool connect(const std::string& host, int p = 22,
            const std::string& user = "",
            const std::string& pass = "") override;

        bool upload(const std::string& local_file, const std::string& remote_file) override;
        bool download(const std::string& remote_file, const std::string& local_file) override;
        bool delete_file(const std::string& remote_file) override;
        std::vector<file_info> list_directory(const std::string& path) override;
        bool create_directory(const std::string& path) override;
        bool remove_directory(const std::string& path) override;
    };
}  

#endif // USE_CURL
