#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <condition_variable>

// USE_OPENSSL 매크로가 있을 때만 OpenSSL 구조체 전방 선언
#ifdef USE_OPENSSL
    typedef struct ssl_ctx_st SSL_CTX;
    typedef struct ssl_st SSL;
#endif

#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::ftp::tcp {

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
        virtual void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) = 0; //
    };

    // Default progress listener
    class default_progress_listener : public i_progress_listener {
    public:
        void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) override; //
    };

    // Filtered progress listener
    class filtered_progress_listener : public i_progress_listener {
    private:
        std::shared_ptr<i_progress_listener> inner_shared; //
        i_progress_listener* inner_raw{ nullptr }; //

        int min_percent; //
        int min_ms;      //

        int last_dl_percent; //
        int last_ul_percent; //
        std::chrono::steady_clock::time_point last_forward; //

        i_progress_listener* inner() const noexcept {
            return inner_shared ? inner_shared.get() : inner_raw; //
        }

    public:
        filtered_progress_listener() noexcept; //
        explicit filtered_progress_listener(i_progress_listener* target,
            int min_percent_ = 1, int min_ms_ = 100) noexcept; //
        explicit filtered_progress_listener(std::shared_ptr<i_progress_listener> target,
            int min_percent_ = 1, int min_ms_ = 100) noexcept; //

        void set_target(i_progress_listener* target) noexcept; //
        void set_target(std::shared_ptr<i_progress_listener> target) noexcept; //
        void set_policy(int min_percent_, int min_ms_) noexcept; //
        void on_progress(std::int64_t dlnow, std::int64_t dltotal,
            std::int64_t ulnow, std::int64_t ultotal) override; //
    };

    // FTP/SFTP base class
    class ftp_client_base {
    protected:
        std::string host_name; //
        int port; //
        std::string user_name; //
        std::string password; //
        std::string last_error; //
        i_progress_listener* progress_listener; //

        std::mutex response_mutex;
        std::condition_variable response_cv;
        std::string response_buffer;
        bool has_response{ false };

        std::unique_ptr<mino::network::tcp::tcp_client> control_client;

        // [변경] FTPS 확장용 변수들도 OpenSSL 매크로 내부에 포함
#ifdef USE_OPENSSL
        SSL_CTX* ssl_ctx{ nullptr };
        SSL* ssl{ nullptr };
#endif

    public:
        ftp_client_base();
        virtual ~ftp_client_base();

        std::string get_last_error() const; //
        void set_progress_listener(i_progress_listener* listener); //
        void remove_progress_listener(); //

        virtual bool connect(const std::string& host, int p,
            const std::string& user, const std::string& pass) = 0; //
        virtual bool upload(const std::string& local_file, const std::string& remote_file) = 0; //
        virtual bool download(const std::string& remote_file, const std::string& local_file) = 0; //
        virtual bool delete_file(const std::string& remote_file) = 0; //
        virtual std::vector<file_info> list_directory(const std::string& path) = 0; //
        virtual bool create_directory(const std::string& path) = 0; //
        virtual bool remove_directory(const std::string& path) = 0; //
    };

    // FTP client class
    class ftp_client : public ftp_client_base {
    private:
        std::string read_control_response(std::chrono::seconds timeout = std::chrono::seconds(5));
        bool send_command(const std::string& cmd, const std::string& arg = "");
        std::unique_ptr<mino::network::tcp::tcp_client> establish_data_connection();
        std::int64_t get_remote_file_size(const std::string& remote_file);

    public:
        ftp_client();
        ~ftp_client() override;

        bool connect(const std::string& host, int p = 21,
            const std::string& user = "anonymous",
            const std::string& pass = "") override; //

        bool upload(const std::string& local_file, const std::string& remote_file) override; //
        bool download(const std::string& remote_file, const std::string& local_file) override; //
        bool delete_file(const std::string& remote_file) override; //
        std::vector<file_info> list_directory(const std::string& path) override; //
        bool create_directory(const std::string& path) override; //
        bool remove_directory(const std::string& path) override; //
    };

    // SFTP client class
    class sftp_client : public ftp_client_base {
    public:
        sftp_client();
        ~sftp_client() override;

        bool connect(const std::string& host, int p = 22,
            const std::string& user = "",
            const std::string& pass = "") override; //

        bool upload(const std::string& local_file, const std::string& remote_file) override; //
        bool download(const std::string& remote_file, const std::string& local_file) override; //
        bool delete_file(const std::string& remote_file) override; //
        std::vector<file_info> list_directory(const std::string& path) override; //
        bool create_directory(const std::string& path) override; //
        bool remove_directory(const std::string& path) override; //
    };

}  
