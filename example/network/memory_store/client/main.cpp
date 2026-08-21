#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/external/log/spd/auto_color_sink.hpp"

// network memory_store client
#include "mino/network/ethernet.hpp"
#include "mino/network/memory_store/client.hpp"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    namespace mnm = mino::network::memory_store;
    using memory_store_client = mnm::memory_store_client;

    // 메모리 저장소 클라이언트 
    memory_store_client client;

    // 로거 생성 및 설정
    namespace mels = mino::external::log::spd;
    auto console_sink = std::make_shared<mels::auto_color_sink<std::mutex>>();
    std::vector<::spdlog::sink_ptr> sinks{ console_sink };
    auto client_logger
        = std::make_shared<::spdlog::logger>("ms_client_logger", sinks.begin(), sinks.end());
    // client_logger->set_level(::spdlog::level::info);
    client.set_logger(client_logger);

    // 메모리 저장소 서버 환경 설정
    auto server_ip = "127.0.0.1";
    auto server_port = 43322;
    client.set_server_env(server_ip, server_port);

    // 서버에게 요청 후, 응답을 대기하는 시간 (단위:초)    
    auto timeout_sec = std::chrono::seconds(5);
    client.set_timeout(timeout_sec);

    client_logger->info("==================================================");
    client_logger->info("  <light_green>Memory Store Client Full Command Test Pipeline</light_green>");
    client_logger->info("==================================================");

    client_logger->info("Connecting to server (127.0.0.1:9999)...");
    std::chrono::seconds tcp_timeout = std::chrono::seconds(5); // TCP 연결 시도 시, 서버가 응답할 때까지 기다리는 최대 시간
    if (!client.connect(tcp_timeout)) { // 서버로의 연결 시도 
        client_logger->error("Connection <orange>Failed.</orange> Make sure server instance is online.");
#ifdef _WIN32
        WSACleanup();
#endif
        return (-1);
    }
    client_logger->info("Network Connection established <gold>successfully.</gold>");

    try {
        //--------------------
        // 0. Get Server Latency Test
        //-----------------------
        client_logger->info("--- <light_green>[0] Testing Server Latency</light_green> ---");
        auto latency_ms = client.request_server_sleep_ms();
        if (latency_ms < 0) {
            client_logger->error("Failed to retrieve server latency.");
        }
        else {
            client_logger->info("Server latency: {} ms", latency_ms);
        }

        // --------------------------------------------------
        // 1. SET Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[1] Testing SET Commands</light_green> ---");

        auto app_module_key = "app_mode";
        // auto app_module_key = "app mode"; // with Space character
        // auto app_module_key = "한 글"; // non-Eglish key 
        auto app_module_value = "production";
        bool set_res1 = client.set(app_module_key, app_module_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", app_module_key, app_module_value, set_res1 ? "SUCCESS" : "FAILED");

        auto build_version_key = "build_version";
        auto build_version_value = "v1.4.2_C++17";
        bool set_res2 = client.set(build_version_key, build_version_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", build_version_key, build_version_value, set_res2 ? "SUCCESS" : "FAILED");

        auto welcome_msg_key = "welcome_msg";
        // auto welcome_msg_key = "welcome msg";
        // auto welcome_msg_key = "한 글";
        auto welcome_msg_value = "Hello World Memory Store 한글";
        bool set_res3 = client.set(welcome_msg_key, welcome_msg_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", welcome_msg_key, welcome_msg_value, set_res3 ? "SUCCESS" : "FAILED");

        auto temp_data_key = "temp_data";
        auto temp_data_value = "volatile_value";
        bool set_res4 = client.set(temp_data_key, temp_data_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", temp_data_key, temp_data_value, set_res4 ? "SUCCESS" : "FAILED");

        // --------------------------------------------------
        // 2. GET Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[2] Testing GET Commands</light_green> ---");

        auto server_name_key = "server_name";
        auto server_name_value = client.get(server_name_key);
        client_logger->info("<cyan>GET</cyan> [{}]: <green>{}</green>", server_name_key, server_name_value.value_or("VALUE_NOT_FOUND"));

        // auto welcome_msg_key = "welcome_msg";
        auto welcome_msg_get_value = client.get(welcome_msg_key);
        client_logger->info("<cyan>GET</cyan> [{}]: <green>{}</green>", welcome_msg_key, welcome_msg_get_value.value_or("VALUE_NOT_FOUND"));

        auto non_existent_key = "non_existent_key";
        auto non_existent_value = client.get(non_existent_key);
        if (!non_existent_value.has_value()) {
            client_logger->info("<cyan>GET</cyan> [{}]: Key does <orange>not exist</orange>. (Expected Behavior)", non_existent_key);
        }
        else {
            client_logger->error("<cyan>GET</cyan> [{}]: <orange>Error</orange> - Unexpected data returned: {}", non_existent_key, *non_existent_value);
        }

        // --------------------------------------------------
        // 3. DEL Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[3] Testing DEL Commands</light_green> ---");

        auto del1_key = "temp_data";
        int del_cnt1 = client.del(del1_key);
        client_logger->info("<dark_red>DEL</dark_red> [{}] (Exists) -> Erased count: <pink>{}</pink> item(s)", del1_key, del_cnt1);

        auto get_after_del = client.get(del1_key);
        client_logger->info("<grey>Verification</grey> - <cyan>GET</cyan> [{}] after DEL: <pink>{}</pink>", del1_key, get_after_del.has_value() ? "FAILED (Still Exists)" : "SUCCESS (Null)");

        int del_cnt2 = client.del(del1_key);
        client_logger->info("<dark_red>DEL</dark_red> [{}] (Already Deleted) -> Erased count: <pink>{}</pink> item(s)", del1_key, del_cnt2);

        // --------------------------------------------------
        // 4. SAVE Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[4] Testing SAVE Command</light_green> ---");
        client_logger->info("Requesting server to dump current memory states to local file...");

        if (client.request_server_save()) {
            client_logger->info("SAVE <gold>Completed</gold>: Server file synchronized.");
        }
        else {
            client_logger->error("SAVE Request <orange>Rejected</orange>.");
        }

        // --------------------------------------------------
        // 4.5 DUMP Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[4.5] Testing DUMP Command</light_green> ---");
        client_logger->info("Requesting server to DUMP all key/value pairs to client...");


        try {
            // DUMP 요청 전에, 서버의 현재 대기 시간을 밀리초 단위로 요청하여 가져옵니다.
            auto latency_ms = client.request_server_latency_ms();
            if (latency_ms < 0) {
                client_logger->error("Failed to retrieve server latency.");
            }
            else {
                client_logger->info("Server latency: {} ms", latency_ms);
            }

            // DUMP 요청 후, 대기 시간은 서버의 set_sleep_for_transmission() 설정 시간을 고려하여 답아야 한다.
            // 예를 들어 서버에서 송신 시간마다 50ms 정도 대디하면,
            // 키의 갯수 * 송신 대기 시간 이상의 시간을 dump_timeout 으로 잡아야 한다.
            // auto dump_timeout = std::chrono::seconds(10);
            auto dump_timeout_ms = std::chrono::milliseconds(latency_ms);
            auto dump_timeout = std::chrono::duration_cast<std::chrono::seconds>(dump_timeout_ms * 100);
            client_logger->info("Latency-based DUMP timeout set to {} seconds ({} ms)", dump_timeout.count(), dump_timeout_ms.count());

            auto dump = client.request_server_dump(dump_timeout);
            if (dump.empty()) {
                client_logger->info("DUMP: No entries returned (empty storage or failure).");
            }
            else {
                client_logger->info("DUMP: Received {} entries.", static_cast<int>(dump.size()));
                for (const auto& kv : dump) {
                    client_logger->info("DUMP => <light_blue>{}</light_blue> : <green>{}</green>", kv.first, kv.second);
                }
            }
        }
        catch (const std::exception& ex) {
            client_logger->error("DUMP request failed: {}", ex.what());
        }

        // --------------------------------------------------
        // 5. LOAD Command Test
        // --------------------------------------------------
        client_logger->info("--- <light_green>[5] Testing LOAD Command</light_green> ---");
        client_logger->info("Clearing <green>'{}'</green> from active session to verify restoration...", server_name_key);
        client.del(server_name_key);

        client_logger->info("Current status - <cyan>GET</cyan> [{}]: {}", server_name_key, client.get(server_name_key).value_or("<orange>NULL</orange> (Ready for Load)"));

        client_logger->info("Triggering server-side file LOAD. <grey>(Conflicting client queues will freeze until done)</grey>");
        if (client.request_server_load()) {
            client_logger->info("LOAD <gold>Completed</gold>: Active map restored and unfrozen.");
        }
        else {
            client_logger->error("LOAD Request <orange>Failed</orange>.");
        }

        auto get_after_load = client.get(server_name_key);
        client_logger->info("Restored status - <cyan>GET</cyan> [{}]: {}", server_name_key, get_after_load.value_or("<orange>RESTORATION_FAILED</orange>"));
        client_logger->info("Restored status - <cyan>GET</cyan> [build_version]: {}", client.get("build_version").value_or("<orange>RESTORATION_FAILED</orange>"));

        // --------------------------------------------------
        // 6. Stop and Reconnect Sequence
        // --------------------------------------------------
        client_logger->info("--- <light_green>[6] Testing Client Stop & Reconnect Life-cycle</light_green> ---");
        client_logger->info("<grey>Terminating current active connection socket...</grey>");
        client.stop();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        client_logger->info("<grey>Reinitializing</grey> connection via connect()...");
        if (client.connect(tcp_timeout)) {
            client_logger->info("Re-established session <gold>successfully</gold>.");

            bool set_reconnect = client.set("reconnect_status", "active_ok");
            client_logger->info("Post-reconnect <magenta>SET</magenta> check: <pink>{}</pink>", set_reconnect ? "SUCCESS" : "FAILED");
            client_logger->info("Post-reconnect <cyan>GET</cyan> check: <pink>{}</pink>", client.get("reconnect_status").value_or("FAILED"));
        }
        else {
            client_logger->error("Reconnection <orange>failed</orange>.");
        }

    }
    catch (const std::exception& ex) {
        client_logger->critical("[<red>Pipeline Exception Caught</red>] Network infrastructure <orange>failure</orange>: <pink>{}</pink>", ex.what());
    }

    client_logger->info("==================================================");
    client_logger->info("  <light_green>Memory Store Test Complete. Exiting Program.</light_green>");
    client_logger->info("==================================================");

    client.stop();

#ifdef _WIN32 
    WSACleanup();
#endif
    return 0;
}
