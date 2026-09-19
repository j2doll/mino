#include <stdexcept>
#include <thread>
#include <sstream>

#include "mino/core/json/json.hpp"
#include "mino/core/log/tinylog/logger.hpp"
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
        std::string_view, // topic
        std::string_view msg_kind,
        std::string_view body,
        uint64_t // timestamp
    )
    {
        if (msg_kind != "rpc_res")
            return;

        try {
            auto j = mino::core::json::parser::parse(body);
            if (!j.is_object() || !j.has_path("request_id")) {
                if (logger) logger->error("[RPC Client Core] Invalid RPC response format: missing request_id or not an object");
                return;
            }

            std::string req_id = j["request_id"].get_string();

            std::promise<json> target_promise;
            bool found = false;

            {
                std::scoped_lock lock(map_mutex);
                auto it = response_map.find(req_id);
                if (it != response_map.end()) {
                    target_promise = std::move(it->second);
                    response_map.erase(it);
                    found = true;
                }
            }

            if (found) {
                target_promise.set_value(std::move(j));
            }
        }
        catch (const std::exception& e) {
            if (logger) logger->error("[RPC Client Core] Message Handling Error: {}", e.what());
        }
    }

    void rpc_client_base::handle_disconnection() {
        {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            if (logger) logger->warn("[RPC Client Core] Connection <bright_yellow>broken</bright_yellow>. Aborting calls. thread_id={}", ss.str());
        }

        std::unordered_map<std::string, std::promise<json>> temp_map;

        {
            std::scoped_lock lock(map_mutex);
            temp_map.swap(response_map);
        }

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

    void rpc_client_base::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> custom_logger) {
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
        if (logger) logger->info("[RPC Client Core] Attempting to connect publisher and subscriber to broker.");

        if (!pub.connect(tcp_sleep_time)) {
            if (logger) logger->error("[RPC Client Core] Publisher failed to connect.");
            return false;
        }

        if (!sub.connect(tcp_sleep_time)) {
            if (logger) logger->error("[RPC Client Core] Subscriber failed to connect. Disconnecting publisher.");
            pub.disconnect();
            return false;
        }

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

    // 새로 추가한 상태 확인 함수
    bool rpc_client_base::is_connected() const {
        return pub.is_connected() && sub.is_connected();
    }

    std::pair<rpc_status, rpc_client_base::json> rpc_client_base::call_raw(
        std::string_view service_name,
        const json& raw_argument,
        std::chrono::seconds timeout)
    {
        if (!pub.is_connected()) {
            handle_disconnection();
            return { { rpc_error_code::connection_broken, "Link already dead" }, mino::core::json::value(mino::core::json::object_t{}) };
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
            return { { rpc_error_code::connection_broken, "Publish failed: " + err_msg }, mino::core::json::value(mino::core::json::object_t{}) };
        }

        try {
            if (future_res.wait_for(timeout) == std::future_status::timeout) {
                std::scoped_lock lock(map_mutex);
                response_map.erase(req_id);
                return { { rpc_error_code::timeout, "Request timed out" }, mino::core::json::value(mino::core::json::object_t{}) };
            }

            auto response_json = future_res.get();
            bool is_success = response_json["success"].get_bool();

            if (is_success) {
                return { { rpc_error_code::success, "OK" }, response_json["result"] };
            }
            else {
                std::string err_type = response_json["error_code"].get_string();
                rpc_error_code code = rpc_error_code::unknown_error;

                if (err_type == "service_not_found") code = rpc_error_code::service_not_found;
                else if (err_type == "invalid_argument") code = rpc_error_code::invalid_argument;
                else if (err_type == "internal_server_error") code = rpc_error_code::internal_server_error;

                return { { code, response_json["result"].get_string() }, mino::core::json::value(mino::core::json::object_t{}) };
            }
        }
        catch (const std::runtime_error& ex) {
            return {
                { rpc_error_code::connection_broken,
                  std::string("Channel disconnected: ") + ex.what()
                },
                mino::core::json::value(mino::core::json::object_t{})
            };
        }
    }

    void rpc_client_base::disconnect() {
        sub.set_on_message_handler(nullptr);
        handle_disconnection();
        pub.disconnect();
        sub.disconnect();
    }

}
