#include <stdexcept>
#include <thread>
#include <sstream>

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include "mino/network/rpc/rpc_client_base.hpp"
#include "mino/network/rpc/rpc_protocol_util.hpp"

namespace mino::network::rpc {

    rpc_client_base::~rpc_client_base() = default;

    rpc_client_base::rpc_client_base() {
        sub.set_on_message_handler([this](auto t, auto k, auto b, auto ts) {
            on_message_received(t, k, b, ts);
            });
    }

    void rpc_client_base::on_message_received(
        std::string_view , // topic,
        std::string_view msg_kind,
        std::string_view body,
        uint64_t // timestamp
    )
    {
        if (msg_kind != "rpc_res")
            return;

        try {
            auto j = json::parse(body);
            std::string req_id = j["request_id"].get<std::string>();

            std::promise<json> target_promise;
            bool found = false;

            // 뮤텍스 락 범위를 최소화하여 데드락 발생 소지를 완전히 제거합니다.
            {
                std::scoped_lock lock(map_mutex);
                auto it = response_map.find(req_id);
                if (it != response_map.end()) {
                    target_promise = std::move(it->second);
                    response_map.erase(it);
                    found = true;
                }
            }

            // set_value는 뮤텍스 락 밖에서 실행해야 안전합니다.
            if (found) {
                target_promise.set_value(j);
            }
        }
        catch (const std::exception& e) {
            if (logger) logger->error("[RPC Client Core] JSON Parse Error: {}", e.what());
        }
    }

    void rpc_client_base::handle_disconnection() {
        {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            if (logger) logger->warn("[RPC Client Core] Connection <orange>broken</orange>. Aborting calls. thread_id={}", ss.str());
        }

        std::unordered_map<std::string, std::promise<json>> temp_map;

        // 원본 맵을 통째로 로컬 영역으로 swap하여 락을 쥔 채 오래 머물지 않도록 합니다.
        {
            std::scoped_lock lock(map_mutex);
            temp_map.swap(response_map);
        }

        // 락이 완전히 풀린 안전한 상태에서 대기 풀에 예외를 주입합니다.
        for (auto& [req_id, pr] : temp_map) {
            try {
                pr.set_exception(std::make_exception_ptr(std::runtime_error("rpc_connection_broken")));
            }
            catch (...) {}
        }
    }

    void rpc_client_base::set_id(std::string unique_id) {
        client_id = std::move(unique_id);
        sub.set_topic({ "rpc_res/" + client_id });
    }

    void rpc_client_base::set_logger(std::shared_ptr<spdlog::logger> custom_logger) {
        logger = custom_logger;
        pub.set_logger(logger);
        sub.set_logger(logger);
    }

    void rpc_client_base::set_broker(const std::string& ip, unsigned short port) {
        pub.set_broker(ip, port);
        sub.set_broker(ip, port);
    }

    void rpc_client_base::set_connection_timeout(std::chrono::milliseconds timeout) {
        connection_timeout = timeout;
    }

    bool rpc_client_base::connect(std::chrono::seconds tcp_sleep_time) {
        // 시도 로그
        if (logger) logger->info("[RPC Client Core] Attempting to connect publisher and subscriber to broker.");

        // 우선 각각의 구성요소에 대해 연결을 시도합니다.
        if (!pub.connect(tcp_sleep_time)) {
            if (logger) logger->error("[RPC Client Core] Publisher failed to connect.");
            return false;
        }

        if (!sub.connect(tcp_sleep_time)) {
            if (logger) logger->error("[RPC Client Core] Subscriber failed to connect. Disconnecting publisher.");
            pub.disconnect();
            return false;
        }

        // 실제로 연결이 확립되었는지 타임아웃 내에 확인합니다.
        auto elapsed = std::chrono::milliseconds(0);
        const auto interval = std::chrono::milliseconds(10);

        while ((!pub.is_connected() || !sub.is_connected()) && elapsed < connection_timeout) {
            std::this_thread::sleep_for(interval);
            elapsed += interval;
        }

        if (!pub.is_connected() || !sub.is_connected()) {
            if (logger) {
                logger->error("[RPC Client Core] Connection timeout: Publisher or Subscriber failed to establish link within {}ms", connection_timeout.count());
            }
            disconnect();
            return false;
        }

        if (logger) {
            logger->info("[RPC Client Core] Connected and verified broker session <green>successfully</green>.");
        }
        return true;
    }

    std::pair<rpc_status, rpc_client_base::json> rpc_client_base::call_raw(
        std::string_view service_name,
        const json& raw_argument, std::chrono::seconds timeout)
    {
        if (!pub.is_connected()) {
            handle_disconnection();
            return { { rpc_error_code::connection_broken, "Link already dead" }, json::object() };
        }

        std::string req_id = client_id + "_" + std::to_string(++sequence_id);
        std::future<json> future_res;
        {
            std::scoped_lock lock(map_mutex);
            future_res = response_map[req_id].get_future();
        }

        std::string res_topic = "rpc_res/" + client_id;
        std::string payload = rpc_protocol_util::serialize_request(req_id, res_topic, raw_argument);

        auto [send_ok, err_msg] = pub.publish(service_name, "rpc_req", payload);
        if (!send_ok) {
            std::scoped_lock lock(map_mutex);
            response_map.erase(req_id);
            return { { rpc_error_code::connection_broken, "Publish failed: " + err_msg }, json::object() };
        }

        try {
            if (future_res.wait_for(timeout) == std::future_status::timeout) {
                std::scoped_lock lock(map_mutex);
                response_map.erase(req_id);
                return { { rpc_error_code::timeout, "Request timed out" }, json::object() };
            }

            auto response_json = future_res.get();
            bool is_success = response_json["success"].get<bool>();

            if (is_success) {
                return { { rpc_error_code::success, "OK" }, response_json["result"] };
            }
            else {
                std::string err_type = response_json["error_code"].get<std::string>();
                rpc_error_code code = rpc_error_code::unknown_error;

                if (err_type == "service_not_found") code = rpc_error_code::service_not_found;
                else if (err_type == "invalid_argument") code = rpc_error_code::invalid_argument;
                else if (err_type == "internal_server_error") code = rpc_error_code::internal_server_error;

                return { { code, response_json["result"].get<std::string>() }, json::object() };
            }
        }
        catch (const std::runtime_error& ex) {
            return {
                        { rpc_error_code::connection_broken,
                          std::string("Channel disconnected: ") + ex.what()
                        },
                        json::object()
                    };
        }
    }

    void rpc_client_base::disconnect() {
        // [리눅스 교정 핵심] 소켓을 해제하기 전에 빈 콜백을 주입하여 백그라운드 스레드의 상위 이벤트 전파를 차단합니다.
        sub.set_on_message_handler(nullptr);

        handle_disconnection();
        pub.disconnect();
        sub.disconnect();
    }

}
