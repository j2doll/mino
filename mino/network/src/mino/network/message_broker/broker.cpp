#include <algorithm>
#include <string_view>

#include <spdlog/spdlog.h>

#include "mino/network/message_broker/message_broker.hpp"

namespace mino::network::message_broker {

    broker::broker(std::shared_ptr<spdlog::logger> custom_logger) {
        set_logger(custom_logger);

        if (logger) logger->info("[Broker] Broker instance created.");

        // 복사 캡처([=]) 또는 주입 상태 변화 대응을 위해 인스턴스 포인터 참조 가동
        server.set_on_connect_callback([this](socket_t client_socket, const std::string& client_ip) {
             if (logger)logger->info("[Broker] Client connected: {} (Socket: {})", client_ip, client_socket);
        });

        server.set_on_close_callback([this](socket_t client_socket, const std::string& reason) {
            std::scoped_lock lock(registry_mutex);
            for (auto& [topic, sockets] : topic_registry) {
                sockets.erase(std::remove(sockets.begin(), sockets.end(), client_socket), sockets.end());
            }
            stream_buffers.erase(client_socket);
            if (logger) logger->warn("[Broker] Client disconnected (Socket: {}). Registry cleaned. Reason: {}", client_socket, reason);
        });

        server.set_on_receive_callback([this](socket_t client_socket, const std::string& data) {
            std::scoped_lock lock(registry_mutex);

            stream_buffers[client_socket].append(data);
            std::string& buffer = stream_buffers[client_socket];

            ascii_header header;

            while (parse_ascii_header(buffer, header)) {
                size_t total_packet_size = header.header_full_size + header.body_len;

                if (buffer.size() < total_packet_size)
                    break;

                std::string_view current_stream(buffer);
                std::string_view body = current_stream.substr(header.header_full_size, header.body_len);

                if (header.type == msg_type::subscribe) {
                    auto& sockets = topic_registry[header.topic];
                    if (std::find(sockets.begin(), sockets.end(), client_socket) == sockets.end()) {
                        sockets.push_back(client_socket);
                        if (logger) {
                            logger->info("[Broker] Socket {} registered to topic: [{}] (ASCII Protocol)", client_socket, header.topic);
                        }
                    }
                }
                else if (header.type == msg_type::publish) {
                    if (logger) {
                        logger->info("[Broker] Published topic [{}] from socket: {}", header.topic, client_socket);
                    }

                    if (auto it = topic_registry.find(header.topic); it != topic_registry.end()) {
                        std::string relay_packet = make_packet(msg_type::publish, header.topic, header.msg_kind, body);
                        for (socket_t sub_socket : it->second) {
                            server.send_to_client(sub_socket, relay_packet);
                        }
                        if (logger) {
                            logger->debug("[Broker] Relayed topic <{}> to {} subscribers.", header.topic, it->second.size());
                        }
                    }
                    else {
                        if (logger) {
                            logger->warn("[Broker] No subscribers found for topic: [{}]", header.topic);
                        }
                    }
                }

                buffer.erase(0, total_packet_size);
            }
        });
    }

    void broker::set_logger(std::shared_ptr<spdlog::logger> custom_logger) {
        if (custom_logger) {
            logger = custom_logger;
            server.set_logger(custom_logger); // set tcp_server spdlog logger 
        }
    }

    void broker::start_broker(const std::string& ip, int port) {
        if (logger) logger->info("[Broker] Attempting to start broker on {}:{}", ip, port);

        if (server.start(ip, port) == mino::network::tcp::tcp_server::start_result::success) {
            if (logger) logger->info("[Broker] Central Message Broker started on {}:{}", ip, port);
        }
        else {
            if (logger) logger->error("[Broker] Failed to start broker on {}:{}", ip, port);
        }
    }

    void broker::quit() {
        if (logger) logger->info("[Broker] Shutting down broker.");

        server.quit();
    }

    void broker::shutdown_by_force() {
        if (logger) logger->warn("[Broker] Forcefully shutting down broker.");

        try {
            server.shutdown_by_force();
        } catch (const std::exception& ex) {
            if (logger) logger->error("[Broker] Exception during forced shutdown: {}", ex.what());
        } catch (...) {
            if (logger) logger->error("[Broker] Unknown exception during forced shutdown.");
        }
    }

} 
