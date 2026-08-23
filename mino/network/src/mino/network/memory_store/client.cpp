#include <chrono>
#include <stdexcept>

#include "mino/network/memory_store/client.hpp"
#include "mino/network/memory_store/protocol.hpp"

namespace mino::network::memory_store {

    namespace {
        inline bool contains_quote(const std::string& s) {
            return s.find('"') != std::string::npos;
        }

        inline bool contains_whitespace(const std::string& s) {
            for (unsigned char c : s) {
                if (std::isspace(c))
                    return true;
            }
            return false;
        }

        inline std::string maybe_quote(const std::string& s) {
            if (s.empty())
                return s;
            if (contains_quote(s))
                return std::string(); // signal invalid (empty => invalid)
            if (contains_whitespace(s)) {
                return std::string("\"") + s + std::string("\"");
            }
            return s;
        }
    } // namespace 

    memory_store_client::memory_store_client() :
        timeout_(std::chrono::seconds(5)),
        logger_(nullptr),
        current_response_{},
        has_response_(false),
        remote_ip_{},
        remote_port_(0)
    {
        client_.set_on_receive([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(response_mutex_);
            current_response_ = data;
            if (!current_response_.empty() && current_response_.back() == '\n') {
                current_response_.pop_back();
            }
            has_response_ = true;
            response_cv_.notify_one();
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
    }

    std::optional<std::string> memory_store_client::send_and_wait(const std::string& command, std::chrono::seconds timeout) {
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] Send failed: Disconnected");
            return std::nullopt;
        }

        std::unique_lock<std::mutex> lock(response_mutex_);
        has_response_ = false;
        current_response_.clear();

        if (client_.send_data(command + "\n") < 0) {
            if (logger_) logger_->error("[memory_store_client] Failed to write socket buffer");
            return std::nullopt;
        }

        bool success = response_cv_.wait_for(lock, timeout, [this] { return has_response_; });
        if (!success || !client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] Request timeout or network failure during wait.");
            return std::nullopt;
        }

        return current_response_;
    }

    bool memory_store_client::set(const std::string& key, const std::string& value) {
        std::string qkey = maybe_quote(key);
        if (qkey.empty()) {
            if (logger_) logger_->error("[memory_store_client] Invalid key contains double quote: '{}'", key);
            return false;
        }

        auto res_opt = send_and_wait("SET " + qkey + " \"" + value + "\"", timeout_);
        if (res_opt) {
            if (*res_opt == "+OK") {
                return true;
            }
            else {
                if (logger_) logger_->error("[memory_store_client] SET command failed: {}", *res_opt);
                return false;
            }
        }
        else {
            if (logger_) logger_->error("[memory_store_client] SET command failed: No response from server");
            return false;
        }
    }

    std::optional<std::string> memory_store_client::get(const std::string& key) {
        std::string qkey = maybe_quote(key);
        if (qkey.empty()) {
            if (logger_) logger_->error("[memory_store_client] Invalid key contains double quote: '{}'", key);
            return std::nullopt;
        }

        auto res_opt = send_and_wait("GET " + qkey, timeout_);
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
        std::string qkey = maybe_quote(key);
        if (qkey.empty()) {
            if (logger_) logger_->error("[memory_store_client] Invalid key contains double quote: '{}'", key);
            return 0;
        }

        auto res_opt = send_and_wait("DEL " + qkey, timeout_);
        if (!res_opt) {
            if (logger_) logger_->error("[memory_store_client] DEL command failed: No response from server");
            return 0;
        }

        std::string res = *res_opt;
        if (!res.empty() && res[0] == ':')
            return std::stoi(res.substr(1));

        return 0;
    }

    int memory_store_client::delete_all() {
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] delete_all failed: Disconnected");
            return -1;
        }

        // 1. latency를 조회하여 dump 요청 대기 시간 계산
        long latency_ms = request_server_latency_ms();
        std::chrono::seconds dump_timeout = timeout_;
        if (latency_ms > 0) {
            dump_timeout = std::chrono::seconds(std::max<long>(timeout_.count(), (latency_ms / 1000) + 3));
        }

        // 2. 서버에 있는 모든 키/값 덤프
        auto entries = request_server_dump(dump_timeout);
        if (entries.empty()) {
            return 0;
        }

        // 3. 각 키를 순회하며 삭제
        int deleted_count = 0;
        for (const auto& [key, _] : entries) {
            deleted_count += del(key);
        }

        if (logger_) {
            logger_->info("[memory_store_client] delete_all completed. Total {} keys erased.", deleted_count);
        }

        return deleted_count;
    }

    bool memory_store_client::request_server_save() {
        return send_and_wait("SAVE", timeout_) == "+OK";
    }

    bool memory_store_client::request_server_load() {
        return send_and_wait("LOAD", timeout_) == "+OK";
    }

    std::vector<std::pair<std::string, std::string>> memory_store_client::request_server_dump(std::chrono::seconds timeout) {
        if (!client_.is_connected()) {
            if (logger_) logger_->error("[memory_store_client] DUMP failed: Disconnected");
            return {};
        }

        std::vector<std::pair<std::string, std::string>> results;
        auto start = std::chrono::steady_clock::now();

        {
            std::unique_lock<std::mutex> lock(response_mutex_);
            has_response_ = false;
            current_response_.clear();

            if (client_.send_data(std::string("DUMP\n")) < 0) {
                if (logger_) logger_->error("[memory_store_client] Failed to write DUMP command to socket");
                return {};
            }

            while (true) {
                auto now = std::chrono::steady_clock::now();
                if (now - start > timeout) {
                    if (logger_) logger_->error("[memory_store_client] DUMP request timed out");
                    break;
                }
                auto remaining = timeout - std::chrono::duration_cast<std::chrono::seconds>(now - start);

                if (logger_) logger_->info("[memory_store_client] Waiting for DUMP response chunk, remaining time: {} seconds", remaining.count());

                bool got = response_cv_.wait_for(lock, remaining, [this] { return has_response_; });
                if (!got) {
                    if (logger_) logger_->error("[memory_store_client] Timeout waiting for DUMP response chunk");
                    break;
                }

                std::string resp = current_response_;
                has_response_ = false;
                current_response_.clear();

                if (resp == "+EMPTY") {
                    break;
                }

                if (resp == "+END") {
                    break;
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

        return results;
    }

    long memory_store_client::request_server_sleep_ms() {
        auto res_opt = send_and_wait("SLEEP_MS", timeout_);
        if (!res_opt) {
            if (logger_) logger_->error("[memory_store_client] SLEEP_MS command failed: No response from server");
            return -1;
        }

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
        auto res_opt = send_and_wait("LATENCY", timeout_);
        if (!res_opt) {
            if (logger_) logger_->error("[memory_store_client] LATENCY command failed: No response from server");
            return -1;
        }

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
