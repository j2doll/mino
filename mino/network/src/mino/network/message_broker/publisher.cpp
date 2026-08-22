#include <thread>

#include <spdlog/spdlog.h>

#include "mino/network/message_broker/pubsub_protocol.hpp"
#include "mino/network/message_broker/publisher.hpp"

namespace mino::network::message_broker {

    publisher::publisher(std::shared_ptr<spdlog::logger> custom_logger) {
        set_logger(custom_logger);
    }

    void publisher::set_logger(std::shared_ptr<spdlog::logger> custom_logger) {
        logger = custom_logger;
        client.set_logger(logger); 
    }

    void publisher::set_broker(const std::string& ip, unsigned short port) {
        broker_ip = ip;
        broker_port = port;
        client.set_server(broker_ip, broker_port);
    }

    void publisher::set_connection_option(std::chrono::seconds interval, int retries) {
        retry_interval = interval;
        max_retries = retries;
    }

    bool publisher::connect(std::chrono::seconds tcp_sleep_time) {
        if (broker_ip.empty() ||
            broker_port == 0)
            return false;

        int attempts = 0;
        while (!client.start(tcp_sleep_time)) {
            attempts++;

            if (max_retries > 0 &&
                attempts >= max_retries)
                return false;

            if (logger) logger->warn("[Publisher] Connect failed. Attempt: {}. Retrying...", attempts);

            std::this_thread::sleep_for(retry_interval);
        }

        if (logger) logger->info("[Publisher] Trying to connect to broker.");

        return true;
    }

    std::pair<bool, std::string> publisher::publish(std::string_view topic, std::string_view msg_kind, std::string_view message) {

        if (!client.is_connected()) {
            return { false, "disconnection_error" };
        }

        if (topic.empty()) {
            return { false, "invalid_topic" };
        }

        std::string packet = make_packet(msg_type::publish, topic, msg_kind, message);

        if (client.send_data(packet) < 0) {
            if (logger) logger->error("[Publisher] Failed to send packet for topic: {}", topic);
            return { false, "socket_error" };
        }

        if (logger) logger->info("[Publisher] Packet sent for topic: <yellow>{0}</yellow>, kind: <orange>{1}</orange>, message length: <grey>{2}</grey>", topic, msg_kind, message.size());

        return { true, "success" };
    }

    void publisher::disconnect() {
        if (logger) logger->info("[Publisher] Disconnecting from broker.");
        client.stop();
    }

    void publisher::shutdown_by_force() {
        if (logger) logger->info("[Publisher] Force shutdown initiated.");

        try {
            client.shutdown_by_force();
        } catch (const std::exception& e) {
            if (logger) logger->error("[Publisher] Exception during force shutdown: {}", e.what());
        } catch (...) {
            if (logger) logger->error("[Publisher] Unknown exception during force shutdown.");
        }
    }

    bool publisher::is_connected() const {
        return client.is_connected();
    }

}  
