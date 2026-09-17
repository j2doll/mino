#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "mino/core/string/string.hpp"
#include "mino/core/string/print.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/redis/redis.hpp"

namespace mcs = mino::core::string;
namespace mcsp = mino::core::string::print;
auto tce = mcs::to_console_encoding;

bool test_tcp_ipv4();
bool test_tcp_ipv6();
bool test_tls_client();

// main() 구동 전에 redis-server가 실행 중이어야 함.
int main() {
    namespace mn = mino::network;
    using mnsock = mn::sock;

    mnsock msck;

    test_tcp_ipv4();
    // test_tcp_ipv6();
    // test_tls_client();

    return 0;
}

// -----------------------------------------------------------------------------
// 1. TCP IPv4 클라이언트 테스트
// -----------------------------------------------------------------------------
bool test_tcp_ipv4() {
    mcsp::println("\n========================================");
    mcsp::println(" [1] TCP IPv4 Redis 테스트 (127.0.0.1:6379)");
    mcsp::println("========================================");

    mino::network::redis::redis_tcp_client client;
    if (!client.connect("127.0.0.1", 6379, AF_INET)) { // [1] TCP IPv4 연결 시도
        mcsp::eprintln("TCP IPv4 연결 실패!");
        return false;
    }
    mcsp::println("TCP IPv4 연결 성공!");

    try {
        std::string ret_ping = client.ping("hello_ipv4"); // [2] PING 명령 전송 및 응답 확인
        mcsp::println("PING: {}", ret_ping);

        client.set("key:ipv4", "data_4"); // [3] SET 명령 전송
        mcsp::println("SET key:ipv4 -> OK");

        auto val = client.get("key:ipv4"); // [4] GET 명령 전송 및 결과 확인
        mcsp::println("GET key:ipv4 -> {}", val.value_or("null"));
    }
    catch (const std::exception& e) {
        mcsp::eprintln("예외 발생: {}", e.what());
        client.disconnect();
        return false;
    }

    client.disconnect(); // [5] 연결 종료
    return true;
}

// -----------------------------------------------------------------------------
// 2. TCP IPv6 클라이언트 테스트
// -----------------------------------------------------------------------------
bool test_tcp_ipv6() {
    mcsp::println("\n========================================");
    mcsp::println(" [2] TCP IPv6 Redis 테스트 (::1:6379)");
    mcsp::println("========================================");

    mino::network::redis::redis_tcp_client client;
    if (!client.connect("::1", 6379, AF_INET6)) {
        mcsp::eprintln("TCP IPv6 연결 실패! (서버 바인딩 확인 필요)");
        return false;
    }
    mcsp::println("TCP IPv6 연결 성공!");

    try {
        mcsp::println("PING: {}", client.ping("hello_ipv6"));

        client.set("key:ipv6", "data_6");
        auto val = client.get("key:ipv6");

        mcsp::println("GET key:ipv6 -> {}", val.value_or("null"));
    }
    catch (const std::exception& e) {
        mcsp::eprintln("예외 발생: {}", e.what());
        client.disconnect();
        return false;
    }

    client.disconnect();
    return true;
}

// -----------------------------------------------------------------------------
// 3. TLS 보안 클라이언트 테스트 (기본 포트 6380)
// -----------------------------------------------------------------------------
bool test_tls_client() {
    mcsp::println("\n========================================");
    mcsp::println(" [3] TLS 보안 Redis 테스트 (127.0.0.1:6380)");
    mcsp::println("========================================");

    mino::network::redis::redis_tls_client client;

    mino::network::redis::redis_tls_config tls_config;
    tls_config.verify_peer = true;
    tls_config.ca_cert_file = "certs/ca.crt";
    tls_config.sni_hostname = "localhost";
    client.configure_tls(tls_config);

    if (!client.connect("127.0.0.1", 6380, AF_INET)) {
        mcsp::eprintln("TLS 연결 실패! (인증서 설정 및 포트 6380 확인 필요)");
        return false;
    }
    mcsp::println("TLS 핸드셰이크 및 보안 세션 연결 성공!");

    try {
        // client.auth("your_password");

        mcsp::println("TLS PING: {}", client.ping("hello_tls"));
        client.set("key:tls", "encrypted_session_token");
        auto val = client.get("key:tls");
        mcsp::println("GET key:tls -> {}", val.value_or("null"));

        auto hash_count = client.execute_command({ "HSET", "user:session", "user_id", "admin", "role", "root" });
        mcsp::println("HSET 결과 필드 수: {}", hash_count.as_integer().value_or(0));

    }
    catch (const std::exception& e) {
        mcsp::eprintln("예외 발생: {}", e.what());
        client.disconnect();
        return false;
    }

    client.disconnect();
    return true; 
}

// -----------------------------------------------------------------------------
// Redis 인증 방법
// -----------------------------------------------------------------------------
//////////////////////////////////////////////////
// (1) Redis 애플리케이션 레벨 인증
//---------------------------------------------
// (1-1) 단일 비밀번호 인증 (Redis 5 이하 또는 기본 default 계정):
// bool is_ok = client.auth("my_secret_password");
//---------------------------------------------
// (1-2) ACL 기반 사용자 계정 인증 (Redis 6 이상):
// bool is_ok = client.auth("service_user", "service_password");
//
//////////////////////////////////////////////////
// (2) TLS 레벨 상호 인증 (mTLS, 클라이언트 인증서)
// mino::network::redis::redis_tls_config tls_config;
// tls_config.verify_peer = true;
// tls_config.ca_cert_file = "certs/ca.crt";
// tls_config.sni_hostname = "redis.example.com";
// 
// // 클라이언트 인증서/키 설정 (mTLS)
// tls_config.client_cert_file = "certs/client.crt";
// tls_config.client_key_file = "certs/client.key";
// 
// client.configure_tls(tls_config);
// 
//////////////////////////////////////////////////
// 인증 전에는 서버 버전을 미리 조회할 수 없으므로
// 다음과 같이 시도를 한다.  
//
// bool auth(const std::string& password, const std::string& username = "") {
//     // 1. 유저명이 비어있거나 "default"인 경우: 전 버전 공통인 단일 인자 AUTH 전송
//     if (username.empty() || username == "default") {
//         auto res = execute_command({"AUTH", password});
//         auto str_val = res.as_string();
//         return str_val.has_value() && *str_val == "OK";
//     }
// 
//     // 2. 특정 유저명이 명시된 경우: Redis 6+ ACL 커맨드 전송
//     auto res = execute_command({"AUTH", username, password});
//     if (res.is_error()) {
//         auto err_msg = res.as_string().value_or("");
//         
//         // Redis 5 이하 서버는 AUTH 인자 2개를 인식하지 못함
//         if (err_msg.find("wrong number of arguments") != std::string::npos) {
//             throw std::runtime_error(
//                 "Authentication failed: Target Redis server does not support ACL (Redis < 6.0)."
//             );
//         }
//         return false; // 비밀번호 불일치 (-WRONGPASS 등)
//     }
// 
//     auto str_val = res.as_string();
//     return str_val.has_value() && *str_val == "OK";
// }
//////////////////////////////////////////////////


