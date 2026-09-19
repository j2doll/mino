#include <algorithm>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <cctype>
#include <string>
#include <iostream>
#include <vector>

#include "mino/core/crypt/keyless_cipher.hpp"
#include "mino/network/memory_store/server.hpp"
#include "mino/network/memory_store/protocol.hpp"

namespace mino::network::memory_store {

    memory_store_server::memory_store_server() :
        bind_port_(0),
        db_filepath_("memory_store.db"),
        is_auto_save_running_(false),
        auto_save_interval_(0),
        sleep_for_transmission_(std::chrono::milliseconds(50)),
        logger_(nullptr)
    {
    }

    memory_store_server::~memory_store_server() {
        stop();
    }

    void memory_store_server::set_network(const std::string& ip, int port) {
        bind_ip_ = ip;
        bind_port_ = port;
    }

    void memory_store_server::set_storage_file(const std::string& filepath) {
        db_filepath_ = filepath;
    }

    void memory_store_server::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger_ = logger_ptr;
        server_.set_logger(logger_);
    }

    void memory_store_server::set_auto_save(std::chrono::seconds interval) {
        auto_save_interval_ = interval;
    }

    void memory_store_server::set_sleep_for_transmission(std::chrono::milliseconds sleep_duration) {
        sleep_for_transmission_ = sleep_duration;
    }

    bool memory_store_server::start() {
        server_.set_on_receive_callback([this](socket_t fd, const std::string& data) {
            this->handle_on_receive(fd, data);
            });

        // 클라이언트 연결 종료 시 수신 버퍼 정리
        server_.set_on_close_callback([this](socket_t fd, const std::string& /*reason*/) {
            std::lock_guard<std::mutex> lock(client_buffers_mutex_);
            client_buffers_.erase(fd);
            });

        if (logger_) logger_->info("[memory_store_server] Starting server at {}:{}", bind_ip_, bind_port_);

        auto ret_tcp = server_.start(bind_ip_, bind_port_);
        if (ret_tcp != mino::network::tcp::tcp_server::start_result::success) {
            if (logger_) logger_->error("[memory_store_server] Failed to start TCP server. Error code: {}", static_cast<int>(ret_tcp));
            return false;
        }

        if (auto_save_interval_.count() > 0) {
            is_auto_save_running_ = true;
            auto_save_thread_ = std::thread(&memory_store_server::auto_save_loop, this);
            if (logger_) logger_->info("[memory_store_server] Periodic auto-save enabled. Interval: {} seconds", auto_save_interval_.count());
        }
        else {
            if (logger_) logger_->info("[memory_store_server] Periodic auto-save is disabled.");
        }

        return true;
    }

    void memory_store_server::stop() {
        if (logger_) logger_->info("[memory_store_server] Stopping server...");

        if (is_auto_save_running_) {
            is_auto_save_running_ = false;
            auto_save_cv_.notify_all();
            if (auto_save_thread_.joinable()) {
                auto_save_thread_.join();
            }
        }

        server_.quit();

        std::lock_guard<std::mutex> lock(client_buffers_mutex_);
        client_buffers_.clear();
    }

    bool memory_store_server::save_to_file(const std::string& filename) {
        namespace fs = std::filesystem;
        fs::path out_path = fs::path(filename).is_absolute() ? fs::path(filename) : (fs::current_path() / filename);

        // 1. 메모리 스냅샷 복사 (storage_mutex_ 최소 점유)
        std::string plain;
        {
            std::shared_lock<std::shared_mutex> lock(storage_mutex_);
            for (const auto& [key, val] : storage_) {
                plain.append(std::to_string(key.size()));
                plain.push_back(' ');
                plain.append(std::to_string(val.size()));
                plain.push_back(' ');
                if (!key.empty()) plain.append(key.data(), key.size());
                if (!val.empty()) plain.append(val.data(), val.size());
                plain.push_back('\n');
            }
        }

        // 2. 암호화 (락 해제 상태에서 수행)
        std::string cipher_text;
        try {
            cipher_text = mino::core::crypt::keyless_cipher::encrypt(plain);
        }
        catch (const std::exception& ex) {
            if (logger_) logger_->error("[memory_store_server] Encryption failed for {}: {}", out_path.string(), ex.what());
            return false;
        }

        // 3. 파일 쓰기 동기화 (file_mutex_ 사용)
        std::lock_guard<std::mutex> f_lock(file_mutex_);
        std::error_code ec;
        if (out_path.has_parent_path()) {
            fs::create_directories(out_path.parent_path(), ec);
            if (ec) {
                if (logger_) logger_->error("[memory_store_server] Failed to create directories for {}: {}", out_path.string(), ec.message());
                return false;
            }
        }

        std::ofstream file(out_path.string(), std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            if (logger_) logger_->error("[memory_store_server] Failed to open file for write: {}", out_path.string());
            return false;
        }

        if (!cipher_text.empty()) {
            file.write(cipher_text.data(), static_cast<std::streamsize>(cipher_text.size()));
        }
        if (!file) {
            if (logger_) logger_->error("[memory_store_server] Failed while writing to {}", out_path.string());
            return false;
        }

        if (logger_) logger_->info("[memory_store_server] Successfully saved encrypted data to {}", out_path.string());
        return true;
    }

    bool memory_store_server::load_from_file(const std::string& filename) {
        if (filename.empty()) {
            if (logger_) logger_->error("[memory_store_server] Load failed: filename is empty");
            return false;
        }

        namespace fs = std::filesystem;
        fs::path in_path = fs::path(filename).is_absolute() ? fs::path(filename) : (fs::current_path() / filename);

        std::string cipher_text;
        {
            // 1. 파일 읽기 (storage_mutex_를 잡지 않고 file_mutex_만 점유)
            std::lock_guard<std::mutex> f_lock(file_mutex_);
            std::ifstream file(in_path.string(), std::ios::binary);
            if (!file.is_open()) {
                if (logger_) logger_->error("[memory_store_server] Failed to open file for read: {}", in_path.string());
                return false;
            }

            file.seekg(0, std::ios::end);
            std::streamsize fsize = file.tellg();
            file.seekg(0, std::ios::beg);
            if (fsize > 0) {
                cipher_text.resize(static_cast<size_t>(fsize));
                file.read(&cipher_text[0], fsize);
                if (!file) {
                    if (logger_) logger_->error("[memory_store_server] Failed reading file contents from {}", in_path.string());
                    return false;
                }
            }
        }

        // 2. 복호화
        std::string plain;
        try {
            plain = mino::core::crypt::keyless_cipher::decrypt(cipher_text);
        }
        catch (const std::exception& ex) {
            if (logger_) logger_->error("[memory_store_server] Decryption failed for {}: {}", in_path.string(), ex.what());
            return false;
        }

        // 3. 임시 맵에 파싱
        std::unordered_map<std::string, std::string> new_storage;
        const char* buf = plain.data();
        size_t pos = 0;
        size_t len = plain.size();

        auto skip_whitespace = [&]() {
            while (pos < len && std::isspace(static_cast<unsigned char>(buf[pos]))) ++pos;
            };

        while (pos < len) {
            skip_whitespace();
            if (pos >= len) break;

            size_t start = pos;
            while (pos < len && std::isdigit(static_cast<unsigned char>(buf[pos]))) ++pos;
            if (start == pos) break;

            size_t klen = 0;
            try {
                klen = static_cast<size_t>(std::stoull(std::string(&buf[start], pos - start)));
            }
            catch (...) {
                if (logger_) logger_->error("[memory_store_server] Failed parsing key length from decrypted data");
                return false;
            }

            if (pos >= len || buf[pos] != ' ') return false;
            ++pos;

            start = pos;
            while (pos < len && std::isdigit(static_cast<unsigned char>(buf[pos]))) ++pos;
            if (start == pos) break;

            size_t vlen = 0;
            try {
                vlen = static_cast<size_t>(std::stoull(std::string(&buf[start], pos - start)));
            }
            catch (...) {
                if (logger_) logger_->error("[memory_store_server] Failed parsing value length from decrypted data");
                return false;
            }

            if (pos >= len || buf[pos] != ' ') return false;
            ++pos;

            if (pos + klen > len) return false;
            std::string key;
            if (klen > 0) {
                key.assign(&buf[pos], klen);
                pos += klen;
            }

            if (pos + vlen > len) return false;
            std::string val;
            if (vlen > 0) {
                val.assign(&buf[pos], vlen);
                pos += vlen;
            }

            if (pos < len && buf[pos] == '\n') ++pos;

            new_storage[std::move(key)] = std::move(val);
        }

        // 4. 파싱 완료 후 한 번에 스왑 (락 점유 극소화)
        {
            std::unique_lock<std::shared_mutex> lock(storage_mutex_);
            storage_ = std::move(new_storage);
        }

        if (logger_) logger_->info("[memory_store_server] Load complete. Restored {} entries.", storage_.size());
        return true;
    }

    void memory_store_server::auto_save_loop() {
        std::unique_lock<std::mutex> lock(auto_save_mutex_);
        while (is_auto_save_running_) {
            auto_save_cv_.wait_for(lock, auto_save_interval_, [this] { return !is_auto_save_running_; });
            if (!is_auto_save_running_) break;

            if (logger_) logger_->info("[memory_store_server] Triggering periodic auto-save...");
            save_to_file(db_filepath_);
        }
    }

    void memory_store_server::handle_on_receive(socket_t client_socket, const std::string& data) {
        std::vector<std::string> complete_commands;

        // 스트림 버퍼링 처리: 개행('\n') 단위로 명령어 분리
        {
            std::lock_guard<std::mutex> lock(client_buffers_mutex_);
            std::string& buf = client_buffers_[client_socket];
            buf.append(data);

            size_t pos = 0;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                complete_commands.push_back(std::move(line));
                buf.erase(0, pos + 1);
            }
        }

        for (const auto& cmd_line : complete_commands) {
            if (!cmd_line.empty()) {
                process_command(client_socket, cmd_line);
            }
        }
    }

    void memory_store_server::process_command(socket_t client_socket, const std::string& command_line) {
        auto tokens = parse_command(command_line);
        if (tokens.empty()) {
            server_.send_to_client(client_socket, "-ERR empty command\n");
            return;
        }

        std::string cmd = tokens[0];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "SET") {
            if (tokens.size() < 3) {
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "-ERR wrong number of arguments for 'set'\n");
                return;
            }

            if (!is_valid_key(tokens[1])) {
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "-ERR invalid key format\n");
                return;
            }

            {
                std::unique_lock<std::shared_mutex> lock(storage_mutex_);
                storage_[tokens[1]] = tokens[2];
            }

            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            server_.send_to_client(client_socket, "+OK\n");
            if (logger_) logger_->info("[memory_store_server] SET key '{}' for client {}", tokens[1], client_socket);
        }
        else if (cmd == "GET") {
            if (tokens.size() < 2) {
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "-ERR wrong number of arguments for 'get'\n");
                return;
            }

            std::string val;
            bool found = false;
            {
                std::shared_lock<std::shared_mutex> lock(storage_mutex_);
                auto it = storage_.find(tokens[1]);
                if (it != storage_.end()) {
                    val = it->second;
                    found = true;
                }
            }

            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            if (found) {
                server_.send_to_client(client_socket, "+" + val + "\n");
            }
            else {
                server_.send_to_client(client_socket, "$-1\n");
            }
        }
        else if (cmd == "DEL") {
            if (tokens.size() < 2) {
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "-ERR wrong number of arguments for 'del'\n");
                return;
            }

            size_t erased_count = 0;
            {
                std::unique_lock<std::shared_mutex> lock(storage_mutex_);
                erased_count = storage_.erase(tokens[1]);
            }

            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            server_.send_to_client(client_socket, ":" + std::to_string(erased_count) + "\n");
        }
        else if (cmd == "SAVE") {
            std::string target_file = (tokens.size() >= 2) ? tokens[1] : db_filepath_;
            bool success = save_to_file(target_file);

            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            server_.send_to_client(client_socket, success ? "+OK\n" : "-ERR failed to save file\n");
        }
        else if (cmd == "LOAD") {
            std::string target_file = (tokens.size() >= 2) ? tokens[1] : db_filepath_;
            bool success = load_from_file(target_file);

            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            server_.send_to_client(client_socket, success ? "+OK\n" : "-ERR failed to load file\n");
        }
        else if (cmd == "DUMP") {
            // 스냅샷을 생성하여 락을 빠르게 해제 (다른 클라이언트의 Write 블로킹 방지)
            std::vector<std::pair<std::string, std::string>> snapshot;
            {
                std::shared_lock<std::shared_mutex> lock(storage_mutex_);
                snapshot.reserve(storage_.size());
                for (const auto& kv : storage_) {
                    snapshot.push_back(kv);
                }
            }

            if (snapshot.empty()) {
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "+EMPTY\n");
            }
            else {
                for (const auto& [key, val] : snapshot) {
                    if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                    server_.send_to_client(client_socket, "+" + key + " " + val + "\n");
                }
                if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
                server_.send_to_client(client_socket, "+END\n");
            }
        }
        else if (cmd == "LATENCY") {
            size_t key_count = 0;
            {
                std::shared_lock<std::shared_mutex> lock(storage_mutex_);
                key_count = storage_.size();
            }

            long long total_ms = static_cast<long long>(key_count) * static_cast<long long>(sleep_for_transmission_.count());
            server_.send_to_client(client_socket, ":" + std::to_string(total_ms) + "\n");
        }
        else if (cmd == "SLEEP_MS") {
            long long sleep_ms = static_cast<long long>(sleep_for_transmission_.count());
            server_.send_to_client(client_socket, ":" + std::to_string(sleep_ms) + "\n");
        }
        else {
            if (sleep_for_transmission_.count() > 0) std::this_thread::sleep_for(sleep_for_transmission_);
            server_.send_to_client(client_socket, "-ERR unknown command\n");
        }
    }

    bool memory_store_server::load() {
        return load_from_file(db_filepath_);
    }

    void memory_store_server::print_all() const {
        std::shared_lock<std::shared_mutex> lock(storage_mutex_);
        if (logger_) {
            if (storage_.empty()) {
                logger_->info("[memory_store_server] Storage is empty.");
            }
            else {
                logger_->info("[memory_store_server] Listing all key/value pairs:");
                for (const auto& [key, val] : storage_) {
                    logger_->info("[memory_store_server] {} => {}", key, val);
                }
            }
        }
        else {
            if (storage_.empty()) {
                std::cout << "[memory_store_server] Storage is empty.\n";
            }
            else {
                std::cout << "[memory_store_server] Listing all key/value pairs:\n";
                for (const auto& [key, val] : storage_) {
                    std::cout << key << " => " << val << '\n';
                }
            }
        }
    }

}
