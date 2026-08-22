#pragma once

#include <string>
#include <mutex>
#include <memory>

#include <spdlog/spdlog.h>

#include "mino/network/ethernet.hpp"

namespace mino::network::udp {

    class  udp_sender {
    protected:
        std::string server_ip_;
        unsigned short server_port_;
        int address_family_ = AF_INET;

        socket_t socket_fd_;

        sockaddr_storage server_addr_{};
        socklen_t server_addr_len_ = 0;
        std::mutex send_mutex_;

        static bool fill_sockaddr(const std::string& ip, unsigned short port, sockaddr_storage& addr, socklen_t& addr_len, int& family);

        // spdlog 로거 (등록되지 않을 수 있음)
        std::shared_ptr<spdlog::logger> logger;

    public:
        udp_sender();
        ~udp_sender();

        void set_server(const std::string& ip, unsigned short port, int family = AF_INET);
        bool set_multicast_ttl(int ttl);

        bool is_created() const;
        bool is_ipv4() const;
        bool is_ipv6() const;
        bool create(int fam, bool resur_add, bool use_multicast);

        ssize_t send_data(const std::string& data);
        ssize_t send_data_to(const std::string& data, const std::string& ip, unsigned short port);

        void stop();
        void shutdown_by_force();

        // spdlog 로거 등록
        void set_logger(std::shared_ptr<spdlog::logger> logger_ptr);

    };

}  
