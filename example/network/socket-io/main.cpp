#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <string>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/json/json.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/socket-io/socketio_client.hpp"

namespace sio = mino::network::socket_io;

bool g_keep_running = true;
sio::socketio_client_base* g_active_client = nullptr;

void handle_signal(int signum) {
    namespace dtutil = mino::core::datetime::util;
    auto current_local = dtutil::current_time_string();

    std::cout << current_local << "\n[System] Caught signal ("
        << signum << "). Stopping loop...\n";
    g_keep_running = false;
    if (g_active_client) {
        g_active_client->close("Interrupted by user");
    }
}

// NOTICE: main 수행 전에 다음을 수행.
//  (1) install_sio.cmd 로 모듈 설치 (Windows)
//  (2) node server.js 로 서버 실행 (Node.js)
int main() {
    mino::network::sock mnsock;
    std::signal(SIGINT, handle_signal);

    namespace dtutil = mino::core::datetime::util;

    // =========================================================================
    // [버전 및 설정 변수]
    // =========================================================================
    const std::string target_namespace = "/";
    const int target_version = 2;
    //
    // const std::string target_namespace = "/";
    // const int target_version = 3;
    // 
    //const int target_version = 4;
    //// const std::string target_namespace = "/";
    // const std::string target_namespace = "/chat";

    // 룸 이름 (룸을 사용하지 않을 때는 "" 로 설정 가능)
    // const std::string target_room = "lobby";
    const std::string target_room = "";

    std::unique_ptr<sio::socketio_client_base> client;
    int port = 0;
    std::string version_name;
    std::string ping_event;
    std::string pong_event;

    // 각 버전별 설정값 하드코딩
    if (target_version == 2) {
        client = sio::create_socketio_client(sio::socketio_version::v2);
        port = 52000;
        version_name = "v2";
        ping_event = "ping_v2";
        pong_event = "pong_v2";
    }
    else if (target_version == 3) {
        client = sio::create_socketio_client(sio::socketio_version::v4);
        port = 53000;
        version_name = "v3";
        ping_event = "ping_v3";
        pong_event = "pong_v3";
    }
    else { // 4
        client = sio::create_socketio_client(sio::socketio_version::v4);
        port = 54000;
        version_name = "v4";
        ping_event = "ping_v4";
        pong_event = "pong_v4";
    }

    g_active_client = client.get();

    std::cout << dtutil::current_time_string() << " [App] Configured Target: "
        << version_name << " (Port: " << port << "), NS: " << target_namespace << "\n";

    client->set_server_endpoint("127.0.0.1", port);
    client->set_connect_timeout_seconds(5);

    // 1. 네임스페이스 설정
    client->set_namespace(target_namespace);

    // 2. 세션 연결 성공 시 핸들러
    client->on_open([&client, version_name, ping_event, target_room]() {
        namespace dtutil = mino::core::datetime::util;
        std::cout << dtutil::current_time_string()
            << " [App] Socket.IO (" << version_name << ") session connected.\n";

        // 기본 Ping 송신
        std::string msg = "{\"msg\":\"Hello from C++ " + version_name + " client!\"}";
        client->emit(ping_event, msg);

        // target_room이 설정되어 있는 경우에만 Room 입장 요청
        if (!target_room.empty()) {
            std::cout << dtutil::current_time_string()
                << " [App] Joining room: " << target_room << "\n";
            client->join_room(target_room);
        }
        });

    // 3. Pong 응답 수신
    client->on(pong_event, [pong_event](const std::string& data) {
        namespace dtutil = mino::core::datetime::util;
        std::cout
            << dtutil::current_time_string()
            << " [App Specific Event: " << pong_event << "] Received: " << data << "\n";

        mino::core::json::parser json_parser;
        auto parsed_json = json_parser.parse(data);
        if (!data.empty() && !parsed_json.is_null()) {
            auto ret_json = mino::core::json::serializer::serialize(parsed_json, 4);
            std::cout << ret_json << std::endl;
        }
        });

    // 4. Room 브로드캐스트 이벤트 수신
    client->on("room_broadcast", [](const std::string& data) {
        namespace dtutil = mino::core::datetime::util;
        std::cout << dtutil::current_time_string()
            << " [App Room Broadcast] Received: " << data << "\n";
        });

    // 5. 모든 이벤트 메시지 수신 모니터링
    client->on([](const std::string& payload) {
        namespace dtutil = mino::core::datetime::util;
        std::cout << dtutil::current_time_string() << " [App All Events] " << payload << "\n";
        });

    client->on_close([](const std::string& reason) {
        namespace dtutil = mino::core::datetime::util;
        std::cout << dtutil::current_time_string() << " [App] Connection lost: " << reason << "\n";
        });

    client->on_error([](const std::string& err) {
        namespace dtutil = mino::core::datetime::util;
        std::cerr << dtutil::current_time_string() << " [App Error] " << err << "\n";
        });

    const int reconnect_interval_seconds = 3;

    // 6. 연결 및 재연결 제어 루프
    while (g_keep_running) {
        auto current_local = dtutil::current_time_string();
        std::cout << "\n" << current_local << " [App] Attempting to connect to "
            << version_name << " server (Port " << port << ")...\n";

        client->reset();

        if (client->connect()) {
            current_local = dtutil::current_time_string();
            std::cout << current_local << " [App] Connected successfully. Entering event loop...\n";

            client->run_event_loop();
        }
        else {
            current_local = dtutil::current_time_string();
            std::cerr << current_local << " [App] Connection attempt failed.\n";
        }

        if (!g_keep_running) break;

        current_local = dtutil::current_time_string();
        std::cout << current_local << " [App] Waiting "
            << reconnect_interval_seconds << " seconds before reconnecting...\n";

        int waited_ms = 0;
        int target_ms = reconnect_interval_seconds * 1000;
        while (g_keep_running && waited_ms < target_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            waited_ms += 100;
        }
    }

    auto current_local = dtutil::current_time_string();
    std::cout << current_local << " [App] Program exited safely.\n";
    g_active_client = nullptr;
    return 0;
}
