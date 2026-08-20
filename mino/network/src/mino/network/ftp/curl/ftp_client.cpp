#ifdef USE_CURL

#include <cstdint>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <curl/curl.h>

#include "mino/network/ftp/curl/ftp_client.hpp"

using namespace mino::network::ftp::curl;

namespace mino::network::ftp::curl {

    std::string get_curl_supported_protocols() {
        std::ostringstream oss;
        curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
        if (data && data->protocols) {
            for (const char* const* proto = data->protocols; *proto; ++proto) {
                if (oss.tellp() > 0) {
                    oss << '\n';
                }
                oss << *proto;
            }
        }
        return oss.str();
    }

}

// --- default_progress_listener 구현 ---
void default_progress_listener::on_progress(std::int64_t dlnow, std::int64_t dltotal,
    std::int64_t ulnow, std::int64_t ultotal) {
    if (dlnow < 0 || dltotal < 0 || ulnow < 0 || ultotal < 0) {
        std::cout << "[Default] Progress: [Invalid data]\n";
        return;
    }

    if (dltotal > 0) {
        double pct = (static_cast<double>(dlnow) / dltotal) * 100.0;
        std::cout << "[Default] Download: " << std::fixed << std::setprecision(2) << pct
            << "% (" << dlnow << "/" << dltotal << ")\n";
    }
    if (ultotal > 0) {
        double pct = (static_cast<double>(ulnow) / ultotal) * 100.0;
        std::cout << "[Default] Upload: " << std::fixed << std::setprecision(2) << pct
            << "% (" << ulnow << "/" << ultotal << ")\n";
    }
}

// --- filtered_progress_listener 구현 ---
filtered_progress_listener::filtered_progress_listener() noexcept
    : inner_shared(nullptr),
    inner_raw(nullptr),
    min_percent(1),
    min_ms(100),
    last_dl_percent(-1),
    last_ul_percent(-1),
    last_forward(std::chrono::steady_clock::now()) {
}

filtered_progress_listener::filtered_progress_listener(i_progress_listener* target,
    int min_percent_,
    int min_ms_) noexcept
    : inner_shared(nullptr),
    inner_raw(target),
    min_percent(min_percent_),
    min_ms(min_ms_),
    last_dl_percent(-1),
    last_ul_percent(-1),
    last_forward(std::chrono::steady_clock::now()) {
}

filtered_progress_listener::filtered_progress_listener(std::shared_ptr<i_progress_listener> target,
    int min_percent_,
    int min_ms_) noexcept
    : inner_shared(std::move(target)),
    inner_raw(nullptr),
    min_percent(min_percent_),
    min_ms(min_ms_),
    last_dl_percent(-1),
    last_ul_percent(-1),
    last_forward(std::chrono::steady_clock::now()) {
}

void filtered_progress_listener::set_target(i_progress_listener* target) noexcept {
    inner_shared.reset();
    inner_raw = target;
    last_dl_percent = last_ul_percent = -1;
    last_forward = std::chrono::steady_clock::now();
}

void filtered_progress_listener::set_target(std::shared_ptr<i_progress_listener> target) noexcept {
    inner_shared = std::move(target);
    inner_raw = nullptr;
    last_dl_percent = last_ul_percent = -1;
    last_forward = std::chrono::steady_clock::now();
}

void filtered_progress_listener::set_policy(int min_percent_, int min_ms_) noexcept {
    min_percent = min_percent_;
    min_ms = min_ms_;
}

void filtered_progress_listener::on_progress(std::int64_t dlnow, std::int64_t dltotal,
    std::int64_t ulnow, std::int64_t ultotal) {
    i_progress_listener* target = inner();
    if (!target) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_forward).count();

    bool forward = false;

    if (dltotal > 0) {
        int pct = static_cast<int>((dlnow * 100) / dltotal);
        if (last_dl_percent < 0 || pct >= last_dl_percent + min_percent) {
            last_dl_percent = pct;
            forward = true;
        }
    }

    if (ultotal > 0) {
        int pct = static_cast<int>((ulnow * 100) / ultotal);
        if (last_ul_percent < 0 || pct >= last_ul_percent + min_percent) {
            last_ul_percent = pct;
            forward = true;
        }
    }

    if (!forward && elapsed_ms >= min_ms) {
        forward = true;
    }

    if (forward) {
        last_forward = now;
        target->on_progress(dlnow, dltotal, ulnow, ultotal);
    }
}

// --- ftp_client_base 콜백 함수 ---
size_t ftp_client_base::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ostream* os = static_cast<std::ostream*>(userp);
    size_t total = size * nmemb;
    if (os) {
        os->write(static_cast<char*>(contents), static_cast<std::streamsize>(total));
        return total;
    }
    return 0;
}

size_t ftp_client_base::read_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::istream* is = static_cast<std::istream*>(stream);
    size_t max_to_read = size * nmemb;
    if (is && is->good()) {
        is->read(static_cast<char*>(ptr), static_cast<std::streamsize>(max_to_read));
        return static_cast<size_t>(is->gcount());
    }
    return 0;
}

int ftp_client_base::progress_callback_internal(void* clientp, std::int64_t dltotal, std::int64_t dlnow,
    std::int64_t ultotal, std::int64_t ulnow) {
    if (!clientp) return 0;
    ftp_client_base* self = static_cast<ftp_client_base*>(clientp);
    if (self->progress_listener) {
        self->progress_listener->on_progress(dlnow, dltotal, ulnow, ultotal);
    }
    return 0;
}

// --- ftp_client_base 기본 구현 ---
ftp_client_base::ftp_client_base() : curl_handle(nullptr), port(0), progress_listener(nullptr) {
    curl_handle = curl_easy_init();
}

ftp_client_base::~ftp_client_base() {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
        curl_handle = nullptr;
    }
}

std::string ftp_client_base::get_last_error() const { return last_error; }
void ftp_client_base::set_progress_listener(i_progress_listener* listener) { progress_listener = listener; }
void ftp_client_base::remove_progress_listener() { progress_listener = nullptr; }

// --- ftp_client 실제 구현 ---
ftp_client::ftp_client() : ftp_client_base() {}

bool ftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    host_name = host;
    port = p;
    user_name = user;
    password = pass;
    last_error.clear();

    if (!curl_handle) {
        last_error = "CURL 핸들 초기화 실패";
        return false;
    }

    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/";
    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool ftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    last_error.clear();
    std::ifstream file(local_file, std::ios::binary);
    if (!file.is_open()) {
        last_error = "로컬 파일을 열 수 없습니다: " + local_file;
        return false;
    }

    std::string remote_path = remote_file;
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }
    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/" + remote_path;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &file);

    if (progress_listener) {
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback_internal);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, this);
    }

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool ftp_client::download(const std::string& remote_file, const std::string& local_file) {
    last_error.clear();
    std::ofstream file(local_file, std::ios::binary);
    if (!file.is_open()) {
        last_error = "로컬 파일을 생성할 수 없습니다: " + local_file;
        return false;
    }

    std::string remote_path = remote_file;
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }
    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/" + remote_path;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &file);

    if (progress_listener) {
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback_internal);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, this);
    }

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool ftp_client::delete_file(const std::string& remote_file) {
    last_error.clear();
    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/";
    std::string cmd = "DELE " + remote_file;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

std::vector<file_info> ftp_client::list_directory(const std::string& path) {
    last_error.clear();
    std::string remote_path = path;
    if (!remote_path.empty() && remote_path.back() != '/') {
        remote_path += "/";
    }
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }

    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/" + remote_path;
    std::ostringstream response;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return {};
    }

    std::vector<file_info> results;
    std::istringstream iss(response.str());
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            file_info info;
            info.name = line;
            info.size = 0;
            info.is_directory = false;
            results.push_back(info);
        }
    }
    return results;
}

bool ftp_client::create_directory(const std::string& path) {
    last_error.clear();
    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/";
    std::string cmd = "MKD " + path;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool ftp_client::remove_directory(const std::string& path) {
    last_error.clear();
    std::string url = "ftp://" + host_name + ":" + std::to_string(port) + "/";
    std::string cmd = "RMD " + path;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

// --- sftp_client 실제 구현 ---
sftp_client::sftp_client() : ftp_client_base() {}

bool sftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    host_name = host;
    port = p;
    user_name = user;
    password = pass;
    last_error.clear();

    if (!curl_handle) {
        last_error = "CURL 핸들 초기화 실패";
        return false;
    }

    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/";
    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    std::ostringstream dummy;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dummy);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool sftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    last_error.clear();
    std::ifstream file(local_file, std::ios::binary);
    if (!file.is_open()) {
        last_error = "로컬 파일을 열 수 없습니다: " + local_file;
        return false;
    }

    std::string remote_path = remote_file;
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }
    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/" + remote_path;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &file);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    if (progress_listener) {
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback_internal);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, this);
    }

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool sftp_client::download(const std::string& remote_file, const std::string& local_file) {
    last_error.clear();
    std::ofstream file(local_file, std::ios::binary);
    if (!file.is_open()) {
        last_error = "로컬 파일을 생성할 수 없습니다: " + local_file;
        return false;
    }

    std::string remote_path = remote_file;
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }
    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/" + remote_path;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    if (progress_listener) {
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback_internal);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, this);
    }

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool sftp_client::delete_file(const std::string& remote_file) {
    last_error.clear();
    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/";
    std::string cmd = "rm " + remote_file;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    std::ostringstream dummy;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dummy);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

std::vector<file_info> sftp_client::list_directory(const std::string& path) {
    last_error.clear();
    std::string remote_path = path;
    if (!remote_path.empty() && remote_path.back() != '/') {
        remote_path += "/";
    }
    if (!remote_path.empty() && remote_path[0] == '/') {
        remote_path = remote_path.substr(1);
    }

    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/" + remote_path;
    std::ostringstream response;

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return {};
    }

    std::vector<file_info> results;
    std::istringstream iss(response.str());
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            file_info info;
            info.name = line;
            info.size = 0;
            info.is_directory = false;
            results.push_back(info);
        }
    }
    return results;
}

bool sftp_client::create_directory(const std::string& path) {
    last_error.clear();
    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/";
    std::string cmd = "mkdir " + path;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    std::ostringstream dummy;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dummy);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool sftp_client::remove_directory(const std::string& path) {
    last_error.clear();
    std::string url = "sftp://" + host_name + ":" + std::to_string(port) + "/~/";
    std::string cmd = "rmdir " + path;

    struct curl_slist* headerlist = curl_slist_append(nullptr, cmd.c_str());

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, user_name.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);
    curl_easy_setopt(curl_handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

    std::ostringstream dummy;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dummy);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headerlist);

    if (res != CURLE_OK) {
        last_error = curl_easy_strerror(res);
        return false;
    }
    return true;
}

#endif // USE_CURL
