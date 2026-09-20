#include "mino/network_openssl/redis/basic_redis_client.hpp"
#include "mino/network_openssl/redis/resp_parser.hpp"

#include <thread>
#include <stdexcept>

namespace mino::network_openssl::redis {

    template <typename transport_t>
    basic_redis_client<transport_t>::basic_redis_client() {
        transport_layer.set_on_receive([this](const std::string& chunk) {
            this->handle_network_receive(chunk);
            });  

                transport_layer.set_on_close([this]() {
                this->clear_pending_requests();
                    });  
    }

    template <typename transport_t>
    basic_redis_client<transport_t>::~basic_redis_client() {
        disconnect();
    }

    template <typename transport_t>
    void basic_redis_client<transport_t>::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        transport_layer.set_logger(std::move(logger_ptr));  
    }

    template <typename transport_t>
    void basic_redis_client<transport_t>::configure_tls(const redis_tls_config& config) {
        if constexpr (std::is_same_v<transport_t, mino::network_openssl::tls::tls_client>) {
            transport_layer.set_verify_peer(config.verify_peer);  
                if (!config.ca_cert_file.empty() || !config.ca_cert_path.empty()) {
                    transport_layer.set_ca_cert(config.ca_cert_file, config.ca_cert_path);  
                }
            if (!config.sni_hostname.empty()) {
                transport_layer.set_sni_hostname(config.sni_hostname);  
            }
            if (!config.client_cert_file.empty() && !config.client_key_file.empty()) {
                transport_layer.set_client_certificate(config.client_cert_file, config.client_key_file);  
            }
        }
        else {
            (void)config;
        }
    }

    template <typename transport_t>
    bool basic_redis_client<transport_t>::connect(const std::string& ip,
        unsigned short port,
        int family,
        std::chrono::milliseconds wait_limit) {
        transport_layer.set_server(ip, port, family);  
            if (!transport_layer.start()) {
                 
                return false;
            }

        auto start_time = std::chrono::steady_clock::now();
        while (!transport_layer.is_connected()) {
             
            if (std::chrono::steady_clock::now() - start_time > wait_limit) {
                transport_layer.stop();  
                    return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    template <typename transport_t>
    void basic_redis_client<transport_t>::disconnect() {
        transport_layer.stop();  
            clear_pending_requests();
    }

    template <typename transport_t>
    bool basic_redis_client<transport_t>::is_connected() const {
        return transport_layer.is_connected(); 
    }

    template <typename transport_t>
    redis_value basic_redis_client<transport_t>::execute_command(const std::vector<std::string>& command_args,
        std::chrono::milliseconds timeout) {
        if (!transport_layer.is_connected()) {
            
            throw std::runtime_error("redis client is not connected");
        }

        std::future<redis_value> future_response;
        std::string serialized_req = serialize_to_resp_array(command_args);

        {
            std::lock_guard<std::mutex> lock(request_mutex);
            std::promise<redis_value> promise_response;
            future_response = promise_response.get_future();
            pending_requests.push(std::move(promise_response));
        }

        if (transport_layer.send_data(serialized_req) < 0) {
             
            throw std::runtime_error("failed to send data through transport layer");
        }

        if (future_response.wait_for(timeout) == std::future_status::timeout) {
            throw std::runtime_error("redis command execution timed out");
        }

        return future_response.get();
    }

    template <typename transport_t>
    bool basic_redis_client<transport_t>::auth(const std::string& password) {
        auto res = execute_command({ "AUTH", password });
        auto str_val = res.as_string();
        return str_val.has_value() && *str_val == "OK";
    }

    template <typename transport_t>
    bool basic_redis_client<transport_t>::auth(const std::string& username, const std::string& password) {
        auto res = execute_command({ "AUTH", username, password });
        auto str_val = res.as_string();
        return str_val.has_value() && *str_val == "OK";
    }

    template <typename transport_t>
    std::string basic_redis_client<transport_t>::ping(const std::string& message) {
        std::vector<std::string> cmd = { "PING" };
        if (!message.empty()) cmd.push_back(message);
        auto res = execute_command(cmd);
        return res.as_string().value_or("");
    }

    template <typename transport_t>
    bool basic_redis_client<transport_t>::set(const std::string& key, const std::string& value) {
        auto res = execute_command({ "SET", key, value });
        auto str_val = res.as_string();
        return str_val.has_value() && *str_val == "OK";
    }

    template <typename transport_t>
    std::optional<std::string> basic_redis_client<transport_t>::get(const std::string& key) {
        auto res = execute_command({ "GET", key });
        if (res.is_null()) return std::nullopt;
        return res.as_string();
    }

    template <typename transport_t>
    std::string basic_redis_client<transport_t>::serialize_to_resp_array(const std::vector<std::string>& args) {
        std::string serialized = "*" + std::to_string(args.size()) + "\r\n";
        for (const auto& arg : args) {
            serialized += "$" + std::to_string(arg.size()) + "\r\n";
            serialized += arg;
            serialized += "\r\n";
        }
        return serialized;
    }

    template <typename transport_t>
    void basic_redis_client<transport_t>::handle_network_receive(const std::string& chunk) {
        std::lock_guard<std::mutex> buf_lock(buffer_mutex);
        incoming_buffer.append(chunk);

        while (!incoming_buffer.empty()) {
            redis_value parsed_val;
            size_t bytes_consumed = 0;

            bool success = resp_parser::parse(incoming_buffer, parsed_val, bytes_consumed);
            if (!success || bytes_consumed == 0) {
                break;
            }

            incoming_buffer.erase(0, bytes_consumed);

            std::promise<redis_value> target_promise;
            {
                std::lock_guard<std::mutex> req_lock(request_mutex);
                if (!pending_requests.empty()) {
                    target_promise = std::move(pending_requests.front());
                    pending_requests.pop();
                }
                else {
                    continue;
                }
            }
            target_promise.set_value(std::move(parsed_val));
        }
    }

    template <typename transport_t>
    void basic_redis_client<transport_t>::clear_pending_requests() {
        std::lock_guard<std::mutex> lock(request_mutex);
        while (!pending_requests.empty()) {
            try {
                pending_requests.front().set_exception(
                    std::make_exception_ptr(std::runtime_error("connection closed unexpectedly"))
                );
            }
            catch (...) {}
            pending_requests.pop();
        }
    }

    // 명시적 템플릿 인스턴스화
    template class basic_redis_client<mino::network::tcp::tcp_client>; 
    template class basic_redis_client<mino::network_openssl::tls::tls_client>;  

}  
