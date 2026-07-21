#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <memory>

#include <spdlog/fwd.h>

#include "mino/network/ethernet.hpp"

namespace mino::network::udp {

    class  udp_receiver {
    public:
        using callback = std::function<void(const std::string&, const std::string&, uint16_t)>;
        enum class ip_version_t { ipv4, ipv6 };

    protected:
        socket_t server_socket;
        std::atomic<bool> is_running;
        std::thread receiver_thread;

        callback on_receive;
        static constexpr int buffer_size = 1024;

        enum class udp_type_t { none, unicast, multicast, broadcast };
        udp_type_t udp_type;

        std::string receiver_ip;
        unsigned short receiver_port;
        std::string any_address;

        bool enable_reuse_port;
        ip_version_t ip_version;

        // spdlog 로거 (등록되지 않을 수 있음)
        std::shared_ptr<spdlog::logger> logger;

    public:
        udp_receiver(ip_version_t version = ip_version_t::ipv4);
        ~udp_receiver();

        void set_enable_reuse_port(bool enable);
        void set_any_address(const std::string& any_addr);
        void set_ip_version(ip_version_t version);
        void set_on_receive_callback(callback cb);

        // spdlog 로거 등록
        void set_logger(std::shared_ptr<spdlog::logger> logger_ptr);

        bool start_unicast(const std::string& ip, const unsigned short port);
        bool start_multicast(const std::string& multicast_group, const unsigned short port);
        bool start_broadcast(const unsigned short port);

        void quit();
        void shutdown_by_force();

    protected:
        bool start(const std::string& ip, const unsigned short port,
            const bool enable_multicast = false,
            const std::string& multicast_group = "",
            const bool enable_broadcast = false);

        void receive_loop();
    };

}   
