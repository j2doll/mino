#pragma once

#include <string>
#include <mutex>
#include <memory>

#include <spdlog/spdlog.h>

#include "mino/network/ethernet.hpp"

namespace mino::network::udp {

    class  udp_sender {
    protected:
        std::string server_ip;
        unsigned short server_port;
        int address_family = AF_INET;

        socket_t socket_fd;

        sockaddr_storage server_addr{};
        socklen_t server_addr_len = 0;
        std::mutex send_mutex; 

        static bool fill_sockaddr(const std::string& ip, unsigned short port, sockaddr_storage& addr, socklen_t& addr_len, int& family);

        // spdlog 로거 (등록되지 않을 수 있음)
        std::shared_ptr<spdlog::logger> logger;

    public:
        udp_sender();
        ~udp_sender();

        void set_server(const std::string& ip, unsigned short port);
        void set_multicast_ttl(int ttl);

        bool create();

        ssize_t send_data(const std::string& data);
        ssize_t send_data_to(const std::string& data, const std::string& ip, unsigned short port);

        void stop();
        void shutdown_by_force();

        // spdlog 로거 등록
        void set_logger(std::shared_ptr<spdlog::logger> logger_ptr);

    };

}  
