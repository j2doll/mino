#include <iostream>
#include <cstring>

#include <spdlog/spdlog.h>

#include "mino/network/udp/udp_sender.hpp"

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

    // static 멤버 구현: 문자열 IP/포트로 sockaddr_storage 채움
    bool udp_sender::fill_sockaddr(const std::string& ip, unsigned short port, sockaddr_storage& addr, socklen_t& addr_len, int& family) {
        std::memset(&addr, 0, sizeof(addr));

        // 먼저 IPv4 시도
        sockaddr_in addr4{};
        if (inet_pton(AF_INET, ip.c_str(), &addr4.sin_addr) == 1) {
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(port);
            std::memcpy(&addr, &addr4, sizeof(addr4));
            addr_len = static_cast<socklen_t>(sizeof(sockaddr_in));
            family = AF_INET;
            return true;
        }

        // IPv6 시도
        sockaddr_in6 addr6{};
        if (inet_pton(AF_INET6, ip.c_str(), &addr6.sin6_addr) == 1) {
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(port);
            std::memcpy(&addr, &addr6, sizeof(addr6));
            addr_len = static_cast<socklen_t>(sizeof(sockaddr_in6));
            family = AF_INET6;
            return true;
        }

        // IP 문자열이 비어있는 경우 기본 family 사용
        if (ip.empty()) {
            if (family == AF_INET) {
                sockaddr_in def4{};
                def4.sin_family = AF_INET;
                def4.sin_port = htons(port);
                def4.sin_addr.s_addr = htonl(INADDR_ANY);
                std::memcpy(&addr, &def4, sizeof(def4));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in));
                return true;
            }
            else if (family == AF_INET6) {
                sockaddr_in6 def6{};
                def6.sin6_family = AF_INET6;
                def6.sin6_port = htons(port);
                def6.sin6_addr = in6addr_any;
                std::memcpy(&addr, &def6, sizeof(def6));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in6));
                return true;
            }
        }

        return false;
    }

    udp_sender::udp_sender()
#ifdef _WIN32
        : socket_fd(INVALID_SOCKET)
#else
        : socket_fd(-1)
#endif
    {
    }

    udp_sender::~udp_sender() {
        stop();
    }

    void udp_sender::set_server(const std::string& ip, unsigned short port) {
        server_ip = ip;
        server_port = port;
        // address_family는 기본 AF_INET이나, fill_sockaddr가 감지한 값으로 변경됩니다.
        int fam = address_family;
        fill_sockaddr(server_ip, server_port, server_addr, server_addr_len, fam);
        address_family = fam;
    }

    bool udp_sender::create() {
#ifdef _WIN32
        socket_fd = socket(address_family, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd == INVALID_SOCKET) {
            return false;
        }
#else
        socket_fd = socket(address_family, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            return false;
        }
#endif

        // SO_REUSEADDR 허용 (멀티캐스트/바인드/테스트 목적)
        int reuse = 1;
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), static_cast<socklen_t>(sizeof(reuse)));

        return true;
    }

    ssize_t udp_sender::send_data(const std::string& data) {
#ifdef _WIN32
        if (socket_fd == INVALID_SOCKET) return -1;
#else
        if (socket_fd < 0) return -1;
#endif
        std::lock_guard<std::mutex> lock(send_mutex);
        return sendto(socket_fd, data.c_str(), static_cast<int>(data.size()), 0, reinterpret_cast<struct sockaddr*>(&server_addr), server_addr_len);
    }

    ssize_t udp_sender::send_data_to(const std::string& data, const std::string& ip, unsigned short port) {
#ifdef _WIN32
        if (socket_fd == INVALID_SOCKET) return -1;
#else
        if (socket_fd < 0) return -1;
#endif
        sockaddr_storage dest{};
        socklen_t dest_len = 0;
        int fam = AF_UNSPEC;
        if (!fill_sockaddr(ip, port, dest, dest_len, fam)) return -1;

        std::lock_guard<std::mutex> lock(send_mutex);
        return sendto(socket_fd, data.c_str(), static_cast<int>(data.size()), 0, reinterpret_cast<struct sockaddr*>(&dest), dest_len);
    }

    void udp_sender::set_multicast_ttl(int ttl) {
        if (
#ifdef _WIN32
            socket_fd == INVALID_SOCKET
#else
            socket_fd < 0
#endif
            ) return;

        if (address_family == AF_INET) {
            unsigned char v = static_cast<unsigned char>(ttl);
            setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&v), static_cast<socklen_t>(sizeof(v)));
        }
        else if (address_family == AF_INET6) {
            int v = ttl;
            setsockopt(socket_fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&v), static_cast<socklen_t>(sizeof(v)));
        }
    }

    void udp_sender::stop() {
#ifdef _WIN32
        if (socket_fd == INVALID_SOCKET) return;
        closesocket(socket_fd);
        socket_fd = INVALID_SOCKET;
#else
        if (socket_fd < 0) return;
        close(socket_fd);
        socket_fd = -1;
#endif
    }

    void udp_sender::set_logger(std::shared_ptr<spdlog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    void udp_sender::shutdown_by_force() {
        try {

            // 강제 종료: SO_LINGER{l_onoff=1, l_linger=0} 적용 후 즉시 닫음
#ifdef _WIN32
            if (socket_fd == INVALID_SOCKET) return;
#else
            if (socket_fd < 0) return;
#endif

            // 잠깐 락으로 동시 접근 방지
            std::lock_guard<std::mutex> lock(send_mutex);
            close_socket_without_timewait(socket_fd);
#ifdef _WIN32
            socket_fd = INVALID_SOCKET;
#else
            socket_fd = -1;
#endif

        }
        catch (const std::exception& e) {
            if (logger) logger->error("Exception in shutdown_by_force: {}", e.what());
            else std::cerr << "Exception in shutdown_by_force: " << e.what() << std::endl;
        }
        catch (...) {
            if (logger) logger->error("Unknown exception in shutdown_by_force");
            else std::cerr << "Unknown exception in shutdown_by_force" << std::endl;
        }
    }

}  
