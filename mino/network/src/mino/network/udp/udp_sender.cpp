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
    bool udp_sender::fill_sockaddr(
        const std::string& ip, unsigned short port,
        sockaddr_storage& addr, socklen_t& addr_len,
        int& family)
    {
        std::memset(&addr, 0, sizeof(addr));

        if (family == AF_INET) { // IPv4

            if (ip.empty()) {
                sockaddr_in def4{};
                def4.sin_family = AF_INET;
                def4.sin_port = htons(port);
                def4.sin_addr.s_addr = htonl(INADDR_ANY);
                std::memcpy(&addr, &def4, sizeof(def4));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in));
                return true;
            }

            sockaddr_in addr4{};
            if (inet_pton(AF_INET, ip.c_str(), &addr4.sin_addr) == 1) {
                addr4.sin_family = AF_INET;
                addr4.sin_port = htons(port);
                std::memcpy(&addr, &addr4, sizeof(addr4));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in));
                family = AF_INET;
                return true;
            }

            return false;
        }

        if (family == AF_INET6) { // IPv6

            if (ip.empty()) {
                sockaddr_in6 def6{};
                def6.sin6_family = AF_INET6;
                def6.sin6_port = htons(port);
                def6.sin6_addr = in6addr_any;
                std::memcpy(&addr, &def6, sizeof(def6));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in6));
                return true;
            }

            sockaddr_in6 addr6{};
            if (inet_pton(AF_INET6, ip.c_str(), &addr6.sin6_addr) == 1) {
                addr6.sin6_family = AF_INET6;
                addr6.sin6_port = htons(port);
                std::memcpy(&addr, &addr6, sizeof(addr6));
                addr_len = static_cast<socklen_t>(sizeof(sockaddr_in6));
                family = AF_INET6;
                return true;
            }
            else {
                std::cerr << "Invalid IPv6 address: " << ip << std::endl;
            }

            return false;
        }
 
        return false;
    }

    udp_sender::udp_sender()
    {
        server_ip_.clear();
        server_port_ = 0;
        address_family_ = AF_INET;

#ifdef _WIN32
        socket_fd_ = INVALID_SOCKET;
#else
        socket_fd_ = -1;
#endif
        // memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_len_ = 0;
    }

    udp_sender::~udp_sender() {
        stop();
    }

    void udp_sender::set_server(const std::string& ip, unsigned short port, int fam) {
        server_ip_ = ip;
        server_port_ = port;

        // address_family는 기본 AF_INET이나, fill_sockaddr가 감지한 값으로 변경됩니다.
        fill_sockaddr(server_ip_, server_port_, server_addr_, server_addr_len_, fam);
        address_family_ = fam;
    }

    bool udp_sender::is_created() const {
#ifdef _WIN32
        if (socket_fd_ == INVALID_SOCKET) {
            return false;
        }
        return true;
#else
        if (socket_fd_ < 0) {
            return false;
        }
        return true;
#endif 
    }

    bool udp_sender::is_ipv4() const {
        if (address_family_ == AF_INET) {
            return true;
        }
        return false;
    }

    bool udp_sender::is_ipv6() const {
        if (address_family_ == AF_INET6) {
            return true;
        }
        return false;
    }

    bool udp_sender::create(int fam, bool resur_add, bool use_multicast) {

        address_family_ = fam; // ipv4:AF_INET, ipv6:AF_INET6

#ifdef _WIN32
        socket_fd_ = socket(fam, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd_ == INVALID_SOCKET) {
            int err = WSAGetLastError();
            if (logger) logger->error("socket() failed: {}", std::system_category().message(err));
            return false;
        }
#else
        socket_fd_ = socket(fam, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            int err = errno;
            if (logger) logger->error("socket() failed: {}", strerror(err));
            return false;
        }
#endif

        // SO_REUSEADDR (multicast/bind/test purposes)
        if (resur_add) {
            int reuse = 1;
#ifdef _WIN32
            if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), static_cast<int>(sizeof(reuse))) == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (logger) logger->warn("setsockopt(SO_REUSEADDR) failed: {}", std::system_category().message(err));
            }
#else
            if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, static_cast<socklen_t>(sizeof(reuse))) < 0) {
                int err = errno;
                if (logger) logger->warn("setsockopt(SO_REUSEADDR) failed: {}", strerror(err));
            }
#endif
        } // resur_add

        if (use_multicast) {
            // If IPv4 socket, allow broadcast sends (needed when destination is broadcast)
            if (address_family_ == AF_INET) {
                int broadcast = 1;
#ifdef _WIN32
                if (setsockopt(socket_fd_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&broadcast), static_cast<int>(sizeof(broadcast))) == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (logger) logger->warn("setsockopt(SO_BROADCAST) failed: {}", std::system_category().message(err));
                }
#else
                if (setsockopt(socket_fd_, SOL_SOCKET, SO_BROADCAST, &broadcast, static_cast<socklen_t>(sizeof(broadcast))) < 0) {
                    int err = errno;
                    if (logger) logger->warn("setsockopt(SO_BROADCAST) failed: {}", strerror(err));
                }
#endif
            } 

            if (address_family_ == AF_INET6) {
                // Optional: make socket IPv6-only to avoid ambiguous behavior on dual-stack systems.
                int v6only = 1;
#ifdef _WIN32
                if (setsockopt(socket_fd_, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only), static_cast<int>(sizeof(v6only))) == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (logger) logger->warn("setsockopt(IPV6_V6ONLY) failed: {}", std::system_category().message(err));
                }
#else
                if (setsockopt(socket_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, static_cast<socklen_t>(sizeof(v6only))) < 0) {
                    int err = errno;
                    if (logger) logger->warn("setsockopt(IPV6_V6ONLY) failed: {}", strerror(err));
                }
#endif

                // If server_addr was filled and contains a scope id (link-local or interface-scoped address),
                // set the outgoing multicast interface so IPv6 multicast/link-local packets use the correct NIC.
                if (server_addr_len_ >= static_cast<socklen_t>(sizeof(sockaddr_in6))) {
                    auto* sin6 = reinterpret_cast<sockaddr_in6*>(&server_addr_);
                    unsigned int ifidx = sin6->sin6_scope_id;
                    if (ifidx != 0) {
#ifdef _WIN32
                        if (setsockopt(socket_fd_, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast<const char*>(&ifidx), static_cast<int>(sizeof(ifidx))) == SOCKET_ERROR) {
                            int err = WSAGetLastError();
                            if (logger) logger->warn("setsockopt(IPV6_MULTICAST_IF) failed: {}", std::system_category().message(err));
                        }
#else
                        if (setsockopt(socket_fd_, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx, static_cast<socklen_t>(sizeof(ifidx))) < 0) {
                            int err = errno;
                            if (logger) logger->warn("setsockopt(IPV6_MULTICAST_IF) failed: {}", strerror(err));
                        }
#endif
                    }
                }
            }
        } // use_multicast

        return true;
    }

    ssize_t udp_sender::send_data(const std::string& data) {
#ifdef _WIN32
        if (socket_fd_ == INVALID_SOCKET)
            return -1;
#else
        if (socket_fd_ < 0)
            return -1;
#endif
        std::lock_guard<std::mutex> lock(send_mutex_);
        return sendto(socket_fd_, data.c_str(), static_cast<int>(data.size()), 0, reinterpret_cast<struct sockaddr*>(&server_addr_), server_addr_len_);
    }

    ssize_t udp_sender::send_data_to(const std::string& data, const std::string& ip, unsigned short port) {
#ifdef _WIN32
        if (socket_fd_ == INVALID_SOCKET)
            return -1;
#else
        if (socket_fd_ < 0)
            return -1;
#endif
        sockaddr_storage dest{};
        socklen_t dest_len = 0;

        int fam = address_family_; // AF_INET, AF_INET6
        if (!fill_sockaddr(ip, port, dest, dest_len, fam))
            return -1;

        int flags = 0;

        std::lock_guard<std::mutex> lock(send_mutex_);
        auto ret = sendto(
            socket_fd_,
            data.c_str(), static_cast<int>(data.size()),
            flags,
            reinterpret_cast<struct sockaddr*>(&dest), dest_len);

        if (ret < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            auto err_msg = std::system_category().message(err);
            if (logger) logger->error("sendto failed with error: {}", err_msg);
            else std::cerr << "sendto failed with error: " << err_msg << std::endl;
#else
            int err = errno;
            if (logger) logger->error("sendto failed with error: {}", strerror(err));
            else std::cerr << "sendto failed with error: " << strerror(err) <<
                std::endl;
#endif    
        }

        return ret;
    }

    bool udp_sender::set_multicast_ttl(int ttl) {
        if (
#ifdef _WIN32
            socket_fd_ == INVALID_SOCKET
#else
            socket_fd_ < 0
#endif
            ) {
            return false;
        }

        if (address_family_ == AF_INET) {
            unsigned char v = static_cast<unsigned char>(ttl);
            auto ret = setsockopt(socket_fd_, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&v), static_cast<socklen_t>(sizeof(v)));
            if (ret >= 0) {
                return true;
            }
        }

        if (address_family_ == AF_INET6) {
            int v = ttl;
            auto ret = setsockopt(socket_fd_, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&v), static_cast<socklen_t>(sizeof(v)));
            if (ret >= 0) {
                return true;
            }
        }

        return false;
    }

    void udp_sender::stop() {
#ifdef _WIN32
        if (socket_fd_ == INVALID_SOCKET) return;
        closesocket(socket_fd_);
        socket_fd_ = INVALID_SOCKET;
#else
        if (socket_fd_ < 0) return;
        close(socket_fd_);
        socket_fd_ = -1;
#endif
    }

    void udp_sender::set_logger(std::shared_ptr<spdlog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    void udp_sender::shutdown_by_force() {
        try {

            // 강제 종료: SO_LINGER{l_onoff=1, l_linger=0} 적용 후 즉시 닫음
#ifdef _WIN32
            if (socket_fd_ == INVALID_SOCKET) return;
#else
            if (socket_fd_ < 0) return;
#endif

            // 잠깐 락으로 동시 접근 방지
            std::lock_guard<std::mutex> lock(send_mutex_);
            close_socket_without_timewait(socket_fd_);
#ifdef _WIN32
            socket_fd_ = INVALID_SOCKET;
#else
            socket_fd_ = -1;
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
