#include <thread>

#include <nlohmann/json.hpp>
 
#include <spdlog/spdlog.h>

#include "mino/network/rpc/rpc_server_base.hpp"
#include "mino/network/rpc/rpc_protocol_util.hpp"

namespace mino::network::rpc {

    rpc_server_base::rpc_server_base() :
        logger(nullptr),
        startup_timeout(std::chrono::milliseconds(3000))
    {
        sub.set_on_message_handler([this](auto t, auto k, auto b, auto ts) {
                on_request_received(t, k, b, ts);
            });
    }

    void rpc_server_base::on_request_received(
        std::string_view topic,
        std::string_view msg_kind,
        std::string_view body,
        uint64_t ) // timestamp)
    {
        if (msg_kind != "rpc_req") return;
        std::string req_id, res_topic;
        json argument;

        try {
            auto j = json::parse(body);
            req_id = j["request_id"].get<std::string>();
            res_topic = j["response_topic"].get<std::string>();
            argument = j["argument"];
        }
        catch (const std::exception& e) {
            if (logger) logger->error("[RPC Server Core] Invalid packet format: {}", e.what());
            return;
        }

        raw_handler handler = nullptr;
        {
            std::scoped_lock lock(registry_mutex);
            auto it = service_registry.find(std::string(topic));
            if (it != service_registry.end()) handler = it->second;
        }

        std::string res_body;
        if (handler) {
            try {
                json result_json = handler(argument);
                res_body = rpc_protocol_util::serialize_response(req_id, true, "", result_json);
            }
            catch (const json::exception& e) {
                res_body = rpc_protocol_util::serialize_response(req_id, false, "invalid_argument", std::string("Argument schema mismatch: ") + e.what());
            }
            catch (const std::exception& e) {
                res_body = rpc_protocol_util::serialize_response(req_id, false, "internal_server_error", std::string("Server business exception: ") + e.what());
            }
        }
        else {
            res_body = rpc_protocol_util::serialize_response(req_id, false, "service_not_found", "The requested RPC service is not supported by this node");
        }

        pub.publish(res_topic, "rpc_res", res_body);
    }

    void rpc_server_base::setup_logger(std::shared_ptr<spdlog::logger> custom_logger) {
        logger = custom_logger;
        pub.set_logger(logger);
        sub.set_logger(logger);
    }

    void rpc_server_base::register_raw_service(const std::string& name, raw_handler handler) {
        std::scoped_lock lock(registry_mutex);
        service_registry[name] = std::move(handler);
        service_topics.push_back(name);
        sub.set_topic(service_topics);
    }

    void rpc_server_base::set_broker(const std::string& ip, unsigned short port) {
        pub.set_broker(ip, port);
        sub.set_broker(ip, port);
    }

    void rpc_server_base::set_startup_timeout(std::chrono::milliseconds timeout) {
        startup_timeout = timeout;
    }

    bool rpc_server_base::start(std::chrono::seconds tcp_sleep_time) {
        if (!pub.connect(tcp_sleep_time) ||
            !sub.connect(tcp_sleep_time) )
        {
            pub.disconnect();
            return false;
        }

        auto elapsed = std::chrono::milliseconds(0);
        const auto interval = std::chrono::milliseconds(10);

        while (!pub.is_connected() && elapsed < startup_timeout) {
            std::this_thread::sleep_for(interval);
            elapsed += interval;
        }

        if (!pub.is_connected()) {
            if (logger) {
                logger->error("[RPC Server Core] Startup timeout: Failed to connect to broker within {}ms", startup_timeout.count());
            }
            stop();
            return false;
        }

        if (logger) {
            logger->info("[RPC Server Core] Server booted and link verified successfully.");
        }
        return true;
    }

    void rpc_server_base::stop() {
        // [리눅스 교정 핵심] 이벤트를 먼저 무효화한 후 소켓 파괴
        sub.set_on_message_handler(nullptr);
        pub.disconnect();
        sub.disconnect();
    }

} 