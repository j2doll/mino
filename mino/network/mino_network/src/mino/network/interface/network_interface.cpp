#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>

#include "mino/network/network.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "mino/network/interface/network_interface.hpp"

namespace mino {
    namespace network {
        namespace iface {

#ifdef _WIN32
            std::vector<mino::network::iface::interface_info> interface_manager::get_interfaces() {
                std::vector<interface_info> result;
                ULONG outBufLen = 15000;
                PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

                if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen) == NO_ERROR) {
                    for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next) {
                        interface_info info;
                        info.name = pCurr->AdapterName;
                        info.is_running = (pCurr->OperStatus == IfOperStatusUp);

                        // IPv4 추출
                        if (pCurr->FirstUnicastAddress) {
                            char buf[INET_ADDRSTRLEN];
                            sockaddr_in* sa = (sockaddr_in*)pCurr->FirstUnicastAddress->Address.lpSockaddr;
                            inet_ntop(AF_INET, &(sa->sin_addr), buf, INET_ADDRSTRLEN);
                            info.ip_address = buf;
                        }

                        // MAC 추출
                        char mac[20];
                        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                            pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1], pCurr->PhysicalAddress[2],
                            pCurr->PhysicalAddress[3], pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
                        info.mac_address = mac;

                        // 통계 (관리자 권한 불필요)
                        MIB_IF_ROW2 ifRow;
                        memset(&ifRow, 0, sizeof(ifRow));
                        ifRow.InterfaceIndex = pCurr->IfIndex;
                        if (GetIfEntry2(&ifRow) == NO_ERROR) {
                            info.stats.rx_bytes = ifRow.InOctets;
                            info.stats.tx_bytes = ifRow.OutOctets;
                        }

                        result.push_back(info);
                    }
                }
                free(pAddresses);
                return result;
            }
#else
            std::vector<interface_info> interface_manager::get_interfaces() {
                std::vector<interface_info> result;
                struct ifaddrs* ifaddr, * ifa;

                if (getifaddrs(&ifaddr) == -1) return result;

                for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
                    if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) continue;

                    interface_info info;
                    info.name = ifa->ifa_name;
                    info.is_running = (ifa->ifa_flags & IFF_UP);

                    // IPv4 주소 추출
                    char host[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, host, INET_ADDRSTRLEN);
                    info.ip_address = host;

                    // MAC 주소 추출 (/sys/class/net/<interface>/address 읽기)
                    std::ifstream mac_file("/sys/class/net/" + info.name + "/address");
                    if (mac_file.is_open()) {
                        std::getline(mac_file, info.mac_address);
                    }

                    // Linux 통계 (/proc/net/dev 파싱)
                    std::ifstream file("/proc/net/dev");
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find(info.name) != std::string::npos) {
                            size_t colon_pos = line.find(':');
                            if (colon_pos != std::string::npos) {
                                std::stringstream ss(line.substr(colon_pos + 1));
                                uint64_t dummy;
                                ss >> info.stats.rx_bytes; // 1번째: 수신
                                for (int i = 0; i < 7; ++i) ss >> dummy;
                                ss >> info.stats.tx_bytes; // 9번째: 전송
                            }
                            break;
                        }
                    }
                    result.push_back(info);
                }
                freeifaddrs(ifaddr);
                return result;
            }
#endif

        } // namespace iface
    } // namespace network
} // namespace mino
