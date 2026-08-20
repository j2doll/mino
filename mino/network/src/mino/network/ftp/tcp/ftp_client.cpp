
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <thread>
#include <chrono>

#ifdef _WIN32
#   include <winsock2.h>
#   include <ws2tcpip.h>
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <unistd.h>
#   include <netdb.h>
#endif

#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef USE_OPENSSL
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#endif

#include "mino/network/ftp/tcp/ftp_client.hpp"

using namespace mino::network::ftp::tcp;

// --- default_progress_listener implementation ---
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

// --- filtered_progress_listener implementation ---
filtered_progress_listener::filtered_progress_listener() noexcept
    : inner_shared(nullptr), inner_raw(nullptr), min_percent(1), min_ms(100),
    last_dl_percent(-1), last_ul_percent(-1), last_forward(std::chrono::steady_clock::now()) {
}

filtered_progress_listener::filtered_progress_listener(i_progress_listener* target,
    int min_percent_, int min_ms_) noexcept
    : inner_shared(nullptr), inner_raw(target), min_percent(min_percent_), min_ms(min_ms_),
    last_dl_percent(-1), last_ul_percent(-1), last_forward(std::chrono::steady_clock::now()) {
}

filtered_progress_listener::filtered_progress_listener(std::shared_ptr<i_progress_listener> target,
    int min_percent_, int min_ms_) noexcept
    : inner_shared(std::move(target)), inner_raw(nullptr), min_percent(min_percent_), min_ms(min_ms_),
    last_dl_percent(-1), last_ul_percent(-1), last_forward(std::chrono::steady_clock::now()) {
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

// --- ftp_client_base implementation ---
ftp_client_base::ftp_client_base()
    : port(0), progress_listener(nullptr), control_client(std::make_unique<mino::network::tcp::tcp_client>()) {

    control_client->set_on_receive([this](const std::string& data) {
        std::lock_guard<std::mutex> lock(response_mutex);
        response_buffer += data;
        if (response_buffer.size() >= 4) {
            size_t last_line_pos = response_buffer.rfind("\r\n", response_buffer.size() - 3);
            size_t check_pos = (last_line_pos == std::string::npos) ? 0 : last_line_pos + 2;
            if (response_buffer.size() >= check_pos + 4) {
                if (std::isdigit(response_buffer[check_pos]) &&
                    std::isdigit(response_buffer[check_pos + 1]) &&
                    std::isdigit(response_buffer[check_pos + 2]) &&
                    response_buffer[check_pos + 3] == ' ') {
                    has_response = true;
                    response_cv.notify_one();
                }
            }
        }
        });

#ifdef USE_OPENSSL
    static bool ssl_initialized = false;
    if (!ssl_initialized) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ssl_initialized = true;
    }
#endif
}

ftp_client_base::~ftp_client_base() {
    if (control_client) {
        control_client->stop();
    }
#ifdef USE_OPENSSL
    if (ssl) { SSL_free(ssl); }
    if (ssl_ctx) { SSL_CTX_free(ssl_ctx); }
#endif
}

std::string ftp_client_base::get_last_error() const { return last_error; }
void ftp_client_base::set_progress_listener(i_progress_listener* listener) { progress_listener = listener; }
void ftp_client_base::remove_progress_listener() { progress_listener = nullptr; }


// --- ftp_client Real Implementation ---
ftp_client::ftp_client() = default;
ftp_client::~ftp_client() { control_client->stop(); }

std::string ftp_client::read_control_response(std::chrono::seconds timeout) {
    std::unique_lock<std::mutex> lock(response_mutex);
    bool success = response_cv.wait_for(lock, timeout, [this] { return has_response; });

    if (!success) {
        last_error = "Control channel timeout waiting for response.";
        return "";
    }

    std::string res = response_buffer;
    response_buffer.clear();
    has_response = false;
    return res;
}

bool ftp_client::send_command(const std::string& cmd, const std::string& arg) {
    std::string full_cmd = cmd + (arg.empty() ? "" : " " + arg) + "\r\n";
    {
        std::lock_guard<std::mutex> lock(response_mutex);
        has_response = false;
    }
    if (control_client->send_data(full_cmd) < 0) {
        last_error = "Failed to send command: " + cmd;
        return false;
    }
    return true;
}

std::int64_t ftp_client::get_remote_file_size(const std::string& remote_file) {
    if (!send_command("SIZE", remote_file)) return 0;
    std::string resp = read_control_response();
    if (resp.rfind("213", 0) == 0) {
        try {
            return std::stoll(resp.substr(4));
        }
        catch (...) { return 0; }
    }
    return 0;
}

std::unique_ptr<mino::network::tcp::tcp_client> ftp_client::establish_data_connection() {
    if (!send_command("PASV")) return nullptr;
    std::string resp = read_control_response();

    std::regex pasv_regex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    std::smatch match;
    if (!std::regex_search(resp, match, pasv_regex) || match.size() < 7) {
        last_error = "Failed to parse PASV response: " + resp;
        return nullptr;
    }

    std::string data_ip = match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
    unsigned short data_port = static_cast<unsigned short>(std::stoi(match[5].str()) << 8) + static_cast<unsigned short>(std::stoi(match[6].str()));

    auto data_client = std::make_unique<mino::network::tcp::tcp_client>();
    data_client->set_server(data_ip, data_port);

    if (!data_client->start(std::chrono::seconds(5))) {
        last_error = "Failed to start data connection client.";
        return nullptr;
    }

    int retry = 50;
    while (!data_client->is_connected() && retry-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (!data_client->is_connected()) {
        last_error = "Data channel connection timed out.";
        return nullptr;
    }

    return data_client;
}

bool ftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    host_name = host;
    port = p;
    user_name = user;
    password = pass;
    last_error.clear();

    control_client->set_server(host_name, static_cast<unsigned short>(port));
    if (!control_client->start(std::chrono::seconds(5))) {
        last_error = "Could not initialize control connection thread.";
        return false;
    }

    std::string resp = read_control_response();
    if (resp.rfind("220", 0) != 0) { last_error = "Invalid connection greeting: " + resp; return false; }

    if (!send_command("USER", user_name)) return false;
    resp = read_control_response();

    if (resp.rfind("331", 0) == 0) {
        if (!send_command("PASS", password)) return false;
        resp = read_control_response();
    }

    if (resp.rfind("230", 0) != 0) {
        last_error = "FTP Login rejected: " + resp;
        return false;
    }

    return true;
}

bool ftp_client::download(const std::string& remote_file, const std::string& local_file) {
    std::int64_t dltotal = get_remote_file_size(remote_file);

    if (!send_command("TYPE", "I")) return false;
    read_control_response();

    auto data_client = establish_data_connection();
    if (!data_client) return false;

    if (!send_command("RETR", remote_file)) return false;
    std::string resp = read_control_response();
    if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
        last_error = "RETR command rejected: " + resp;
        return false;
    }

    std::ofstream ofs(local_file, std::ios::binary);
    if (!ofs.is_open()) { last_error = "Failed to open local file for writing."; return false; }

    std::mutex data_mutex;
    std::condition_variable data_cv;
    bool data_finished = false;
    std::int64_t dlnow = 0;

    data_client->set_on_receive([&](const std::string& data) {
        std::lock_guard<std::mutex> lock(data_mutex);
        ofs.write(data.data(), data.size());
        dlnow += data.size();
        if (progress_listener) {
            progress_listener->on_progress(dlnow, dltotal, 0, 0);
        }
        });

    data_client->set_on_close([&]() {
        std::lock_guard<std::mutex> lock(data_mutex);
        data_finished = true;
        data_cv.notify_one();
        });

    std::unique_lock<std::mutex> data_lock(data_mutex);
    data_cv.wait(data_lock, [&] { return data_finished; });

    data_client->stop();
    read_control_response();
    return true;
}

bool ftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    std::ifstream ifs(local_file, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) { last_error = "Failed to open local file for reading."; return false; }
    std::int64_t ultotal = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    if (!send_command("TYPE", "I")) return false;
    read_control_response();

    auto data_client = establish_data_connection();
    if (!data_client) return false;

    if (!send_command("STOR", remote_file)) return false;
    std::string resp = read_control_response();
    if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
        last_error = "STOR command rejected: " + resp;
        return false;
    }

    char buffer[4096];
    std::int64_t ulnow = 0;
    while (ifs.good()) {
        ifs.read(buffer, sizeof(buffer));
        std::streamsize bytes_read = ifs.gcount();
        if (bytes_read > 0) {
            data_client->send_data(std::string(buffer, bytes_read));
            ulnow += bytes_read;
            if (progress_listener) {
                progress_listener->on_progress(0, 0, ulnow, ultotal);
            }
        }
    }

    data_client->stop();
    resp = read_control_response();
    return (resp.rfind("226", 0) == 0);
}

bool ftp_client::delete_file(const std::string& remote_file) {
    if (!send_command("DELE", remote_file)) return false;
    return (read_control_response().rfind("250", 0) == 0);
}

std::vector<file_info> ftp_client::list_directory(const std::string& path) {
    if (!send_command("TYPE", "A")) return {};
    read_control_response();

    auto data_client = establish_data_connection();
    if (!data_client) return {};

    if (!send_command("LIST", path)) return {};
    std::string resp = read_control_response();
    if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) return {};

    std::mutex data_mutex;
    std::condition_variable data_cv;
    std::string raw_list;
    bool data_finished = false;

    data_client->set_on_receive([&](const std::string& data) {
        std::lock_guard<std::mutex> lock(data_mutex);
        raw_list += data;
        });
    data_client->set_on_close([&]() {
        std::lock_guard<std::mutex> lock(data_mutex);
        data_finished = true;
        data_cv.notify_one();
        });

    std::unique_lock<std::mutex> data_lock(data_mutex);
    data_cv.wait(data_lock, [&] { return data_finished; });
    data_client->stop();
    read_control_response();

    std::vector<file_info> results;
    std::istringstream iss(raw_list);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        file_info info;
        info.is_directory = (line[0] == 'd');

        std::istringstream line_tokens(line);
        std::string token, perms, links, owner, group, size_str, month, day, time_year, name;
        if (line_tokens >> perms >> links >> owner >> group >> size_str >> month >> day >> time_year) {
            std::getline(line_tokens, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            if (!name.empty() && name.back() == '\r') name.pop_back();

            info.name = name;
            try { info.size = info.is_directory ? 0 : std::stol(size_str); }
            catch (...) { info.size = 0; }
            if (!info.name.empty() && info.name != "." && info.name != "..") {
                results.push_back(info);
            }
        }
    }
    return results;
}

bool ftp_client::create_directory(const std::string& path) {
    if (!send_command("MKD", path)) return false;
    return (read_control_response().rfind("257", 0) == 0);
}

bool ftp_client::remove_directory(const std::string& path) {
    if (!send_command("RMD", path)) return false;
    return (read_control_response().rfind("250", 0) == 0);
}


// --- sftp_client Implementation (libssh2) ---
sftp_client::sftp_client() {
    static bool libssh2_initialized = false;
    if (!libssh2_initialized) {
        libssh2_init(0);
        libssh2_initialized = true;
    }
}

sftp_client::~sftp_client() {
    cleanup();
}

void sftp_client::cleanup() {
    if (sftp_session) {
        libssh2_sftp_shutdown(sftp_session);
        sftp_session = nullptr;
    }
    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        session = nullptr;
    }
    if (socket_fd >= 0) {
#ifdef _WIN32
        closesocket(socket_fd);
#else
        close(socket_fd);
#endif
        socket_fd = -1;
    }
}

bool sftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
    cleanup();
    host_name = host; port = p; user_name = user; password = pass;

    struct addrinfo hints {}, * res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
        last_error = "DNS resolution failed for " + host;
        return false;
    }

    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd < 0) {
        last_error = "Failed to create socket.";
        freeaddrinfo(res);
        return false;
    }

    if (::connect(socket_fd, res->ai_addr, res->ai_addrlen) != 0) {
        last_error = "Failed to connect to SFTP host: " + host;
        freeaddrinfo(res);
        cleanup();
        return false;
    }
    freeaddrinfo(res);

    session = libssh2_session_init();
    if (!session) {
        last_error = "Failed to initialize libssh2 session.";
        cleanup();
        return false;
    }

    if (libssh2_session_handshake(session, socket_fd)) {
        last_error = "SSH Handshake failed.";
        cleanup();
        return false;
    }

    if (libssh2_userauth_password(session, user_name.c_str(), password.c_str())) {
        last_error = "Authentication failed (User/Password incorrect).";
        cleanup();
        return false;
    }

    sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        last_error = "Unable to initialize SFTP subsystem.";
        cleanup();
        return false;
    }

    return true;
}

bool sftp_client::download(const std::string& remote_file, const std::string& local_file) {
    if (!sftp_session) { last_error = "Not connected to SFTP."; return false; }

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(sftp_session, remote_file.c_str(), LIBSSH2_FXF_READ, 0);
    if (!handle) {
        last_error = "Failed to open remote file for download: " + remote_file;
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    std::int64_t dltotal = 0;
    if (libssh2_sftp_fstat(handle, &attrs) == 0) {
        dltotal = attrs.filesize;
    }

    std::ofstream ofs(local_file, std::ios::binary);
    if (!ofs.is_open()) {
        libssh2_sftp_close(handle);
        last_error = "Failed to open local destination file: " + local_file;
        return false;
    }

    char mem[16384];
    std::int64_t dlnow = 0;
    ssize_t rc = 0;
    while ((rc = libssh2_sftp_read(handle, mem, sizeof(mem))) > 0) {
        ofs.write(mem, rc);
        dlnow += rc;
        if (progress_listener) {
            progress_listener->on_progress(dlnow, dltotal, 0, 0);
        }
    }

    libssh2_sftp_close(handle);
    return (rc >= 0);
}

bool sftp_client::upload(const std::string& local_file, const std::string& remote_file) {
    if (!sftp_session) { last_error = "Not connected to SFTP."; return false; }

    std::ifstream ifs(local_file, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) { last_error = "Failed to open local source file: " + local_file; return false; }
    std::int64_t ultotal = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(
        sftp_session, remote_file.c_str(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH
    );

    if (!handle) {
        last_error = "Failed to open/create remote file for upload: " + remote_file;
        return false;
    }

    char mem[16384];
    std::int64_t ulnow = 0;
    while (ifs.good()) {
        ifs.read(mem, sizeof(mem));
        std::streamsize bytes_read = ifs.gcount();
        if (bytes_read <= 0) break;

        char* ptr = mem;
        std::streamsize remaining = bytes_read;
        while (remaining > 0) {
            ssize_t rc = libssh2_sftp_write(handle, ptr, remaining);
            if (rc < 0) {
                last_error = "Failed while writing data to SFTP.";
                libssh2_sftp_close(handle);
                return false;
            }
            ptr += rc;
            remaining -= rc;
        }

        ulnow += bytes_read;
        if (progress_listener) {
            progress_listener->on_progress(0, 0, ulnow, ultotal);
        }
    }

    libssh2_sftp_close(handle);
    return true;
}

bool sftp_client::delete_file(const std::string& remote_file) {
    if (!sftp_session) { last_error = "Not connected to SFTP."; return false; }
    if (libssh2_sftp_unlink(sftp_session, remote_file.c_str()) != 0) {
        last_error = "Failed to delete remote file: " + remote_file;
        return false;
    }
    return true;
}

std::vector<file_info> sftp_client::list_directory(const std::string& path) {
    std::vector<file_info> results;
    if (!sftp_session) { last_error = "Not connected to SFTP."; return results; }

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp_session, path.c_str());
    if (!handle) {
        last_error = "Failed to open remote directory: " + path;
        return results;
    }

    char mem[512];
    char longentry[1024];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (libssh2_sftp_readdir_ex(handle, mem, sizeof(mem), longentry, sizeof(longentry), &attrs) > 0) {
        std::string filename(mem);
        if (filename == "." || filename == "..") continue;

        file_info fi;
        fi.name = filename;
        fi.is_directory = (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
        fi.size = fi.is_directory ? 0 : static_cast<long>(attrs.filesize);

        results.push_back(fi);
    }

    libssh2_sftp_closedir(handle);
    return results;
}

bool sftp_client::create_directory(const std::string& path) {
    if (!sftp_session) { last_error = "Not connected to SFTP."; return false; }
    int rc = libssh2_sftp_mkdir(sftp_session, path.c_str(),
        LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
    if (rc != 0) {
        last_error = "Failed to create remote directory: " + path;
        return false;
    }
    return true;
}

bool sftp_client::remove_directory(const std::string& path) {
    if (!sftp_session) { last_error = "Not connected to SFTP."; return false; }
    if (libssh2_sftp_rmdir(sftp_session, path.c_str()) != 0) {
        last_error = "Failed to remove remote directory: " + path;
        return false;
    }
    return true;
}
