#ifdef USE_CURL

#include "mino/network_curl/downloader/file_downloader.hpp"

#include <fstream>
#include <algorithm>

namespace mino::network_curl::downloader
{

    file_downloader::file_downloader() : ssl_verify_(false) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    file_downloader::~file_downloader() {
        curl_global_cleanup();
    }

    void file_downloader::set_ssl_verify(bool verify) {
        ssl_verify_ = verify;
    }

    void file_downloader::set_progress_callback(progress_callback cb) {
        progress_cb_ = cb;
    }

    std::string file_downloader::get_last_error() const {
        return last_error_;
    }

    size_t file_downloader::write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* stream = static_cast<std::ofstream*>(userdata);
        size_t written = size * nmemb;
        stream->write(static_cast<char*>(ptr), written);
        return stream->good() ? written : 0;
    }

    int file_downloader::progress_callback_trampoline(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        auto* downloader = static_cast<file_downloader*>(clientp);
        if (downloader && downloader->progress_cb_) {
            bool continue_download = downloader->progress_cb_(dlnow, dltotal);
            return continue_download ? 0 : 1; // false 반환 시 다운로드 강제 중단
        }
        return 0;
    }

    std::string file_downloader::extract_filename_from_url(const std::string& url) {
        std::string clean_url = url;
        auto query_pos = clean_url.find_first_of("?#");
        if (query_pos != std::string::npos) {
            clean_url = clean_url.substr(0, query_pos);
        }

        std::filesystem::path path(clean_url);
        std::string filename = path.filename().string();

        if (filename.empty() || filename == "/" || filename == "\\") {
            filename = "downloaded_file";
        }
        return filename;
    }

    bool file_downloader::execute_download(const std::string& url, const std::filesystem::path& output_path) {
        last_error_.clear();

        CURL* curl = curl_easy_init();
        if (!curl) {
            last_error_ = "Failed to initialize CURL handle.";
            return false;
        }

        if (output_path.has_parent_path() && !output_path.parent_path().empty()) {
            std::error_code ec;
            std::filesystem::create_directories(output_path.parent_path(), ec);
        }

        std::ofstream file(output_path, std::ios::binary);
        if (!file.is_open()) {
            last_error_ = "Failed to open local file for writing: " + output_path.string();
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback_trampoline);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);

        if (!ssl_verify_) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        }

        CURLcode res = curl_easy_perform(curl);

        file.close();
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            if (res == CURLE_ABORTED_BY_CALLBACK) {
                last_error_ = "Download aborted by progress callback.";
            }
            else {
                last_error_ = curl_easy_strerror(res);
            }

            std::error_code ec;
            if (std::filesystem::exists(output_path)) {
                std::filesystem::remove(output_path, ec);
            }

            return false;
        }

        return true;
    }

    bool file_downloader::download(const std::string& url, const std::filesystem::path& output_path) {
        return execute_download(url, output_path);
    }

    std::filesystem::path file_downloader::download_auto_name(const std::string& url, const std::filesystem::path& output_directory) {
        std::string filename = extract_filename_from_url(url);
        std::filesystem::path final_path = output_directory / filename;

        if (execute_download(url, final_path)) {
            return final_path;
        }

        return {};
    }

}  

#endif // USE_CURL
