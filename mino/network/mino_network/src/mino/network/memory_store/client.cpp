#include <chrono>
#include <stdexcept>
#include <algorithm>

#include "mino/network/memory_store/client.hpp"
#include "mino/network/memory_store/protocol.hpp"

namespace mino::network::memory_store {

    memory_store_client::memory_store_client() :
        timeout_(std::chrono::seconds(5)),
        logger_(nullptr),
        remote_ip_{},
        remote_port_(0)
    {
        // 스트림 수신 시 개행('\n') 단위로 분리하여 큐에 적재
        client_.set_on_receive([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(response_mutex_);
            rx_buffer_.append(data);

            size_t pos = 0;
            while ((pos = rx_buffer_.find('\n')) != std::string::npos) {
                std::string line = rx_buffer_.substr(0, pos);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                response_queue_.push(std::move(line));
                rx_buffer_.erase(0, pos + 1);
            }
            response_cv_.notify_all();
            });

        // 소켓 끊김 시 대기 중인 스레드 즉각 해제
        client_.set_on_close([this]() {
            std::lock_guard<std::mutex> lock(response_mutex_);
            response_cv_.notify_all();
            });
    }

    memory_store_client::~memory_store_client() {
        stop();
    }

    void memory_store_client::set_server_env(const std::string& ip, unsigned short port) {
        remote_ip_ = ip;
        remote_port_ = port;
    }

    void memory_store_client::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger_ = logger_ptr;
        client_.set_logger(logger_);
    }

    void memory_store_client::set_timeout(std::chrono::seconds timeout) {
        timeout_ = timeout;
    }

    bool memory_store_client::connect(std::chrono::seconds tcp_timeout) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        client_.set_server(remote_ip_, remote_port_, AF_INET);
        if (logger_) logger_->info("[memory_store_client] Connecting to server {}:{}", remote_ip_, remote_port_);

        if (!client_.start(tcp_timeout))
            return false;

        int retry = 30;
        while (!client_.is_connected() && retry-- > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return client_.is_connected();
    }

    void memory_store_client::stop() {
        if (logger_) logger_->info("[memory_store_client] Disconnecting client...");
        client_.stop();

        std::lock_guard<std::mutex> lock(response_mutex_);
        while (!response_queue_.empty()) {
            response_queue_.pop();
        }
        rx_buffer_.clear();
        response_cv_.notify_all();
    }

    std::optional<std::string> memory_store_client::send_and_wait(const std::string& command, std::chrono::seconds timeout) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        return send_and_wait_locked(command, timeout);
    }

    std::optional<std::string> memory_store_client::send_and_wait_locked(const std::string& command, std::chrono::seconds timeout) {
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] Send failed: Disconnected");
            return std::nullopt;
        }

        std::unique_lock<std::mutex> lock(response_mutex_);
        // 이전 응답 찌꺼기 제거
        while (!response_queue_.empty()) {
            response_queue_.pop();
        }

        if (client_.send_data(command + "\n") < 0) {
            if (logger_) logger_->error("[memory_store_client] Failed to write socket buffer");
            return std::nullopt;
        }

        bool success = response_cv_.wait_for(lock, timeout, [this] {
            return !response_queue_.empty() || !client_.is_connected();
            });

        if (!success || response_queue_.empty() || !client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] Request timeout or network failure during wait.");
            return std::nullopt;
        }

        std::string resp = std::move(response_queue_.front());
        response_queue_.pop();
        return resp;
    }

    bool memory_store_client::set(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        if (!is_valid_key(key)) {
            if (logger_) logger_->error("[memory_store_client] Invalid key format: '{}'", key);
            return false;
        }

        // 값 내부의 이스케이프 문자 인코딩
        std::string escaped_val;
        escaped_val.reserve(value.size() + 16);
        for (char ch : value) {
            if (ch == '\\') escaped_val += "\\\\";
            else if (ch == '"') escaped_val += "\\\"";
            else if (ch == '\n') escaped_val += "\\n";
            else if (ch == '\r') escaped_val += "\\r";
            else escaped_val += ch;
        }

        auto res_opt = send_and_wait_locked("SET " + key + " \"" + escaped_val + "\"", timeout_);
        if (res_opt && *res_opt == "+OK") {
            return true;
        }

        if (logger_) logger_->error("[memory_store_client] SET failed: {}", res_opt ? *res_opt : "No response");
        return false;
    }

    std::optional<std::string> memory_store_client::get(const std::string& key) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        if (!is_valid_key(key)) {
            if (logger_) logger_->error("[memory_store_client] Invalid key format: '{}'", key);
            return std::nullopt;
        }

        auto res_opt = send_and_wait_locked("GET " + key, timeout_);
        if (!res_opt) {
            if (logger_) logger_->error("[memory_store_client] GET command failed: No response from server");
            return std::nullopt;
        }

        std::string res = *res_opt;
        if (res.empty() || res == "$-1" || res.rfind("-ERR", 0) == 0)
            return std::nullopt;
        if (res[0] == '+')
            return res.substr(1);
        return res;
    }

    int memory_store_client::del(const std::string& key) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        return del_locked(key);
    }

    int memory_store_client::del_locked(const std::string& key) {
        if (!is_valid_key(key)) {
            if (logger_) logger_->error("[memory_store_client] Invalid key format: '{}'", key);
            return 0;
        }

        auto res_opt = send_and_wait_locked("DEL " + key, timeout_);
        if (!res_opt) {
            if (logger_) logger_->error("[memory_store_client] DEL command failed: No response from server");
            return 0;
        }

        std::string res = *res_opt;
        if (!res.empty() && res[0] == ':') {
            try {
                return std::stoi(res.substr(1));
            }
            catch (const std::exception& e) {
                if (logger_) logger_->error("[memory_store_client] Failed to parse DEL response '{}': {}", res, e.what());
                return 0;
            }
        }
        return 0;
    }

    int memory_store_client::delete_all() {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] delete_all failed: Disconnected");
            return -1;
        }

        long latency_ms = request_server_latency_ms();
        std::chrono::seconds dump_timeout = timeout_;
        if (latency_ms > 0) {
            dump_timeout = std::chrono::seconds(std::max<long>(timeout_.count(), (latency_ms / 1000) + 3));
        }

        auto entries = request_server_dump(dump_timeout);
        if (entries.empty()) {
            return 0;
        }

        int deleted_count = 0;
        for (const auto& [key, _] : entries) {
            deleted_count += del_locked(key);
        }

        if (logger_) {
            logger_->info("[memory_store_client] delete_all completed. Total {} keys erased.", deleted_count);
        }

        return deleted_count;
    }

    bool memory_store_client::request_server_save() {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        return send_and_wait_locked("SAVE", timeout_) == "+OK";
    }

    bool memory_store_client::request_server_load() {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        return send_and_wait_locked("LOAD", timeout_) == "+OK";
    }

    std::vector<std::pair<std::string, std::string>> memory_store_client::request_server_dump(std::chrono::seconds timeout) {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] DUMP failed: Disconnected");
            return {};
        }

        std::vector<std::pair<std::string, std::string>> results;
        auto start = std::chrono::steady_clock::now();

        {
            std::unique_lock<std::mutex> lock(response_mutex_);
            while (!response_queue_.empty()) {
                response_queue_.pop();
            }

            if (client_.send_data("DUMP\n") < 0) {
                if (logger_) logger_->error("[memory_store_client] Failed to write DUMP command to socket");
                return {};
            }

            while (true) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
                if (elapsed >= timeout) {
                    if (logger_) logger_->error("[memory_store_client] DUMP request timed out");
                    break;
                }
                auto remaining = timeout - elapsed;

                bool got = response_cv_.wait_for(lock, remaining, [this] {
                    return !response_queue_.empty() || !client_.is_connected();
                    });

                if (!got || !client_.is_connected()) {
                    if (logger_) logger_->error("[memory_store_client] Timeout or network failure during DUMP");
                    break;
                }

                while (!response_queue_.empty()) {
                    std::string resp = std::move(response_queue_.front());
                    response_queue_.pop();

                    if (resp == "+EMPTY" || resp == "+END") {
                        return results;
                    }

                    if (!resp.empty() && resp[0] == '+') {
                        std::string payload = resp.substr(1);
                        auto pos = payload.find(' ');
                        if (pos == std::string::npos) {
                            results.emplace_back(payload, std::string());
                        }
                        else {
                            results.emplace_back(payload.substr(0, pos), payload.substr(pos + 1));
                        }
                    }
                }
            }
        }

        return results;
    }

    long memory_store_client::request_server_sleep_ms() {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        auto res_opt = send_and_wait_locked("SLEEP_MS", timeout_);
        if (!res_opt) return -1;

        std::string res = *res_opt;
        if (!res.empty() && res[0] == ':') {
            try {
                return std::stol(res.substr(1));
            }
            catch (...) {
                return -1;
            }
        }
        return -1;
    }

    long memory_store_client::request_server_latency_ms() {
        std::lock_guard<std::recursive_mutex> req_lock(request_mutex_);
        auto res_opt = send_and_wait_locked("LATENCY", timeout_);
        if (!res_opt) return -1;

        std::string res = *res_opt;
        if (!res.empty() && res[0] == ':') {
            try {
                return std::stol(res.substr(1));
            }
            catch (...) {
                return -1;
            }
        }
        return -1;
    }

}

