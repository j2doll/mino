#include <iostream>
#include <thread>
#include <chrono>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/ws/ws.hpp"

#ifdef USE_CURL

class custom_event_listener : public mino::network::ws::ws_event_listener {
public:
    void on_connected() override {
        namespace dtutil = mino::core::datetime::util;
        auto current_local = dtutil::current_time_string();
        std::cout
            << current_local
            << " [Class Listener] Connected!" << std::endl;
    }
    void on_message(const std::string& message, mino::network::ws::ws_frame_type) override {
        namespace dtutil = mino::core::datetime::util;
        auto current_local = dtutil::current_time_string();
        std::cout
            << current_local
            << " [Class Listener] Message: " << message << std::endl;
    }
    void on_error(const std::string& error_message) override {
        namespace dtutil = mino::core::datetime::util;
        auto current_local = dtutil::current_time_string();
        std::cerr
            << current_local
            << " [Class Listener] Error: " << error_message << std::endl;
    }
    void on_disconnected(int status_code, const std::string& reason) override {
        namespace dtutil = mino::core::datetime::util;
        auto current_local = dtutil::current_time_string();
        std::cout
            << current_local
            << " [Class Listener] Closed (" << status_code << "): " << reason << std::endl;
    }
};

// ws 테스트를 하려면, python ws_server.py 를 먼저 실행.
// wss 테스트를 하려면, python wss_server.py 를 먼저 실행.
//  wss는 공개 인증서 파일(cert.pem), 개인 키 파일(key.pem) 파일이 필요하며,
//  wss_server.py 에서 경로를 지정해야 함.
int main() {
    mino::network::sock mnsock;

    namespace mnws = mino::network::ws;
    using ws_client = mnws::ws_client;
    using wss_client = mnws::wss_client;
    using ws_connection_config = mnws::ws_connection_config;
    using ws_callbacks = mnws::ws_callbacks;

    bool testWss = false;  // wss 연동 테스트 여부
    bool testWs  = true;   // ws 연동 테스트 여부

    // 1. wss_client + 클래스 리스너 사용 예시
    if (testWss)
    {
        wss_client client;
        client.set_url("wss://127.0.0.1:8766");

        ws_connection_config conn_cfg;
        conn_cfg.connect_timeout_sec = 5;
        client.configure_connection(conn_cfg);
        client.configure_ssl(true, true);

        custom_event_listener listener;
        client.set_listener(&listener);

        if (client.connect()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            client.send_text("Hello from mino::network::ws namespace!");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            client.disconnect();
        }
    }

    // 2. ws_client + 람다 함수 콜백 사용 예시
    if (testWs)
    {
        ws_client client;
        client.set_url("ws://127.0.0.1:8765");

        ws_callbacks callbacks;
        callbacks.on_connected = []() {
            namespace dtutil = mino::core::datetime::util;
            auto current_local = dtutil::current_time_string();
            std::cout
                << current_local  << " [Callback] Connected!" << std::endl;
        };
        callbacks.on_message = [](const std::string& msg, mino::network::ws::ws_frame_type) {
            namespace dtutil = mino::core::datetime::util;
            auto current_local = dtutil::current_time_string();
            std::cout
                << current_local  << " [Callback] Received: " << msg << std::endl;
        };
        callbacks.on_error = [](const std::string& err) {
            namespace dtutil = mino::core::datetime::util;
            auto current_local = dtutil::current_time_string();
            std::cerr
                << current_local  << " [Callback] Error: " << err << std::endl;
        };
        callbacks.on_disconnected = [](int code, const std::string& reason) {
            namespace dtutil = mino::core::datetime::util;
            auto current_local = dtutil::current_time_string();
            std::cout
                << current_local  << " [Callback] Disconnected: " << reason << std::endl;
        };

        client.set_callbacks(callbacks);

        if (client.connect()) {
            namespace dtutil = mino::core::datetime::util;
            auto current_local = dtutil::current_time_string();

            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::string msg = "Hello via Plain WS!";
            client.send_text(msg);
            std::cout << current_local  << " send_text() called: '" << msg << "'" << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(2));
            client.disconnect();
        }
    }

    return 0;
}

#endif // USE_CURL
