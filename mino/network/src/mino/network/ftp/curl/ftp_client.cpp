#ifdef USE_CURL

#include <cstdint>
#include <string>
#include <sstream>

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

// --- default_progress_listener implementation ---
void default_progress_listener::on_progress(std::int64_t dlnow, std::int64_t dltotal,
    std::int64_t ulnow, std::int64_t ultotal) {
    // Simple console output (extend if needed)
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

// --- filtered_progress_listener implementation ---, last_ul_percent(-1)
filtered_progress_listener::filtered_progress_listener() noexcept
    : inner_shared(nullptr),
    inner_raw(nullptr),
    min_percent(1),
    min_ms(100),
    last_dl_percent(-1),
    last_ul_percent(-1),
    last_forward(std::chrono::steady_clock::now()) {
}

// non-owning ctor
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

// owning ctor
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

// set non-owning target
void filtered_progress_listener::set_target(i_progress_listener* target) noexcept {
    inner_shared.reset();
    inner_raw = target;
    last_dl_percent = last_ul_percent = -1;
    last_forward = std::chrono::steady_clock::now();
}

// set owning target
void filtered_progress_listener::set_target(std::shared_ptr<i_progress_listener> target) noexcept {
    inner_shared = std::move(target);
    inner_raw = nullptr;
    last_dl_percent = last_ul_percent = -1;
    last_forward = std::chrono::steady_clock::now();
}

// set policy
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

    // Download percent check
    if (dltotal > 0) {
        int pct = static_cast<int>((dlnow * 100) / dltotal);
        if (last_dl_percent < 0 || pct >= last_dl_percent + min_percent) {
            last_dl_percent = pct;
            forward = true;
        }
    }

    // Upload percent check
    if (ultotal > 0) {
        int pct = static_cast<int>((ulnow * 100) / ultotal);
        if (last_ul_percent < 0 || pct >= last_ul_percent + min_percent) {
            last_ul_percent = pct;
            forward = true;
        }
    }

    // Time-based forward (prevent silence)
    if (!forward && elapsed_ms >= min_ms) {
        forward = true;
    }

    if (forward) {
        last_forward = now;
        target->on_progress(dlnow, dltotal, ulnow, ultotal);
    }
} 

// --- ftp_client_base basic callback stubs ---
size_t ftp_client_base::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    // Basic stub: if userp is std::ostream*, write to it
    std::ostream* os = static_cast<std::ostream*>(userp);
    size_t total = size * nmemb;
    if (os) {
        os->write(static_cast<char*>(contents), static_cast<std::streamsize>(total));
        return total;
    }
    return total; // Adjust on failure if necessary
}

size_t ftp_client_base::read_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
    // Basic stub: if stream is std::istream*, read from it
    std::istream* is = static_cast<std::istream*>(stream);
    size_t total = size * nmemb;
    if (is) {
        is->read(static_cast<char*>(ptr), static_cast<std::streamsize>(total));
        return static_cast<size_t>(is->gcount());
    }
    return 0;
}

int ftp_client_base::progress_callback_internal(void* clientp, std::int64_t dltotal, std::int64_t dlnow,
    std::int64_t ultotal, std::int64_t ulnow) {
    if (!clientp) return 0;
    ftp_client_base* self = static_cast<ftp_client_base*>(clientp);
    if (self->progress_listener) {
        // Forward: libcurl provides (dltotal, dlnow, ultotal, ulnow) — align order with header
        self->progress_listener->on_progress(dlnow, dltotal, ulnow, ultotal);
    }
    return 0; // return 0 to continue transfer
}

// --- ftp_client_base basic implementations (stubs) ---
ftp_client_base::ftp_client_base() : curl_handle(nullptr), port(0), progress_listener(nullptr) {}
ftp_client_base::~ftp_client_base() {}
std::string ftp_client_base::get_last_error() const { return last_error; }
void ftp_client_base::set_progress_listener(i_progress_listener* listener) { progress_listener = listener; }
void ftp_client_base::remove_progress_listener() { progress_listener = nullptr; }

// --- ftp_client minimal stub implementations (replace with real implementations as needed) ---
ftp_client::ftp_client() = default;

bool ftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    // Replace with real implementation if available.
    host_name = host;
    port = p;
    user_name = user;
    password = pass;
    last_error.clear();
    // Stub: not attempting real connection — real implementation required
    last_error = "ftp_client::connect() not implemented in this stub.";
    return false;
}

bool ftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    last_error = std::string("ftp_client::upload() not implemented in this stub. remote_file: ")
        + remote_file + ", local_file: " + local_file;
    return false;
}

bool ftp_client::download(const std::string& remote_file, const std::string& local_file) {
    last_error = std::string("ftp_client::download() not implemented in this stub. remote_file: ")
        + remote_file + ", local_file: " + local_file;
    return false;
}

bool ftp_client::delete_file(const std::string& remote_file) {
    last_error = std::string("ftp_client::delete_file() not implemented in this stub. remote_file: ")
        + remote_file;
    return false;
}

std::vector<file_info> ftp_client::list_directory(const std::string& path) {
    last_error = std::string("ftp_client::list_directory() not implemented in this stub. path: ") + path;
    return {};
}

bool ftp_client::create_directory(const std::string& path) {
    last_error = std::string("ftp_client::create_directory() not implemented in this stub. path: ") + path;
    return false;
}

bool ftp_client::remove_directory(const std::string& path) {
    last_error = std::string("ftp_client::remove_directory() not implemented in this stub. path: ") + path;
    return false;
}

// --- sftp_client minimal stubs ---
sftp_client::sftp_client() = default;

bool sftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    host_name = host;
    port = p;
    user_name = user;
    password = pass;
    last_error.clear();
    last_error = "sftp_client::connect() not implemented in this stub.";
    return false;
}

bool sftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    last_error = std::string("sftp_client::upload() not implemented in this stub. remote_file: ")
        + remote_file + ", local_file: " + local_file;
    return false;
}

bool sftp_client::download(const std::string& remote_file, const std::string& local_file) {
    last_error = std::string("sftp_client::download() not implemented in this stub. remote_file: ")
        + remote_file + ", local_file: " + local_file;
    return false;
}

bool sftp_client::delete_file(const std::string& remote_file) {
    last_error = std::string("sftp_client::delete_file() not implemented in this stub. remote_file: ")
        + remote_file;
    return false;
}

std::vector<file_info> sftp_client::list_directory(const std::string& path) {
    last_error = std::string("sftp_client::list_directory() not implemented in this stub. path: ") + path;
    return {};
}

bool sftp_client::create_directory(const std::string& path) {
    last_error = std::string("sftp_client::create_directory() not implemented in this stub. path: ") + path;
    return false;
}

bool sftp_client::remove_directory(const std::string& path) {
    last_error = std::string("sftp_client::remove_directory() not implemented in this stub. path: ") + path;
    return false;
}

#endif // USE_CURL 
