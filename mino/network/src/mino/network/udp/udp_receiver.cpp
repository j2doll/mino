#include <iostream>
#include <cstring>
#include <cerrno>

#include <spdlog/spdlog.h>

#include "mino/network/udp/udp_receiver.hpp"

namespace mino::network::udp {

    // Helper: 소켓을 TIME_WAIT 없이 즉시 종료(RST)하도록 SO_LINGER 설정 후 닫습니다.
    static void close_socket_without_timewait(socket_t fd) {
        if (fd
#ifdef _WIN32
            == INVALID_SOCKET
#else
            < 0
#endif
        ) {
            return;
        }

        struct linger so_linger;
        so_linger.l_onoff = 1;    // linger 옵션 활성화
        so_linger.l_linger = 0;   // 0초 -> 즉시 RST 전송

#ifdef _WIN32
        setsockopt(fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&so_linger), static_cast<int>(sizeof(so_linger)));
        closesocket(fd);
#else
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, static_cast<socklen_t>(sizeof(so_linger)));
        close(fd);
#endif
    }

    udp_receiver::udp_receiver(ip_version_t version)
        : is_running(false),
#ifdef _WIN32
        server_socket(INVALID_SOCKET), // [변경]
#else
        server_socket(-1),             // [변경]
#endif
        receiver_ip(""), receiver_port(0), udp_type(udp_type_t::none),
        any_address(version == ip_version_t::ipv4 ? "0.0.0.0" : "::"),
        enable_reuse_port(true), ip_version(version) {
    }

    void udp_receiver::set_ip_version(ip_version_t version) {
        ip_version = version;
        any_address = (version == ip_version_t::ipv4) ? "0.0.0.0" : "::";
    }

    udp_receiver::~udp_receiver() {
        quit();
    }

    void udp_receiver::set_enable_reuse_port(bool enable) {
        enable_reuse_port = enable;
    }

    void udp_receiver::set_any_address(const std::string& any_addr) {
        any_address = any_addr;
    }

    void udp_receiver::set_logger(std::shared_ptr<spdlog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    bool udp_receiver::start_unicast(const std::string& ip, const unsigned short port) {
        if (!start(ip, port)) {
            udp_type = udp_type_t::none; receiver_ip.clear(); receiver_port = 0;
            return false;
        }
        udp_type = udp_type_t::unicast; receiver_ip = ip; receiver_port = port;
        return true;
    }

    bool udp_receiver::start_multicast(const std::string& multicast_group, const unsigned short port) {
        if (!start(any_address, port, true, multicast_group)) {
            udp_type = udp_type_t::none; receiver_ip.clear(); receiver_port = 0;
            return false;
        }
        udp_type = udp_type_t::multicast; receiver_ip = multicast_group; receiver_port = port;
        return true;
    }

    bool udp_receiver::start_broadcast(const unsigned short port) {
        if (!start(any_address, port, false, "", true)) {
            udp_type = udp_type_t::none; receiver_ip.clear(); receiver_port = 0;
            return false;
        }
        udp_type = udp_type_t::broadcast; receiver_ip = any_address; receiver_port = port;
        return true;
    }

    bool udp_receiver::start(const std::string& ip, const unsigned short port,
        const bool enable_multicast, const std::string& multicast_group, const bool enable_broadcast) {
        int family = (ip_version == ip_version_t::ipv4) ? AF_INET : AF_INET6;
        server_socket = socket(family, SOCK_DGRAM, 0);

        // [변경] socket_t 기반 에러 체크
#ifdef _WIN32
        if (server_socket == INVALID_SOCKET) {
#else
        if (server_socket < 0) {
#endif
            if (logger) logger->error("Socket creation failed: {}", std::strerror(errno));
            else perror("Socket creation failed");
            return false;
        }

        if (enable_reuse_port) {
            int reuse = 1;
            if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
                if (logger) logger->error("Failed to set SO_REUSEADDR: {}", std::strerror(errno));
                else perror("Failed to set SO_REUSEADDR");
                return false;
            }
        }

        if (enable_broadcast && ip_version == ip_version_t::ipv4) {
            int broadcast = 1;
            if (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) < 0) {
                if (logger) logger->error("Failed to enable broadcast: {}", std::strerror(errno));
                else perror("Failed to enable broadcast");
                return false;
            }
        }

        if (ip_version == ip_version_t::ipv4) {
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

            if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                if (logger) logger->error("Bind failed: {}", std::strerror(errno));
                else perror("Bind failed");
#ifdef _WIN32
                closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
                close(server_socket); server_socket = -1;
#endif
                return false;
            }

            if (enable_multicast && !multicast_group.empty()) {
                struct ip_mreq mreq {};
                inet_pton(AF_INET, multicast_group.c_str(), &mreq.imr_multiaddr);
                mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                if (setsockopt(server_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) < 0) {
                    if (logger) logger->error("Failed to join multicast group: {}", std::strerror(errno));
                    else perror("Failed to join multicast group");
#ifdef _WIN32
                    closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
                    close(server_socket); server_socket = -1;
#endif
                    return false;
                }
            }
        }
        else { // IPv6
            sockaddr_in6 server_addr6{};
            server_addr6.sin6_family = AF_INET6;
            server_addr6.sin6_port = htons(port);
            inet_pton(AF_INET6, ip.c_str(), &server_addr6.sin6_addr);

            if (bind(server_socket, (struct sockaddr*)&server_addr6, sizeof(server_addr6)) < 0) {
                if (logger) logger->error("Bind failed: {}", std::strerror(errno));
                else perror("Bind failed");
#ifdef _WIN32
                closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
                close(server_socket); server_socket = -1;
#endif
                return false;
            }

            if (enable_multicast && !multicast_group.empty()) {
                struct ipv6_mreq mreq6 {};
                inet_pton(AF_INET6, multicast_group.c_str(), &mreq6.ipv6mr_multiaddr);
                mreq6.ipv6mr_interface = 0;
                if (setsockopt(server_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*)&mreq6, sizeof(mreq6)) < 0) {
                    if (logger) logger->error("Failed to join IPv6 multicast group: {}", std::strerror(errno));
                    else perror("Failed to join IPv6 multicast group");
#ifdef _WIN32
                    closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
                    close(server_socket); server_socket = -1;
#endif
                    return false;
                }
            }
        }

        is_running = true;
        receiver_thread = std::thread(&udp_receiver::receive_loop, this);
        return true;
        }

    void udp_receiver::set_on_receive_callback(callback cb) {
        on_receive = std::move(cb);
    }

    void udp_receiver::quit() {
        if (is_running) {
            is_running = false;

            // [변경] 소켓 폐쇄 처리를 join보다 먼저 수행하여 recvfrom 블로킹 상태를 깨웁니다.
#ifdef _WIN32
            if (server_socket != INVALID_SOCKET) {
                closesocket(server_socket); server_socket = INVALID_SOCKET;
            }
#else
            if (server_socket >= 0) {
                close(server_socket); server_socket = -1;
            }
#endif

            if (receiver_thread.joinable()) {
                receiver_thread.join();
            }
        }
    }

    void udp_receiver::shutdown_by_force() {
        try {

            // 강제 종료: SO_LINGER{l_onoff=1, l_linger=0} 적용 후 즉시 닫음
            if (is_running) {
                is_running = false;
            }

#ifdef _WIN32
            if (server_socket != INVALID_SOCKET) {
                close_socket_without_timewait(server_socket);
                server_socket = INVALID_SOCKET;
            }
#else
            if (server_socket >= 0) {
                close_socket_without_timewait(server_socket);
                server_socket = -1;
            }
#endif

            if (receiver_thread.joinable()) {
                receiver_thread.join();
            }

        } catch (const std::exception& ex) {
            if (logger) logger->error("Exception in shutdown_by_force: {}", ex.what());
            else std::cerr << "Exception in shutdown_by_force: " << ex.what() << std::endl;
        } catch (...) {
            if (logger) logger->error("Unknown exception in shutdown_by_force");
            else std::cerr << "Unknown exception in shutdown_by_force" << std::endl;
        }
    }

    void udp_receiver::receive_loop() {
        char buffer[buffer_size];

        if (ip_version == ip_version_t::ipv4) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            while (is_running) {
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &client_len);
                if (bytes_received < 0) {
                    if (is_running) {
                        if (logger) logger->error("Receive failed: {}", std::strerror(errno));
                        else perror("Receive failed");
                    }
                    continue;
                }
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                uint16_t client_port = ntohs(client_addr.sin_port);
                if (on_receive) on_receive(std::string(buffer, bytes_received), std::string(client_ip), client_port);
            }
        }
        else { // IPv6
            sockaddr_in6 client_addr6{};
            socklen_t client_len6 = sizeof(client_addr6);
            while (is_running) {
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr6, &client_len6);
                if (bytes_received < 0) {
                    if (is_running) {
                        if (logger) logger->error("Receive failed: {}", std::strerror(errno));
                        else perror("Receive failed");
                    }
                    continue;
                }
                char client_ip[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &client_addr6.sin6_addr, client_ip, INET6_ADDRSTRLEN);
                uint16_t client_port = ntohs(client_addr6.sin6_port);
                if (on_receive) on_receive(std::string(buffer, bytes_received), std::string(client_ip), client_port);
            }
        }
    }

} 
