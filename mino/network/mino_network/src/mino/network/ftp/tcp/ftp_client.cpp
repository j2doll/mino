#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <thread>
#include <chrono>

#include "mino/network/ssh/ssh_client.hpp"
#include "mino/network/ssh/ssh_buffer.hpp"

#include "mino/network/ftp/tcp/ftp_client.hpp"

namespace mino::network::ftp::tcp { 

    // --- default_progress_listener implementation ---
    void default_progress_listener::on_progress(std::int64_t dlnow, std::int64_t dltotal,
        std::int64_t ulnow, std::int64_t ultotal) {
        if (dlnow < 0 || dltotal < 0 || ulnow < 0 || ultotal < 0) return;

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
    }

    ftp_client_base::~ftp_client_base() {
        if (control_client) {
            control_client->stop();
        }
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

        std::ofstream ofs(local_file, std::ios::binary);
        if (!ofs.is_open()) {
            last_error = "Failed to open local file for writing: " + local_file;
            data_client->stop();
            return false;
        }

        std::mutex data_mutex;
        std::condition_variable data_cv;
        bool data_finished = false;
        std::int64_t dlnow = 0;

        // 1. [레이스 컨디션 방지] RETR 명령 전송 전에 수신/종료 콜백 등록
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

        // 2. RETR 명령 전송
        if (!send_command("RETR", remote_file)) {
            data_client->stop();
            return false;
        }

        std::string resp = read_control_response();
        if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
            last_error = "RETR command rejected: " + resp;
            data_client->stop();
            return false;
        }

        // 3. 연결 상태 사전 검사 및 wait_for(타임아웃) 수행
        {
            std::unique_lock<std::mutex> data_lock(data_mutex);
            if (!data_client->is_connected()) {
                data_finished = true;
            }

            bool completed = data_cv.wait_for(data_lock, std::chrono::seconds(60), [&] {
                return data_finished;
                });

            if (!completed) {
                last_error = "Data channel download timed out.";
                data_client->stop();
                return false;
            }
        }

        data_client->stop();
        ofs.close();

        resp = read_control_response();
        return (resp.rfind("226", 0) == 0 || resp.rfind("250", 0) == 0);
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

        std::mutex data_mutex;
        std::condition_variable data_cv;
        std::string raw_list;
        bool data_finished = false;

        // 1. [레이스 컨디션 방지] LIST 명령 전에 콜백 등록
        data_client->set_on_receive([&](const std::string& data) {
            std::lock_guard<std::mutex> lock(data_mutex);
            raw_list += data;
            });

        data_client->set_on_close([&]() {
            std::lock_guard<std::mutex> lock(data_mutex);
            data_finished = true;
            data_cv.notify_one();
            });

        // 2. LIST 명령 전송
        if (!send_command("LIST", path)) {
            data_client->stop();
            return {};
        }

        std::string resp = read_control_response();
        if (resp.rfind("150", 0) != 0 && resp.rfind("125", 0) != 0) {
            data_client->stop();
            return {};
        }

        // 3. 타임아웃 및 조기 종료 검사
        {
            std::unique_lock<std::mutex> data_lock(data_mutex);
            if (!data_client->is_connected()) {
                data_finished = true;
            }

            bool completed = data_cv.wait_for(data_lock, std::chrono::seconds(10), [&] {
                return data_finished;
                });

            if (!completed) {
                last_error = "Timeout waiting for directory listing completion.";
                data_client->stop();
                return {};
            }
        }

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
                try {
                    info.size = info.is_directory ? 0 : std::stoll(size_str);
                }
                catch (...) {
                    info.size = 0;
                }

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


    // --- SFTP v3 Protocol Constants ---
    namespace {
        constexpr uint8_t SSH_FXP_INIT = 1;
        constexpr uint8_t SSH_FXP_VERSION = 2;
        constexpr uint8_t SSH_FXP_OPEN = 3;
        constexpr uint8_t SSH_FXP_CLOSE = 4;
        constexpr uint8_t SSH_FXP_READ = 5;
        constexpr uint8_t SSH_FXP_WRITE = 6;
        constexpr uint8_t SSH_FXP_FSTAT = 8;
        constexpr uint8_t SSH_FXP_OPENDIR = 11;
        constexpr uint8_t SSH_FXP_READDIR = 12;
        constexpr uint8_t SSH_FXP_REMOVE = 13;
        constexpr uint8_t SSH_FXP_MKDIR = 14;
        constexpr uint8_t SSH_FXP_RMDIR = 15;

        constexpr uint8_t SSH_FXP_STATUS = 101;
        constexpr uint8_t SSH_FXP_HANDLE = 102;
        constexpr uint8_t SSH_FXP_DATA = 103;
        constexpr uint8_t SSH_FXP_NAME = 104;
        constexpr uint8_t SSH_FXP_ATTRS = 105;

        constexpr uint32_t SSH_FX_OK = 0;
        constexpr uint32_t SSH_FX_EOF = 1;

        constexpr uint32_t SSH_FXF_READ = 0x00000001;
        constexpr uint32_t SSH_FXF_WRITE = 0x00000002;
        constexpr uint32_t SSH_FXF_CREAT = 0x00000008;
        constexpr uint32_t SSH_FXF_TRUNC = 0x00000010;

        constexpr uint32_t SSH_FILEXFER_ATTR_SIZE = 0x00000001;
        constexpr uint32_t SSH_FILEXFER_ATTR_PERMISSIONS = 0x00000004;
    }

    // --- sftp_client Implementation (Pure C++ / No libssh2) ---
    sftp_client::sftp_client() = default;
    sftp_client::~sftp_client() { cleanup(); }

    void sftp_client::cleanup() {
        if (ssh_) {
            ssh_->stop();
            ssh_.reset();
        }
    }

    void sftp_client::send_sftp_packet(const mino::network::ssh::ssh_buffer& payload) {
        if (!ssh_) return;
        mino::network::ssh::ssh_buffer pkt;
        pkt.write_uint32(static_cast<uint32_t>(payload.size()));
        pkt.write_raw(payload.data().data(), payload.size());
        ssh_->send_channel_data(pkt.data().data(), pkt.size());
    }

    mino::network::ssh::ssh_buffer sftp_client::recv_sftp_packet(std::chrono::milliseconds timeout) {
        if (!ssh_) return {};
        uint8_t len_buf[4];
        if (!ssh_->recv_channel_exact(len_buf, 4, timeout)) {
            return {};
        }
        uint32_t len = (static_cast<uint32_t>(len_buf[0]) << 24) |
            (static_cast<uint32_t>(len_buf[1]) << 16) |
            (static_cast<uint32_t>(len_buf[2]) << 8) |
            static_cast<uint32_t>(len_buf[3]);

        std::vector<uint8_t> body(len);
        if (!ssh_->recv_channel_exact(body.data(), len, timeout)) {
            return {};
        }
        return mino::network::ssh::ssh_buffer(std::move(body));
    }

    std::string sftp_client::sftp_open(const std::string& path, uint32_t flags, uint32_t mode) {
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_OPEN);
        req.write_uint32(id);
        req.write_string(path);
        req.write_uint32(flags);
        if (flags & SSH_FXF_CREAT) {
            req.write_uint32(SSH_FILEXFER_ATTR_PERMISSIONS);
            req.write_uint32(mode);
        }
        else {
            req.write_uint32(0);
        }
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0) return "";

        uint8_t type = resp.read_byte();
        resp.read_uint32(); // ID
        if (type == SSH_FXP_HANDLE) {
            return resp.read_string();
        }
        if (type == SSH_FXP_STATUS) {
            uint32_t code = resp.read_uint32();
            last_error = "SFTP OPEN rejected, status: " + std::to_string(code);
        }
        return "";
    }

    bool sftp_client::sftp_close(const std::string& handle) {
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_CLOSE);
        req.write_uint32(id);
        req.write_string(handle);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_STATUS) return false;
        resp.read_uint32();
        return (resp.read_uint32() == SSH_FX_OK);
    }

    std::int64_t sftp_client::sftp_fstat_size(const std::string& handle) {
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_FSTAT);
        req.write_uint32(id);
        req.write_string(handle);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_ATTRS) return 0;
        resp.read_uint32();
        uint32_t flags = resp.read_uint32();
        if (flags & SSH_FILEXFER_ATTR_SIZE) {
            return static_cast<std::int64_t>(resp.read_uint64());
        }
        return 0;
    }

    bool sftp_client::connect(const std::string& host, int p, const std::string& user, const std::string& pass) {
        cleanup();
        host_name = host;
        port = p;
        user_name = user;
        password = pass;

        ssh_ = std::make_unique<mino::network::ssh::ssh_client>();
        if (!ssh_->connect_sync(host, static_cast<uint16_t>(port), user, pass)) {
            last_error = "Failed to establish SSH connection/authentication.";
            cleanup();
            return false;
        }

        if (!ssh_->request_subsystem("sftp")) {
            last_error = "Server rejected SFTP subsystem request.";
            cleanup();
            return false;
        }

        // Exchange SFTP Version (Init -> Version)
        mino::network::ssh::ssh_buffer init_pkt;
        init_pkt.write_byte(SSH_FXP_INIT);
        init_pkt.write_uint32(3); // Version 3
        send_sftp_packet(init_pkt);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_VERSION) {
            last_error = "SFTP subsystem handshake failed.";
            cleanup();
            return false;
        }

        return true;
    }

    bool sftp_client::download(const std::string& remote_file, const std::string& local_file) {
        if (!ssh_ || !ssh_->is_authenticated()) {
            last_error = "Not connected to SFTP.";
            return false;
        }

        std::string handle = sftp_open(remote_file, SSH_FXF_READ, 0);
        if (handle.empty()) {
            last_error = "Failed to open remote file: " + remote_file;
            return false;
        }

        std::int64_t dltotal = sftp_fstat_size(handle);
        std::ofstream ofs(local_file, std::ios::binary);
        if (!ofs.is_open()) {
            sftp_close(handle);
            last_error = "Failed to open local destination file: " + local_file;
            return false;
        }

        uint64_t offset = 0;
        const uint32_t chunk_size = 16384; // 16KB 표준 청크
        std::int64_t dlnow = 0;
        bool ok = true;

        while (true) {
            uint32_t id = next_id();
            mino::network::ssh::ssh_buffer req;
            req.write_byte(SSH_FXP_READ);
            req.write_uint32(id);
            req.write_string(handle);
            req.write_uint64(offset);
            req.write_uint32(chunk_size);
            send_sftp_packet(req);

            auto resp = recv_sftp_packet(std::chrono::milliseconds(15000));
            if (resp.size() == 0) { ok = false; break; }

            uint8_t type = resp.read_byte();
            resp.read_uint32(); // ID

            if (type == SSH_FXP_DATA) {
                std::string data = resp.read_string();
                if (data.empty()) break;
                ofs.write(data.data(), data.size());
                offset += data.size();
                dlnow += data.size();
                if (progress_listener) {
                    progress_listener->on_progress(dlnow, dltotal, 0, 0);
                }
            }
            else if (type == SSH_FXP_STATUS) {
                uint32_t code = resp.read_uint32();
                if (code == SSH_FX_EOF) {
                    break; // Complete
                }
                last_error = "SFTP READ error code: " + std::to_string(code);
                ok = false;
                break;
            }
            else {
                ok = false;
                break;
            }
        }

        sftp_close(handle);
        return ok;
    }

    bool sftp_client::upload(const std::string& local_file, const std::string& remote_file) {
        if (!ssh_ || !ssh_->is_authenticated()) {
            last_error = "Not connected to SFTP.";
            return false;
        }

        std::ifstream ifs(local_file, std::ios::binary | std::ios::ate);
        if (!ifs.is_open()) {
            last_error = "Failed to open local source file: " + local_file;
            return false;
        }
        std::int64_t ultotal = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::string handle = sftp_open(
            remote_file,
            SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC,
            0644
        );
        if (handle.empty()) {
            last_error = "Failed to open remote file for upload: " + remote_file;
            return false;
        }

        uint64_t offset = 0;
        std::int64_t ulnow = 0;

        // SSH 채널 최대 패킷 크기(32KB) 한도 초과 방지를 위한 16KB 표준 청크
        const size_t chunk_size = 16384;
        std::vector<char> buf(chunk_size);
        bool ok = true;

        while (ifs.good() && ulnow < ultotal) {
            ifs.read(buf.data(), buf.size());
            std::streamsize bytes = ifs.gcount();
            if (bytes <= 0) break;

            uint32_t id = next_id();
            mino::network::ssh::ssh_buffer req;
            req.write_byte(SSH_FXP_WRITE);
            req.write_uint32(id);
            req.write_string(handle);
            req.write_uint64(offset);
            req.write_bytes(reinterpret_cast<const uint8_t*>(buf.data()), static_cast<size_t>(bytes));
            send_sftp_packet(req);

            // 여유 있는 15초 타임아웃
            auto resp = recv_sftp_packet(std::chrono::milliseconds(15000));
            if (resp.size() == 0 || resp.read_byte() != SSH_FXP_STATUS) {
                last_error = "SFTP WRITE response timeout or invalid packet.";
                ok = false;
                break;
            }
            resp.read_uint32(); // ID
            uint32_t code = resp.read_uint32();
            if (code != SSH_FX_OK) {
                last_error = "SFTP WRITE failure code: " + std::to_string(code);
                ok = false;
                break;
            }

            offset += bytes;
            ulnow += bytes;
            if (progress_listener) {
                progress_listener->on_progress(0, 0, ulnow, ultotal);
            }
        }

        sftp_close(handle);
        return ok;
    }

    bool sftp_client::delete_file(const std::string& remote_file) {
        if (!ssh_ || !ssh_->is_authenticated()) return false;
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_REMOVE);
        req.write_uint32(id);
        req.write_string(remote_file);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_STATUS) return false;
        resp.read_uint32();
        return (resp.read_uint32() == SSH_FX_OK);
    }

    std::vector<file_info> sftp_client::list_directory(const std::string& path) {
        std::vector<file_info> results;
        if (!ssh_ || !ssh_->is_authenticated()) return results;

        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_OPENDIR);
        req.write_uint32(id);
        req.write_string(path);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_HANDLE) {
            last_error = "Failed to open directory: " + path;
            return results;
        }
        resp.read_uint32();
        std::string handle = resp.read_string();

        while (true) {
            id = next_id();
            mino::network::ssh::ssh_buffer rdir;
            rdir.write_byte(SSH_FXP_READDIR);
            rdir.write_uint32(id);
            rdir.write_string(handle);
            send_sftp_packet(rdir);

            auto dresp = recv_sftp_packet();
            if (dresp.size() == 0) break;

            uint8_t type = dresp.read_byte();
            dresp.read_uint32(); // ID

            if (type == SSH_FXP_NAME) {
                uint32_t count = dresp.read_uint32();
                for (uint32_t i = 0; i < count; ++i) {
                    std::string name = dresp.read_string();
                    std::string longname = dresp.read_string();

                    uint32_t flags = dresp.read_uint32();
                    std::int64_t fsize = 0;
                    uint32_t perms = 0;
                    if (flags & SSH_FILEXFER_ATTR_SIZE) {
                        fsize = static_cast<std::int64_t>(dresp.read_uint64());
                    }
                    if (flags & 0x00000002) {
                        dresp.read_uint32(); dresp.read_uint32();
                    }
                    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
                        perms = dresp.read_uint32();
                    }
                    if (flags & 0x00000008) {
                        dresp.read_uint32(); dresp.read_uint32();
                    }
                    if (flags & 0x80000000) {
                        uint32_t ext_cnt = dresp.read_uint32();
                        for (uint32_t e = 0; e < ext_cnt; ++e) {
                            dresp.read_string(); dresp.read_string();
                        }
                    }

                    if (name != "." && name != "..") {
                        file_info fi;
                        fi.name = name;
                        fi.is_directory = (flags & SSH_FILEXFER_ATTR_PERMISSIONS) && ((perms & 0040000) == 0040000);
                        fi.size = fi.is_directory ? 0 : fsize;
                        results.push_back(fi);
                    }
                }
            }
            else if (type == SSH_FXP_STATUS) {
                break; // EOF reached
            }
            else {
                break;
            }
        }

        sftp_close(handle);
        return results;
    }

    bool sftp_client::create_directory(const std::string& path) {
        if (!ssh_ || !ssh_->is_authenticated()) return false;
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_MKDIR);
        req.write_uint32(id);
        req.write_string(path);
        req.write_uint32(SSH_FILEXFER_ATTR_PERMISSIONS);
        req.write_uint32(0755);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_STATUS) return false;
        resp.read_uint32();
        return (resp.read_uint32() == SSH_FX_OK);
    }

    bool sftp_client::remove_directory(const std::string& path) {
        if (!ssh_ || !ssh_->is_authenticated()) return false;
        uint32_t id = next_id();
        mino::network::ssh::ssh_buffer req;
        req.write_byte(SSH_FXP_RMDIR);
        req.write_uint32(id);
        req.write_string(path);
        send_sftp_packet(req);

        auto resp = recv_sftp_packet();
        if (resp.size() == 0 || resp.read_byte() != SSH_FXP_STATUS) return false;
        resp.read_uint32();
        return (resp.read_uint32() == SSH_FX_OK);
    }


} // namespace mino::network::ftp
