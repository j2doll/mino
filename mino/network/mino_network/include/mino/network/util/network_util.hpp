#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mino::network::util {

    struct  network_interface_info {
        std::string name;
        std::string description;
        std::string mac_address;

        std::vector<std::string> ipv4_addresses;
        std::vector<std::string> ipv6_addresses;
        std::vector<std::string> dns_servers; // 인덱스 0번이 최우선 순위
        std::string netmask;

        bool is_up;
        bool is_loopback;
        bool is_dhcp_enabled;

        uint32_t metric;
    };

    // 인터페이스 관련 기능
    std::vector<network_interface_info> get_interfaces_sorted_by_priority();

    // IP 유틸리티 기능
    bool is_valid_ipv4(const std::string& ip_address);
    bool is_valid_ipv6(const std::string& ip_address);
    bool is_multicast_ipv4(const std::string& ip_address);

} 

