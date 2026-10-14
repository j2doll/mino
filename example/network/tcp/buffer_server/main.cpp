#include <memory>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <atomic>
#include <cassert>

#include "mino/core/container/container.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/log/log.hpp" 

#include "mino/network/network.hpp"

int main(int argc, char* argv[]) {
    namespace mcc = mino::core::container;
    namespace mcsp = mino::core::string::print;
    namespace mnt = mino::network::tcp;

    using byte_buffer = mcc::byte_buffer;
    using concurrent_byte_buffer = mcc::concurrent_byte_buffer;

    using tcp_server = mnt::tcp_server;
    using start_result = mnt::tcp_server::start_result;

    mino::network::sock mnsock; // 소켓 초기화

    mcsp::println("=== TCP 서버 (폴링 및 슬립 파싱 스레드) ===");

    auto server = std::make_shared<tcp_server>(); // tcp 서버 생성
    assert(server != nullptr);

    concurrent_byte_buffer rx_buffer; // 수신 버퍼
    std::atomic<bool> parsing_running{ true }; // 파싱 스레드 실행 여부

    // [파싱 스레드] 폴링 방식으로 버퍼를 확인하고 매번 슬립
    std::thread parser_thread(
        [server, &rx_buffer, &parsing_running]() {
            while (parsing_running) {
                rx_buffer.with_lock(
                    [server](byte_buffer& buf) {
                        //---------------------------------------
                        // 수신 버퍼 파싱 {{

                        while (buf.readable_bytes() >= 4) { // 최소 헤더 크기(4바이트)보다 커야 파싱 가능
                            struct Header {
                                std::uint16_t id;
                                std::uint16_t length;
                            } header{};

                            buf.peek(&header, sizeof(header)); // peek()은 읽기 포인터를 이동하지 않음

                            std::size_t total_size = sizeof(Header) + header.length; // 구조체 크기 계산

                            if (buf.readable_bytes() < total_size) { // 잘못된 패킷 길이 또는 미완성 패킷 처리
                                break; // 패킷 미완성 시 대기
                            }

                            buf.read(&header, sizeof(header)); // 헤더 읽기 (실제로 읽기 포인터 이동)

                            std::string body(header.length, '\0');
                            buf.read(&body[0], header.length); // 바디 읽기

                            mcsp::println(
                                "[서버 파싱 스레드] 패킷 처리 완료 "
                                "[ID: {}, 길이: {}] 내용: '{}'",
                                header.id, header.length, body);

                        } // while (buf.readable_bytes() >= 4)

                        // 수신 버퍼 파싱 }}
                        //---------------------------------------
                    } // [server](mino::core::container::byte_buffer& buf) { 
                ); // rx_buffer.with_lock 
                
                { // 잠깐 슬립
                    using namespace std::this_thread;
                    using namespace std::chrono;
                    sleep_for(10ms);
                }
            } // while (parsing_running) {
        } // [server, &rx_buffer, &parsing_running]() {
    ); // std::thread parser_thread( 

    // 1. 연결 종료 시: 버퍼 리셋 수행
    server->set_on_close_callback([&rx_buffer](socket_t client_socket, const std::string& reason) {
        rx_buffer.reset(4096);
        mcsp::println("[서버] 클라이언트 소켓 {} 연결 해제, 버퍼 리셋", static_cast<int>(client_socket));
    });

    server->set_on_connect_callback([](socket_t client_socket, const std::string& msg) {
        mcsp::println("[서버] 새로운 클라이언트 소켓 {} 접속 성공", static_cast<int>(client_socket));
    });

    // 2. 데이터 수신 시: 버퍼에 바이트 누적
    server->set_on_receive_callback([&rx_buffer](socket_t client_socket, const std::string& data) {
        rx_buffer.write(data.data(), data.size());
    });

    unsigned short port = 29000;
    auto result = server->start("0.0.0.0", port); // tcp 서버 시작
    if (result != start_result::success) {
        mcsp::eprintln("TCP 서버 시작 실패");
        parsing_running = false;
        if (parser_thread.joinable())
            parser_thread.join();
        return 1;
    }

    mcsp::println("TCP 서버 구동 중: 포트: {} ...", port);

    while (true) {
        using namespace std::this_thread;
        using namespace std::chrono;
        sleep_for(1s);
    }

    parsing_running = false;
    if (parser_thread.joinable()) {
        parser_thread.join();
    }
    server->quit();

    return 0;
}
