#pragma once

#ifdef USE_CURL

#include <string>
#include <functional>
#include <filesystem>

#include <curl/curl.h>

namespace mino::network_curl::downloader
{

    class file_downloader {
    public:
        using progress_callback = std::function<bool(curl_off_t, curl_off_t)>;

        file_downloader();
        ~file_downloader();

        void set_ssl_verify(bool verify);
        void set_progress_callback(progress_callback cb);

        // 수동 경로(풀 경로 포함) 지정 다운로드
        bool download(const std::string& url, const std::filesystem::path& output_path);

        // 자동 이름 설정 다운로드 (저장 디렉토리만 지정, 성공 시 최종 파일 경로 반환, 실패 시 빈 경로 반환)
        std::filesystem::path download_auto_name(const std::string& url, const std::filesystem::path& output_directory);

        // 마지막으로 발생한 에러 메시지 반환
        std::string get_last_error() const;

    private:
        bool ssl_verify_;
        progress_callback progress_cb_;
        std::string last_error_;

        static size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata);
        static int progress_callback_trampoline(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

        bool execute_download(const std::string& url, const std::filesystem::path& output_path);
        std::string extract_filename_from_url(const std::string& url);
    };

} // namespace mino::network_curl::downloader

#endif // USE_CURL
