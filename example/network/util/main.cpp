#include <iostream>
#include <iomanip>

#include "mino/core/string/string.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/util/util.hpp"

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    namespace mnu = mino::network::util;

    std::cout << "=== Network Interface List (Sorted by Priority) ===" << std::endl;
    auto interfaces = mnu::get_interfaces_sorted_by_priority();

    for (const auto& info : interfaces) {
        std::cout << "\n[" << (info.is_up ? "UP" : "DOWN") << "] " << info.description << " (" << info.name << ")" << std::endl;

        if (info.is_up) {
            std::cout << "  - Status:  UP" << std::endl;
        }
        else {
            std::cout << "  - Status:  DOWN" << std::endl;
        }

        std::cout << "  - MAC:     " << info.mac_address << std::endl;

        std::cout << "  - Metric:  " << info.metric << std::endl;


        if (info.is_loopback) {
            std::cout << "  - Loopback: Yes" << std::endl;
        }

        for (const auto& ip : info.ipv4_addresses) {
            std::cout << "  - IPv4:    " << ip << (mnu::is_multicast_ipv4(ip) ? " [Multicast]" : "") << std::endl;
        }

        for (const auto& ip : info.ipv6_addresses) {
            std::cout << "  - IPv6:    " << ip << std::endl;
        }

        if (!info.dns_servers.empty()) {
            std::cout << "  - DNS:     " << info.dns_servers[0] << " (Primary)" << std::endl;
        }

        if (info.is_dhcp_enabled) {
            std::cout << "  - DHCP:    Enabled" << std::endl;
        }
        else {
            std::cout << "  - DHCP:    Disabled" << std::endl;
        }

        for (const auto& dns : info.dns_servers) {
            std::cout << "  - DNS:     " << dns << std::endl;
        }

        // Print netmask as a whole string (not per-character)
        if (!info.netmask.empty()) {
            std::cout << "  - Netmask: " << info.netmask << std::endl;
        }
    }

    // IP Validation Test
    std::cout << "\n=== IP Validation Test ===" << std::endl;
    std::string test_ip = "239.255.255.250";
    std::cout << test_ip << " is valid IPv4? " << std::boolalpha << mnu::is_valid_ipv4(test_ip) << std::endl;
    std::cout << test_ip << " is Multicast?  " << mnu::is_multicast_ipv4(test_ip) << std::endl;

    return 0;
}
