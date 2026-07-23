#include <thread>

#include <spdlog/spdlog.h>

#include "mino/network/message_broker/pubsub_protocol.hpp"
#include "mino/network/message_broker/subscriber.hpp"

namespace mino::network::message_broker {

    subscriber::subscriber(std::shared_ptr<spdlog::logger> custom_logger) {
        set_logger(custom_logger);

        if (logger) logger->info("[Subscriber] Initializing subscriber.");

        client.set_on_connect([this]() {
            if (logger) logger->info("[Subscriber] Connected. Sending subscription packets to broker...");

            for (const auto& topic : sub_topics) {
                std::string sub_packet = make_packet(msg_type::subscribe, topic, "");
                client.send_data(sub_packet);
                if (logger) logger->info("[Subscriber] Sent subscription packet for topic: {}", topic);
            }
        });

        client.set_on_close([this]() {
            if (logger) logger->warn("[Subscriber] Connection closed. Clearing stream buffer.");
            stream_buffer.clear();
        });

        client.set_on_receive([this](const std::string& data) {
            stream_buffer.append(data);
            ascii_header header;
            while (parse_ascii_header(stream_buffer, header)) {
                size_t total_packet_size = header.header_full_size + header.body_len;
                if (stream_buffer.size() < total_packet_size) break;

                std::string_view current_stream(stream_buffer);
                std::string_view body = current_stream.substr(header.header_full_size, header.body_len);

                if (app_callback) {
                    app_callback(header.topic, header.msg_kind, body, header.timestamp);
                }
                stream_buffer.erase(0, total_packet_size);
            }
        });
    }

    void subscriber::set_logger(std::shared_ptr<spdlog::logger> custom_logger) {
        if (custom_logger) {
            logger = custom_logger;
            client.set_logger(logger);
        }
    }

    void subscriber::set_on_message_handler(message_callback cb) {
        app_callback = std::move(cb);
    }

    void subscriber::set_broker(const std::string& ip, unsigned short port) {
        broker_ip = ip;
        broker_port = port;
        client.set_server(broker_ip, broker_port);
    }

    void subscriber::set_topic(const std::vector<std::string>& topics) {
        sub_topics = topics;
    }

    void subscriber::set_connection_option(std::chrono::seconds interval, int retries) {
        retry_interval = interval;
        max_retries = retries;
    }

    bool subscriber::connect(std::chrono::seconds tcp_sleep_time) {
        if (broker_ip.empty() ||
            broker_port == 0) {
            if (logger) logger->error("[Subscriber] Broker IP or port is not set.");
            return false;
        }

        int attempts = 0;
        while (!client.start(tcp_sleep_time)) {
            attempts++;
            if (logger) logger->warn("[Subscriber] Connection attempt {} failed. Retrying...", attempts);
            if (max_retries > 0 && attempts >= max_retries) {
                return false;
            }
            std::this_thread::sleep_for(retry_interval);
        }

        return true;
    }

    bool subscriber::is_connected() const {
        return client.is_connected();
    }

    void subscriber::disconnect() {
        if (logger) logger->info("[Subscriber] Disconnecting from broker.");
        client.stop();
    }

    void subscriber::shutdown_by_force() {
        if (logger) logger->warn("[Subscriber] Forcefully shutting down connection.");
        try {
            client.shutdown_by_force();
        } catch (const std::exception& ex) {
            if (logger) logger->error("[Subscriber] Exception during forced shutdown: {}", ex.what());
        } catch (...) {
            if (logger) logger->error("[Subscriber] Unknown exception during forced shutdown.");
        }
    }

}  