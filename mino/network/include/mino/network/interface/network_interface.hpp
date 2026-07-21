#pragma once

#include <string>
#include <vector>
#include <cstdint>
 
namespace mino::network::iface { 

    struct interface_stats {
        uint64_t rx_bytes = 0; // 누적 수신 바이트
        uint64_t tx_bytes = 0; // 누적 전송 바이트
    };

    struct interface_info {
        std::string name; // 인터페이스 이름 (예: "eth0", "Wi-Fi")
        std::string ip_address; // 인터페이스에 할당된 IP 주소      
        std::string mac_address; // 인터페이스의 MAC 주소
        interface_stats stats; // 인터페이스 통계 정보
        bool is_running = false; // 인터페이스가 현재 활성화되어 있는지 여부 (true: 활성화, false: 비활성화)
    };

    class interface_manager {
    public:
        static std::vector<interface_info> get_interfaces(); // 일반 권한으로 시스템의 모든 네트워크 인터페이스 정보 및 통계를 가져옵니다.

    };

} 
