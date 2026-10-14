#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include "mino/network/network.hpp"
#include "mino/core/container/container.hpp"
#include "mino/core/string/string.hpp"

int main() {
    namespace mcc = mino::core::container;
    namespace mcsp = mino::core::string::print;
    namespace mnu = mino::network::udp;

    using byte_buffer = mcc::byte_buffer;
    using concurrent_byte_buffer = mcc::concurrent_byte_buffer;

    using udp_receiver = mnu::udp_receiver;
    using ip_version_t = mnu::udp_receiver::ip_version_t;

    mino::network::sock mnsock; // 소켓 초기화

    mcsp::println("=== UDP 수신 버퍼 및 파싱 스레드 ===");

    // udp 수신자 생성
    auto receiver = std::make_shared<udp_receiver>(udp_receiver::ip_version_t::ipv4);

    concurrent_byte_buffer rx_buffer; // 수신 버퍼 
    std::atomic<bool> parsing_running{ true };

    // [수신 콜백 설정] 수신된 데이터를 rx_buffer에 저장
    receiver->set_on_receive_callback([&rx_buffer](const std::string& data, const std::string& ip, uint16_t port) {
        std::uint8_t ip_len = static_cast<std::uint8_t>(ip.size()); // 송신자 IP 길이
        std::uint16_t c_port = port; // 송신자 PORT 번호
        std::uint16_t data_len = static_cast<std::uint16_t>(data.size()); // 데이터 길이

        // NOTE: 필드를 하나씩 적지 않고, 구조체를 정의하여 수신 버퍼에 넘기는 방법도 있음. 
        rx_buffer.with_lock([&](mino::core::container::byte_buffer& buf) {
                // 수신 버퍼에 [IP 길이(1)][IP(가변)][포트(2)][데이터 길이(2)][데이터(가변)] 순서로 저장함.
                buf.write(&ip_len, 1); // IP 길이 (1바이트)
                buf.write(ip.data(), ip_len); // IP 문자열 (ip_len 바이트)
                buf.write(&c_port, sizeof(c_port)); // 포트 번호 (2바이트)
                buf.write(&data_len, sizeof(data_len)); // 데이터 길이 (2바이트)
                buf.write(data.data(), data_len); // 데이터 (데이터 길이 바이트)
            });
        });

    // UDP 수신자 바인드 및 대기 시작
    auto receiver_ip = "0.0.0.0";
    unsigned short test_port = 12345;
    if (!receiver->start_unicast(receiver_ip, test_port)) {
        mcsp::eprintln("UDP 수신자 바인드 실패 (포트: {})", test_port);
        return 1;
    }
    mcsp::println("UDP 수신자가 {}:{} 에서 대기 중...", receiver_ip, test_port);

    // [수신 버퍼 파싱 스레드] 
    std::thread parser_thread([&rx_buffer, &parsing_running]() {
        while (parsing_running) {
            rx_buffer.with_lock([](mino::core::container::byte_buffer& buf) {
                while (buf.readable_bytes() >= 6) {
                    // 최소 헤더 크기: [IP 길이(1)][IP(가변)][포트(2)][데이터 길이(2)] = 최소 6바이트
                    // 
                    // 수신 버퍼는 [IP 길이(1)][IP(가변)][포트(2)][데이터 길이(2)][데이터(가변)] 순서로 저장되어 있음.

                    std::uint8_t ip_len = 0;
                    buf.peek(&ip_len, 1);

                    // 헤더 전체 크기 계산: [IP 길이(1)][IP(가변)][포트(2)][데이터 길이(2)]
                    std::size_t header_size = 1 + ip_len + sizeof(std::uint16_t) + sizeof(std::uint16_t);
                    if (buf.readable_bytes() < header_size) {
                        break;
                    }

                    std::vector<std::uint8_t> header_peek(header_size);
                    buf.peek(header_peek.data(), header_size); // 헤더 전체를 미리 읽음

                    // 데이터 길이 추출
                    std::uint16_t data_len = 0;
                    std::size_t data_len_offset = 1 + ip_len + sizeof(std::uint16_t);
                    std::memcpy(&data_len, header_peek.data() + data_len_offset, sizeof(data_len));

                    // 전체 패킷 크기 계산: 헤더 + 데이터
                    std::size_t total_packet_size = header_size + data_len;
                    if (buf.readable_bytes() < total_packet_size) {
                        break; // 페이로드가 다 안 모였거나, 패킷에 문제가 있음.
                    }

                    std::vector<std::uint8_t> full_packet(total_packet_size);
                    buf.read(full_packet.data(), total_packet_size); // 전체 패킷 읽기

                    std::size_t offset = 0;
                    std::uint8_t actual_ip_len = full_packet[offset]; // 실제 IP 길이
                    offset += 1; // IP 길이 1바이트 읽음

                    std::string client_ip(reinterpret_cast<char*>(full_packet.data() + offset), actual_ip_len); // 실제 IP 문자열
                    offset += actual_ip_len; // IP 문자열 크기만큼 읽음

                    std::uint16_t client_port = 0;
                    std::memcpy(&client_port, full_packet.data() + offset, sizeof(client_port)); // 실제 포트 번호
                    offset += sizeof(client_port); // 포트 번호 2바이트 읽음

                    std::uint16_t actual_data_len = 0;
                    std::memcpy(&actual_data_len, full_packet.data() + offset, sizeof(actual_data_len)); // 실제 데이터 길이
                    offset += sizeof(actual_data_len); // 데이터 길이 2바이트 읽음

                    std::string payload(reinterpret_cast<char*>(full_packet.data() + offset), actual_data_len); // 실제 데이터인 문자열 생성

                    mcsp::println(
                        "[파싱 스레드] 수신 완료 [송신자: {}:{}, 길이: {}] 내용: '{}'",
                        client_ip, client_port, actual_data_len, payload);
                }
            });

            // 10ms 주기 폴링 슬립
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        });

     int seq = 1;
    while (true) {
        {
            using namespace std::this_thread;
            using namespace std::chrono;
            sleep_for(5s);
        }
        mcsp::println("수신 대기 중: {}", seq++);
    }

    // 종료 정리
    parsing_running = false;
    if (parser_thread.joinable())
        parser_thread.join();

    receiver->quit(); // UDP 수신자 종료

    mcsp::println("=== UDP 테스트 완료 ===");
    return 0;
}
