#include <algorithm>
#include <cstdio>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <windows.h>
    #include <iphlpapi.h>
    #include <ws2tcpip.h>
#else
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
    #ifdef __linux__
        #include <netpacket/packet.h>
    #endif
#endif

#include "mino/network/util/network_util.hpp"

namespace mino::network::util {

    // Helper: convert prefix length (0..32) to uint32 netmask (host-order)
    static uint32_t prefixLengthToMask32(unsigned char prefixLength) {
        if (prefixLength == 0) return 0u;
        if (prefixLength >= 32) return 0xFFFFFFFFu;
        return 0xFFFFFFFFu << (32 - prefixLength);
    }

    bool is_valid_ipv4(const std::string& ip_address) {
        struct in_addr sa;
        return inet_pton(AF_INET, ip_address.c_str(), &sa) == 1;
    }

    bool is_valid_ipv6(const std::string& ip_address) {
        struct in6_addr sa6;
        return inet_pton(AF_INET6, ip_address.c_str(), &sa6) == 1;
    }

    bool is_multicast_ipv4(const std::string& ip_address) {
        struct in_addr sa;
        if (inet_pton(AF_INET, ip_address.c_str(), &sa) == 1) {
            uint32_t addr = ntohl(sa.s_addr);
            return (addr >= 0xE0000000 && addr <= 0xEFFFFFFF);
        }
        return false;
    }

    std::vector<network_interface_info> get_interfaces_sorted_by_priority() {
        std::vector<network_interface_info> interfaces;

#if defined(_WIN32)
        // Initialize Winsock for getnameinfo and related APIs.
        WSADATA wsaData;
        bool wsaStarted = false;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
            wsaStarted = true;
        }

        ULONG buf_size = 15000;
        IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)malloc(buf_size);
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

        ULONG rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapters, &buf_size);
        if (rc == ERROR_BUFFER_OVERFLOW) {
            adapters = (IP_ADAPTER_ADDRESSES*)realloc(adapters, buf_size);
            rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapters, &buf_size);
        }

        if (rc != ERROR_SUCCESS || adapters == nullptr) {
            if (adapters) free(adapters);
            if (wsaStarted) WSACleanup();
            return interfaces;
        }

        for (PIP_ADAPTER_ADDRESSES curr = adapters; curr != NULL; curr = curr->Next) {
            network_interface_info info{};
            info.name = curr->AdapterName ? curr->AdapterName : "";

            if (curr->Description) {
                char desc[1024] = { 0 };
                // Use WideCharToMultiByte to safely convert wide string to UTF-8 / ANSI.
                int converted = WideCharToMultiByte(CP_UTF8, 0, curr->Description, -1, desc, static_cast<int>(sizeof(desc)), NULL, NULL);
                if (converted > 0) info.description = desc;
            }

            if (curr->PhysicalAddressLength >= 6) {
                char mac[32] = { 0 };
                sprintf_s(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                    curr->PhysicalAddress[0], curr->PhysicalAddress[1], curr->PhysicalAddress[2],
                    curr->PhysicalAddress[3], curr->PhysicalAddress[4], curr->PhysicalAddress[5]);
                info.mac_address = mac;
            }

            for (PIP_ADAPTER_UNICAST_ADDRESS addr = curr->FirstUnicastAddress; addr != NULL; addr = addr->Next) {
                if (addr->Address.lpSockaddr == nullptr) continue;
                char buf[INET6_ADDRSTRLEN] = { 0 };
                if (getnameinfo(addr->Address.lpSockaddr, addr->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST) == 0) {
                    if (addr->Address.lpSockaddr->sa_family == AF_INET) {
                        info.ipv4_addresses.push_back(buf);
                        if (info.netmask.empty()) {
                            uint32_t m = prefixLengthToMask32(addr->OnLinkPrefixLength);
                            // Format from host-order mask directly
                            char maskbuf[16] = { 0 };
                            snprintf(maskbuf, sizeof(maskbuf), "%u.%u.%u.%u",
                                (m >> 24) & 0xFF,
                                (m >> 16) & 0xFF,
                                (m >> 8) & 0xFF,
                                m & 0xFF);
                            info.netmask = maskbuf;
                        }
                    }
                    else if (addr->Address.lpSockaddr->sa_family == AF_INET6) {
                        info.ipv6_addresses.push_back(buf);
                    }
                }
            }

            for (PIP_ADAPTER_DNS_SERVER_ADDRESS dns = curr->FirstDnsServerAddress; dns != NULL; dns = dns->Next) {
                char buf[INET6_ADDRSTRLEN] = { 0 };
                if (dns->Address.lpSockaddr && getnameinfo(dns->Address.lpSockaddr, dns->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST) == 0) {
                    info.dns_servers.push_back(buf);
                }
            }

            info.is_up = (curr->OperStatus == IfOperStatusUp);
            info.is_loopback = (curr->IfType == IF_TYPE_SOFTWARE_LOOPBACK);
            info.is_dhcp_enabled = false;

            uint32_t v4 = curr->Ipv4Metric;
            uint32_t v6 = curr->Ipv6Metric;
            if (v4 == 0 && v6 == 0) info.metric = 0;
            else if (v4 == 0) info.metric = v6;
            else if (v6 == 0) info.metric = v4;
            else info.metric = (v4 < v6) ? v4 : v6;

            interfaces.push_back(info);
        }

        free(adapters);
        if (wsaStarted) WSACleanup();
#else
        // Linux/Unix implementation using getifaddrs

        struct ifaddrs* ifaddr, * ifa;
        if (getifaddrs(&ifaddr) == -1) return interfaces;
        int sock = socket(AF_INET, SOCK_DGRAM, 0);

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;
            auto it = std::find_if(interfaces.begin(), interfaces.end(), [&](auto& i) { return i.name == ifa->ifa_name; });
            if (it == interfaces.end()) {
                network_interface_info ni{}; ni.name = ifa->ifa_name;
                ni.is_up = (ifa->ifa_flags & IFF_UP); ni.is_loopback = (ifa->ifa_flags & IFF_LOOPBACK);
                struct ifreq ifr; std::memset(&ifr, 0, sizeof(ifr));
                std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);

                // get MAC address 
                if (sock >= 0 && ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                    char mac_str[18];
                    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    ni.mac_address = mac_str;
                }

                if (sock >= 0 && ioctl(sock, SIOCGIFMETRIC, &ifr) == 0) ni.metric = ifr.ifr_metric;
                interfaces.push_back(ni); it = interfaces.end() - 1;
            }
            char buf[INET6_ADDRSTRLEN] = { 0 };
            if (ifa->ifa_addr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, buf, INET_ADDRSTRLEN);
                it->ipv4_addresses.push_back(buf);
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6) {
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr, buf, INET6_ADDRSTRLEN);
                it->ipv6_addresses.push_back(buf);
            }
        }
        if (sock >= 0) close(sock);
        freeifaddrs(ifaddr);
#endif

        std::sort(interfaces.begin(), interfaces.end(), [](const auto& a, const auto& b) {
            // 1. loopback always last
            if (a.is_loopback != b.is_loopback) return !a.is_loopback && b.is_loopback;
            // 2. UP first
            if (a.is_up != b.is_up) return a.is_up > b.is_up;
            // 3. lower metric first
            return a.metric < b.metric;
            });
        return interfaces;
    }


} 
