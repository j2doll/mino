#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "mino/core/string/string.hpp"

// mino network interface
#include "mino/network/ethernet.hpp"
#include "mino/network/interface/network_interface.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

int main(int argc, char* argv[]) {
    auto ret_sock = mino::network::init_socket();
    if (ret_sock.has_value()) {
        eprint(tce("[오류] 소켓 초기화 실패: " + ret_sock.value()));
        return 1;
    }

    namespace mni = mino::network::iface;
    using interface_info = mni::interface_info;
    using interface_stats = mni::interface_stats; 

    print(tce("================ [PUBLIC MEMBER TEST] ================"));

    // 1. interface_manager::get_interfaces() 정적 메서드 호출 테스트
    std::vector<interface_info> interfaces = mni::interface_manager::get_interfaces(); 

    if (interfaces.empty()) {
        eprint(tce("[-] 인터페이스 목록이 비어 있습니다."));
    }
    else {
        print(tce("[+] 총 "), interfaces.size(), tce("개의 네트워크 인터페이스를 발견했습니다.\n"));
    }

    // 2. interface_info 및 interface_stats의 모든 public 멤버 접근 테스트
    for (size_t i = 0; i < interfaces.size(); ++i) {
        const interface_info& iface = interfaces[i];

        print(tce("--- [Interface #"), i + 1, tce("] ---"));

        // 인터페이스 이름
        // interface_info::name (std::string)
        print(tce("  1. name          : "), iface.name);

        // 아이피 주소
        // interface_info::ip_address (std::string)
        print(tce("  2. ip_address    : "), (iface.ip_address.empty() ? "(empty)" : iface.ip_address));

        // 맥 주소
        // interface_info::mac_address (std::string)
        print(tce("  3. mac_address   : "), (iface.mac_address.empty() ? "(empty)" : iface.mac_address));

        // 작동 상태
        // interface_info::is_running (bool)
        print(tce("  4. is_running    : "), (iface.is_running ? "true (UP)" : "false (DOWN)"));

        // 수신한 바이트 수
        // interface_info::stats (struct interface_stats)
        // interface_stats::rx_bytes (uint64_t)
        print(tce("  5. stats.rx_bytes: "), iface.stats.rx_bytes, tce(" bytes"));

        // 송신한 바이트 수
        // interface_stats::tx_bytes (uint64_t)
        print(tce("  6. stats.tx_bytes: "), iface.stats.tx_bytes, tce(" bytes\n"));
    }

    // 3. interface_stats 및 interface_info 구조체 기본 생성자 및 멤버 직접 쓰기/읽기 테스트
    print(tce("--- [Struct Direct Member Test] ---"));

    // 인터페이스 통계 구조체
    interface_stats dummy_stats;
    dummy_stats.rx_bytes = 1024; // 누적 수신 바이트
    dummy_stats.tx_bytes = 2048; // 누적 전송 바이트

    // 인터페이스 정보 구조체
    interface_info dummy_info;
    dummy_info.name = "test_eth0"; // 인터페이스 이름
    dummy_info.ip_address = "192.168.0.1"; // 인터페이스에 할당된 IP 주소
    dummy_info.mac_address = "00:11:22:33:44:55"; // 인터페이스의 MAC 주소
    dummy_info.is_running = true; // 인터페이스가 현재 활성화되어 있는지 여부
    dummy_info.stats = dummy_stats; // 인터페이스 통계 정보

    print(tce("  [Direct Assign] "), dummy_info.name,
        tce(" | IP: "), dummy_info.ip_address,
        tce(" | MAC: "), dummy_info.mac_address,
        tce(" | Running: "), (dummy_info.is_running ? "true" : "false"),
        tce(" | RX: "), dummy_info.stats.rx_bytes,
        tce(" | TX: "), dummy_info.stats.tx_bytes);

    print(tce("======================================================"));

    mino::network::close_socket();
    return 0;
}
