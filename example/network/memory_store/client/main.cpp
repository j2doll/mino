#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// network memory_store client
#include "mino/network/ethernet.hpp"
#include "mino/network/memory_store/client.hpp"

int main(int argc, char* argv[]) {
    auto ret_sock = mino::network::init_socket();
    if (ret_sock.has_value()) {
        std::cerr << "Failed: " << ret_sock.value() << std::endl;
        return 1;
    }

    namespace mnm = mino::network::memory_store;
    using memory_store_client = mnm::memory_store_client;

    // 메모리 저장소 클라이언트 
    memory_store_client client;

    // 로거 생성 및 설정 (mino::core::log::tinylog 기반)
    namespace mclt = mino::core::log::tinylog;
    auto console_sink = std::make_shared<mclt::console_sink>("ms_client_console");
    auto client_logger = std::make_shared<mclt::logger>("ms_client_logger");
    client_logger->add_sink(console_sink);
    mclt::logger::register_logger(client_logger);

    client.set_logger(client_logger);

    // 메모리 저장소 서버 환경 설정
    auto server_ip = "127.0.0.1";
    auto server_port = 43322;
    client.set_server_env(server_ip, server_port);

    // 서버에게 요청 후, 응답을 대기하는 시간 (단위:초)    
    auto timeout_sec = std::chrono::seconds(5);
    client.set_timeout(timeout_sec);

    client_logger->info("==================================================");
    client_logger->info("  <bright_green>Memory Store Client Full Command Test Pipeline</bright_green>");
    client_logger->info("==================================================");

    client_logger->info("Connecting to server (127.0.0.1:9999)...");
    std::chrono::seconds tcp_timeout = std::chrono::seconds(5); // TCP 연결 시도 시 응답 대기 시간
    if (!client.connect(tcp_timeout)) {
        client_logger->error("Connection <bright_yellow>Failed.</bright_yellow> Make sure server instance is online.");

        mino::network::close_socket();
        return (-1);
    }
    client_logger->info("Network Connection established <yellow>successfully.</yellow>");

    try {
        //--------------------------------------------------
        // 0. Get Server Latency Test
        //--------------------------------------------------
        client_logger->info("--- <bright_green>[0] Testing Server Latency</bright_green> ---");
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
        client_logger->info("--- <bright_green>[1] Testing SET Commands</bright_green> ---");

        auto app_module_key = "app_mode";
        auto app_module_value = "production";
        bool set_res1 = client.set(app_module_key, app_module_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", app_module_key, app_module_value, set_res1 ? "SUCCESS" : "FAILED");

        auto build_version_key = "build_version";
        auto build_version_value = "v1.4.2_C++17";
        bool set_res2 = client.set(build_version_key, build_version_value);
        client_logger->info("<magenta>SET</magenta> [{} -> <green>{}</green>]: <pink>{}</pink>", build_version_key, build_version_value, set_res2 ? "SUCCESS" : "FAILED");

        auto welcome_msg_key = "welcome_msg";
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
        client_logger->info("--- <bright_green>[2] Testing GET Commands</bright_green> ---");

        auto server_name_key = "server_name";
        auto server_name_value = client.get(server_name_key);
        client_logger->info("<cyan>GET</cyan> [{}]: <green>{}</green>", server_name_key, server_name_value.value_or("VALUE_NOT_FOUND"));

        auto welcome_msg_get_value = client.get(welcome_msg_key);
        client_logger->info("<cyan>GET</cyan> [{}]: <green>{}</green>", welcome_msg_key, welcome_msg_get_value.value_or("VALUE_NOT_FOUND"));

        auto non_existent_key = "non_existent_key";
        auto non_existent_value = client.get(non_existent_key);
        if (!non_existent_value.has_value()) {
            client_logger->info("<cyan>GET</cyan> [{}]: Key does <bright_yellow>not exist</bright_yellow>. (Expected Behavior)", non_existent_key);
        }
        else {
            client_logger->error("<cyan>GET</cyan> [{}]: <bright_yellow>Error</bright_yellow> - Unexpected data returned: {}", non_existent_key, *non_existent_value);
        }

        // --------------------------------------------------
        // 3. DEL Command Test
        // --------------------------------------------------
        client_logger->info("--- <bright_green>[3] Testing DEL Commands</bright_green> ---");

        auto del1_key = "temp_data";
        int del_cnt1 = client.del(del1_key);
        client_logger->info("<red>DEL</red> [{}] (Exists) -> Erased count: <pink>{}</pink> item(s)", del1_key, del_cnt1);

        auto get_after_del = client.get(del1_key);
        client_logger->info("<gray>Verification</gray> - <cyan>GET</cyan> [{}] after DEL: <pink>{}</pink>", del1_key, get_after_del.has_value() ? "FAILED (Still Exists)" : "SUCCESS (Null)");

        int del_cnt2 = client.del(del1_key);
        client_logger->info("<red>DEL</red> [{}] (Already Deleted) -> Erased count: <pink>{}</pink> item(s)", del1_key, del_cnt2);

        // --------------------------------------------------
        // 4. SAVE Command Test
        // --------------------------------------------------
        client_logger->info("--- <bright_green>[4] Testing SAVE Command</bright_green> ---");
        client_logger->info("Requesting server to dump current memory states to local file...");

        if (client.request_server_save()) {
            client_logger->info("SAVE <yellow>Completed</yellow>: Server file synchronized.");
        }
        else {
            client_logger->error("SAVE Request <bright_yellow>Rejected</bright_yellow>.");
        }

        // --------------------------------------------------
        // 4.5 DUMP Command Test
        // --------------------------------------------------
        client_logger->info("--- <bright_green>[4.5] Testing DUMP Command</bright_green> ---");
        client_logger->info("Requesting server to DUMP all key/value pairs to client...");

        try {
            auto latency_ms = client.request_server_latency_ms();
            if (latency_ms < 0) {
                client_logger->error("Failed to retrieve server latency.");
            }
            else {
                client_logger->info("Server latency: {} ms", latency_ms);
            }

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
                    client_logger->info("DUMP => <bright_cyan>{}</bright_cyan> : <green>{}</green>", kv.first, kv.second);
                }
            }
        }
        catch (const std::exception& ex) {
            client_logger->error("DUMP request failed: {}", ex.what());
        }

        // --------------------------------------------------
        // 5. LOAD Command Test
        // --------------------------------------------------
        client_logger->info("--- <bright_green>[5] Testing LOAD Command</bright_green> ---");
        client_logger->info("Clearing <green>'{}'</green> from active session to verify restoration...", server_name_key);
        client.del(server_name_key);

        client_logger->info("Current status - <cyan>GET</cyan> [{}]: {}", server_name_key, client.get(server_name_key).value_or("<bright_yellow>NULL</bright_yellow> (Ready for Load)"));

        client_logger->info("Triggering server-side file LOAD. <gray>(Conflicting client queues will freeze until done)</gray>");
        if (client.request_server_load()) {
            client_logger->info("LOAD <yellow>Completed</yellow>: Active map restored and unfrozen.");
        }
        else {
            client_logger->error("LOAD Request <bright_yellow>Failed</bright_yellow>.");
        }

        auto get_after_load = client.get(server_name_key);
        client_logger->info("Restored status - <cyan>GET</cyan> [{}]: {}", server_name_key, get_after_load.value_or("<bright_yellow>RESTORATION_FAILED</bright_yellow>"));
        client_logger->info("Restored status - <cyan>GET</cyan> [build_version]: {}", client.get("build_version").value_or("<bright_yellow>RESTORATION_FAILED</bright_yellow>"));

        // --------------------------------------------------
        // 6. Stop and Reconnect Sequence
        // --------------------------------------------------
        client_logger->info("--- <bright_green>[6] Testing Client Stop & Reconnect Life-cycle</bright_green> ---");
        client_logger->info("<gray>Terminating current active connection socket...</gray>");
        client.stop();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        client_logger->info("<gray>Reinitializing</gray> connection via connect()...");
        if (client.connect(tcp_timeout)) {
            client_logger->info("Re-established session <yellow>successfully</yellow>.");

            bool set_reconnect = client.set("reconnect_status", "active_ok");
            client_logger->info("Post-reconnect <magenta>SET</magenta> check: <pink>{}</pink>", set_reconnect ? "SUCCESS" : "FAILED");
            client_logger->info("Post-reconnect <cyan>GET</cyan> check: <pink>{}</pink>", client.get("reconnect_status").value_or("FAILED"));
        }
        else {
            client_logger->error("Reconnection <bright_yellow>failed</bright_yellow>.");
        }

    }
    catch (const std::exception& ex) {
        client_logger->critical("[<red>Pipeline Exception Caught</red>] Network infrastructure <bright_yellow>failure</bright_yellow>: <pink>{}</pink>", ex.what());
    }

    // --------------------------------------------------
    // 7. Delete All Command Test
    // --------------------------------------------------
    client_logger->info("--- <bright_green>[Test] Testing delete_all</bright_green> ---");
    int total_deleted = client.delete_all();
    if (total_deleted >= 0) {
        client_logger->info("delete_all <yellow>Completed</yellow>: Total <pink>{}</pink> item(s) erased.", total_deleted);
    }
    else {
        client_logger->error("delete_all <bright_yellow>Failed</bright_yellow>.");
    }

    auto check_after_delete = client.get("build_version");
    client_logger->info("<gray>Verification</gray> - <cyan>GET</cyan> [build_version]: <pink>{}</pink>",
        check_after_delete.has_value() ? "FAILED (Still Exists)" : "SUCCESS (Empty)");

    client_logger->info("==================================================");
    client_logger->info("  <bright_green>Memory Store Test Complete. Exiting Program.</bright_green>");
    client_logger->info("==================================================");

    client.stop();

    mino::network::close_socket();
    return 0;
}
