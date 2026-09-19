#pragma once

#include <string>
#include <vector>
#include <queue>
#include <future>
#include <mutex>
#include <chrono>
#include <memory>
#include <optional>

#include "mino/network/tcp/tcp_client.hpp"
#include "mino/network/tls/tls_client.hpp"
 
#include "mino/network/redis/redis_value.hpp"
#include "mino/network/redis/redis_tls_config.hpp"

namespace mino::network::redis {

    template <typename transport_t = mino::network::tcp::tcp_client>
    class basic_redis_client {
    private:
        transport_t transport_layer;

        std::mutex request_mutex;
        std::queue<std::promise<redis_value>> pending_requests;

        std::mutex buffer_mutex;
        std::string incoming_buffer;

    public:
        basic_redis_client();
        ~basic_redis_client();

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);
        void configure_tls(const redis_tls_config& config);

        bool connect(const std::string& ip,
            unsigned short port,
            int family = AF_INET,
            std::chrono::milliseconds wait_limit = std::chrono::milliseconds(3000));

        void disconnect();
        bool is_connected() const;

        redis_value execute_command(const std::vector<std::string>& command_args,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

        bool auth(const std::string& password);
        bool auth(const std::string& username, const std::string& password);
        std::string ping(const std::string& message = "");
        bool set(const std::string& key, const std::string& value);
        std::optional<std::string> get(const std::string& key);

    private:
        static std::string serialize_to_resp_array(const std::vector<std::string>& args);
        void handle_network_receive(const std::string& chunk);
        void clear_pending_requests();
    };

    using redis_tcp_client = basic_redis_client<mino::network::tcp::tcp_client>; 
        using redis_tls_client = basic_redis_client<mino::network::tls::tls_client>; 

} // namespace mino::network::redis
