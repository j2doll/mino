#include <memory>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <atomic>
#include <cassert>

#include "mino/network/tcp/tcp_client.hpp"
#include "mino/core/container/byte_buffer.hpp"
#include "mino/core/string/print.hpp"

namespace mcsp = mino::core::string::print;

int main() {
    namespace mcc = mino::core::container;
    namespace mcsp = mino::core::string::print;
    namespace mnt = mino::network::tcp;

    using byte_buffer = mcc::byte_buffer;
    using concurrent_byte_buffer = mcc::concurrent_byte_buffer;

    using tcp_client = mnt::tcp_client;

    mino::network::sock mnsock; // 소켓 초기화

    mcsp::println("=== TCP 클라이언트 (폴링 및 슬립 파싱 스레드) ===");

    auto client = std::make_shared<tcp_client>(); // tcp 클라이언트 생성
    assert(client != nullptr);

    client->set_server("127.0.0.1", 29000); // tcp 서버 IP와 포트 설정

    concurrent_byte_buffer rx_buffer; // 수신 버퍼
    std::atomic<bool> parsing_running{ true };

    // [수신 버퍼 파싱 스레드] 폴링 방식으로 응답 스트림 파싱 및 슬립
    std::thread parser_thread(
        [&rx_buffer, &parsing_running]() {
            while (parsing_running) {
                rx_buffer.with_lock(
                    [](mino::core::container::byte_buffer& buf) {
                        //---------------------------------------
                        // 수신 버퍼 파싱 {{

                        while (buf.readable_bytes() >= 4) {
                            struct Header {
                                std::uint16_t id;
                                std::uint16_t length;
                            } header{};

                            buf.peek(&header, sizeof(header));
                            std::size_t total_size = sizeof(Header) + header.length;

                            if (buf.readable_bytes() < total_size) {
                                break;
                            }

                            buf.read(&header, sizeof(header));
                            std::string body(header.length, '\0');
                            buf.read(&body[0], header.length);

                            mcsp::println(
                                "[클라이언트 파싱 스레드] 응답 수신 "
                                "[ID: {}, 길이: {}] 내용: '{}'",
                                header.id, header.length, body);

                        } // while (buf.readable_bytes() >= 4)

                        // 수신 버퍼 파싱 }}
                        //---------------------------------------
                    } // [](mino::core::container::byte_buffer& buf) {
                ); // rx_buffer.with_lock( 

                { // 잠깐 슬립
                    using namespace std::this_thread;
                    using namespace std::chrono;
                    sleep_for(10ms);
                }

            } // while (parsing_running)
        } // [&rx_buffer, &parsing_running]() {
    ); // std::thread parser_thread([&rx_buffer, &parsing_running]() { 

    // 연결 종료 시: 버퍼 리셋
    client->set_on_close([&rx_buffer]() {
        rx_buffer.reset(4096); // 수신 버퍼 초기화
        mcsp::println("[클라이언트] 서버 연결 종료, 버퍼 리셋 완료");
    });

    // 연결 성공 시: 로그 출력
    client->set_on_connect([]() {
        mcsp::println("[클라이언트] 서버 연결 성공!");
    });

    // 데이터 수신 시: 버퍼에 누적
    client->set_on_receive([&rx_buffer](const std::string& data) {
        rx_buffer.write(data.data(), data.size());
    });

    using reconnect_config = mnt::reconnect_config;

    reconnect_config config;
    config.backoff_multiplier = 2.0; // 지수 증가 배수 2배
    config.initial_interval = std::chrono::milliseconds(2000); // 최초 재연결 대기 시간 2초
    config.max_interval = std::chrono::milliseconds(60000); // 최대 재연결 대기 시간 60초
    config.max_retries = 0; // 0: 무제한 재연결 시도
    // 2s -> 4s -> 8s -> 16s -> 32s -> 60s -> 60s -> 60s ...

    client->start(config); // tcp 클라이언트 시작 (재연결 설정 적용)

    // 패킷 전송 헬퍼 람다 (테스트 용도)
    auto send_packet = [&client](std::uint16_t id, const std::string& body) {
        struct Header {
            std::uint16_t id;
            std::uint16_t length;
        } header{};

        std::uint16_t length = static_cast<std::uint16_t>(body.size());
        std::vector<char> packet(4 + length);

        Header hdr;
        hdr.id = id;
        hdr.length = length;

        std::memcpy(packet.data(), &hdr, sizeof(hdr)); // 헤더 복사
        std::memcpy(packet.data() + sizeof(hdr), body.data(), length); // 바디 복사

        client->send_data(std::string(packet.data(), packet.size())); // tcp 서버로 패킷 전송
    };

    int seq = 1;
    while (true) {
        { 
            using namespace std::this_thread;
            using namespace std::chrono;
            sleep_for(3s);
        }

        if (client->is_connected()) {
            std::string msg = "Hello from Client #" + std::to_string(seq++);
            mcsp::println("[클라이언트] 전송: '{}'", msg);
            send_packet(1001, msg); // tcp 서버로 메시지 송신 
        }
        else {
            mcsp::println("[클라이언트] 서버 미연결 ...");
        }
    }

    parsing_running = false;
    if (parser_thread.joinable()) {
        parser_thread.join();
    }
    client->stop();

    return 0;
}
