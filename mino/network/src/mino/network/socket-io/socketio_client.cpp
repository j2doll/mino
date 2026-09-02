#ifdef USE_CURL

#include <iostream>
#include <cstring>
#include <thread>

#include "mino/network/socket-io/socketio_client.hpp"

namespace mino::network::socket_io {

    socketio_client_base::socketio_client_base() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();
    }

    socketio_client_base::~socketio_client_base() {
        close("Destructor called");
        if (curl_handle) {
            curl_easy_cleanup(curl_handle);
        }
        curl_global_cleanup();
    }

    void socketio_client_base::set_server_endpoint(std::string host, int port) {
        server_host = std::move(host);
        server_port = port;
    }

    void socketio_client_base::set_connect_timeout_seconds(long timeout_sec) {
        connect_timeout_seconds = timeout_sec;
    }

    void socketio_client_base::set_event_listener(std::shared_ptr<socketio_event_listener> listener) {
        custom_listener = listener;
    }

    void socketio_client_base::set_namespace(std::string ns) {
        if (ns.empty() || ns[0] != '/') {
            target_namespace = "/" + ns;
        }
        else {
            target_namespace = std::move(ns);
        }
    }

    void socketio_client_base::on_open(std::function<void()> handler) {
        on_open_callback = std::move(handler);
    }

    void socketio_client_base::on_close(std::function<void(const std::string&)> handler) {
        on_close_callback = std::move(handler);
    }

    void socketio_client_base::on_event(std::function<void(const std::string&)> handler) {
        on_event_callback = std::move(handler);
    }

    void socketio_client_base::on_error(std::function<void(const std::string&)> handler) {
        on_error_callback = std::move(handler);
    }

    void socketio_client_base::on(const std::string& event_name, std::function<void(const std::string&)> handler) {
        event_handlers[event_name] = std::move(handler);
    }

    void socketio_client_base::reset() {
        is_running = false;
        if (curl_handle) {
            curl_easy_cleanup(curl_handle);
            curl_handle = nullptr;
        }
        curl_handle = curl_easy_init();
    }

    bool socketio_client_base::connect() {
        if (!curl_handle || server_host.empty() || server_port <= 0) {
            notify_error("Server endpoint (host/port) is not configured properly.");
            return false;
        }

        std::string eio = get_engine_io_version();
        std::string handshake_url = "http://" + server_host + ":" + std::to_string(server_port) +
            "/socket.io/?EIO=" + eio + "&transport=websocket";

        curl_easy_setopt(curl_handle, CURLOPT_URL, handshake_url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connect_timeout_seconds);

        if (curl_easy_perform(curl_handle) != CURLE_OK) {
            notify_error("TCP connection failed.");
            return false;
        }

        std::string http_handshake =
            "GET /socket.io/?EIO=" + eio + "&transport=websocket HTTP/1.1\r\n"
            "Host: " + server_host + ":" + std::to_string(server_port) + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Origin: http://" + server_host + ":" + std::to_string(server_port) + "\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n";

        if (!raw_send(http_handshake)) {
            notify_error("Failed to send HTTP upgrade handshake.");
            return false;
        }

        auto start_time = std::chrono::steady_clock::now();
        std::string response;
        char buffer[1024];

        while (response.find("\r\n\r\n") == std::string::npos) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (elapsed >= connect_timeout_seconds) {
                notify_error("Handshake timed out waiting for 101 Switching Protocols.");
                return false;
            }

            size_t bytes_read = 0;
            CURLcode res = curl_easy_recv(curl_handle, buffer, sizeof(buffer), &bytes_read);
            if (res == CURLE_OK && bytes_read > 0) {
                response.append(buffer, bytes_read);
            }
            else if (res != CURLE_AGAIN && res != CURLE_OK) {
                notify_error("Socket error during handshake reception.");
                return false;
            }
        }

        if (response.find("101 Switching Protocols") == std::string::npos) {
            notify_error("Server refused WebSocket upgrade:\n" + response);
            return false;
        }

        return true;
    }

    void socketio_client_base::emit(const std::string& event_name, const std::string& json_payload) {
        std::string packet = "42";
        if (target_namespace != "/") {
            packet += target_namespace + ",";
        }
        packet += "[\"" + event_name + "\"," + json_payload + "]";
        send_engineio_packet(packet);
    }

    void socketio_client_base::join_room(const std::string& room_name) {
        if (room_name.empty()) return;
        emit("join_room", "{\"room\":\"" + room_name + "\"}");
    }

    void socketio_client_base::leave_room(const std::string& room_name) {
        if (room_name.empty()) return;
        emit("leave_room", "{\"room\":\"" + room_name + "\"}");
    }

    void socketio_client_base::run_event_loop() {
        std::vector<uint8_t> rx_buffer;
        char temp_chunk[2048];
        is_running = true;

        while (is_running) {
            on_heartbeat_tick();

            size_t bytes_read = 0;
            CURLcode res = curl_easy_recv(curl_handle, temp_chunk, sizeof(temp_chunk), &bytes_read);

            if (res != CURLE_OK) {
                if (res == CURLE_AGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                else {
                    notify_error("Network error: " + std::string(curl_easy_strerror(res)));
                    close("Socket connection broken");
                    break;
                }
            }

            if (bytes_read == 0) {
                close("Remote server closed the connection (EOF)");
                break;
            }

            rx_buffer.insert(rx_buffer.end(), temp_chunk, temp_chunk + bytes_read);

            while (rx_buffer.size() >= 2) {
                uint8_t first_byte = rx_buffer[0];
                uint8_t second_byte = rx_buffer[1];

                uint8_t opcode = first_byte & 0x0F;
                bool is_masked = (second_byte & 0x80) != 0;
                size_t payload_len = second_byte & 0x7F;
                size_t header_len = 2;

                if (payload_len == 126) {
                    if (rx_buffer.size() < 4) break;
                    payload_len = (static_cast<size_t>(rx_buffer[2]) << 8) | rx_buffer[3];
                    header_len = 4;
                }
                else if (payload_len == 127) {
                    if (rx_buffer.size() < 10) break;
                    payload_len = 0;
                    for (int i = 0; i < 8; ++i) {
                        payload_len = (payload_len << 8) | rx_buffer[2 + i];
                    }
                    header_len = 10;
                }

                size_t mask_len = is_masked ? 4 : 0;
                size_t total_frame_len = header_len + mask_len + payload_len;

                if (rx_buffer.size() < total_frame_len) {
                    break;
                }

                const uint8_t* mask_key = is_masked ? (rx_buffer.data() + header_len) : nullptr;
                const uint8_t* raw_payload = rx_buffer.data() + header_len + mask_len;

                std::string payload;
                payload.resize(payload_len);
                for (size_t i = 0; i < payload_len; ++i) {
                    if (is_masked) {
                        payload[i] = static_cast<char>(raw_payload[i] ^ mask_key[i % 4]);
                    }
                    else {
                        payload[i] = static_cast<char>(raw_payload[i]);
                    }
                }

                if (opcode == 0x08) {
                    std::string close_reason = "Received WebSocket close frame";
                    if (payload_len >= 2) {
                        uint16_t close_code = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
                        close_reason += " (code: " + std::to_string(close_code) + ")";
                    }
                    close(close_reason);
                    rx_buffer.clear();
                    return;
                }

                if (opcode == 0x01) {
                    dispatch_packet(payload);
                }

                rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin() + total_frame_len);
            }
        }
    }

    void socketio_client_base::close(const std::string& reason) {
        if (!is_running) return;
        is_running = false;
        if (custom_listener) custom_listener->on_close(reason);
        if (on_close_callback) on_close_callback(reason);
    }

    void socketio_client_base::send_engineio_packet(const std::string& packet_data) {
        std::vector<uint8_t> frame;
        frame.push_back(0x81);

        size_t len = packet_data.size();
        uint8_t mask_bit = 0x80;

        if (len <= 125) {
            frame.push_back(static_cast<uint8_t>(len) | mask_bit);
        }
        else if (len <= 65535) {
            frame.push_back(126 | mask_bit);
            frame.push_back((len >> 8) & 0xFF);
            frame.push_back(len & 0xFF);
        }
        else {
            frame.push_back(127 | mask_bit);
            for (int i = 7; i >= 0; --i) frame.push_back((len >> (i * 8)) & 0xFF);
        }

        uint8_t masking_key[4] = { 0x1A, 0x2B, 0x3C, 0x4D };
        frame.insert(frame.end(), masking_key, masking_key + 4);

        for (size_t i = 0; i < len; ++i) {
            frame.push_back(static_cast<uint8_t>(packet_data[i]) ^ masking_key[i % 4]);
        }

        size_t bytes_sent = 0;
        if (curl_easy_send(curl_handle, frame.data(), frame.size(), &bytes_sent) != CURLE_OK) {
            notify_error("Failed to send frame via curl_easy_send.");
        }
    }

    void socketio_client_base::notify_error(const std::string& err_msg) {
        if (custom_listener) custom_listener->on_error(err_msg);
        if (on_error_callback) on_error_callback(err_msg);
    }

    void socketio_client_base::dispatch_packet(const std::string& packet) {
        if (packet.empty()) return;
        char type = packet[0];
        std::string payload = packet.substr(1);

        switch (type) {
        case '0':
            on_handshake_open(payload);
            break;
        case '2':
            handle_ping();
            break;
        case '3':
            handle_pong();
            break;
        case '4':
            handle_socketio_message(payload);
            break;
        case '1':
            close("Server issued disconnect (Packet type 1).");
            break;
        default:
            break;
        }
    }

    void socketio_client_base::handle_socketio_message(const std::string& message) {
        if (message.empty()) return;
        char sio_type = message[0];
        std::string body = message.substr(1);

        if (!body.empty() && body[0] == '/') {
            size_t comma_pos = body.find(',');
            if (comma_pos != std::string::npos) {
                std::string ns = body.substr(0, comma_pos);
                if (ns != target_namespace) {
                    return;
                }
                body = body.substr(comma_pos + 1);
            }
        }

        if (sio_type == '0') {
            if (custom_listener) custom_listener->on_open();
            if (on_open_callback) on_open_callback();
        }
        else if (sio_type == '2') {
            if (custom_listener) custom_listener->on_event(body);
            if (on_event_callback) on_event_callback(body);

            if (!event_handlers.empty() && body.size() >= 2 && body[0] == '[') {
                size_t first_quote = body.find('\"');
                size_t second_quote = body.find('\"', first_quote + 1);

                if (first_quote != std::string::npos && second_quote != std::string::npos) {
                    std::string event_name = body.substr(first_quote + 1, second_quote - first_quote - 1);

                    auto it = event_handlers.find(event_name);
                    if (it != event_handlers.end()) {
                        size_t comma_pos = body.find(',', second_quote);
                        std::string payload_data;
                        if (comma_pos != std::string::npos && comma_pos < body.size() - 1) {
                            size_t last_bracket = body.rfind(']');
                            if (last_bracket != std::string::npos && last_bracket > comma_pos) {
                                payload_data = body.substr(comma_pos + 1, last_bracket - comma_pos - 1);
                            }
                        }
                        it->second(payload_data.empty() ? body : payload_data);
                    }
                }
            }
        }
        else if (sio_type == '4') {
            notify_error("Socket.IO error received: " + body);
        }
    }

    bool socketio_client_base::raw_send(const std::string& data) {
        size_t total_sent = 0;
        while (total_sent < data.size()) {
            size_t sent = 0;
            if (curl_easy_send(curl_handle, data.data() + total_sent, data.size() - total_sent, &sent) != CURLE_OK) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    // -------------------------------------------------------------
    // 파생 클래스 구현
    // -------------------------------------------------------------
    std::string socketio_client_v1::get_engine_io_version() const { return "2"; }
    void socketio_client_v1::on_handshake_open(const std::string&) {
        if (target_namespace == "/") {
            send_engineio_packet("40/");
        }
        else {
            send_engineio_packet("40" + target_namespace + ",");
        }
        last_ping_time = std::chrono::steady_clock::now();
    }
    void socketio_client_v1::handle_ping() { send_engineio_packet("3"); }
    void socketio_client_v1::handle_pong() {}
    void socketio_client_v1::on_heartbeat_tick() {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_ping_time).count() >= 25) {
            send_engineio_packet("2");
            last_ping_time = now;
        }
    }

    std::string socketio_client_v2::get_engine_io_version() const { return "3"; }
    void socketio_client_v2::on_handshake_open(const std::string&) {
        if (target_namespace == "/") {
            send_engineio_packet("40/");
        }
        else {
            send_engineio_packet("40" + target_namespace + ",");
        }
        last_ping_time = std::chrono::steady_clock::now();
    }
    void socketio_client_v2::handle_ping() { send_engineio_packet("3"); }
    void socketio_client_v2::handle_pong() {}
    void socketio_client_v2::on_heartbeat_tick() {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_ping_time).count() >= 20) {
            send_engineio_packet("2");
            last_ping_time = now;
        }
    }

    std::string socketio_client_v4::get_engine_io_version() const { return "4"; }
    void socketio_client_v4::on_handshake_open(const std::string&) {
        if (target_namespace == "/") {
            send_engineio_packet("40");
        }
        else {
            send_engineio_packet("40" + target_namespace + ",");
        }
    }
    void socketio_client_v4::handle_ping() { send_engineio_packet("3"); }
    void socketio_client_v4::handle_pong() {}

    std::unique_ptr<socketio_client_base> create_socketio_client(socketio_version ver) {
        switch (ver) {
        case socketio_version::v1: return std::make_unique<socketio_client_v1>();
        case socketio_version::v2: return std::make_unique<socketio_client_v2>();
        case socketio_version::v4: return std::make_unique<socketio_client_v4>();
        default: return std::make_unique<socketio_client_v4>();
        }
    }

} // namespace mino::network::socket_io

#endif // USE_CURL
