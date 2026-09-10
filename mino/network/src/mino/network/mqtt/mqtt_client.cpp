#include "mino/network/mqtt/mqtt_client.hpp"

namespace mino::network::mqtt {

    mqtt_client::mqtt_client() noexcept
        : host_(""), port_(0), address_family_(AF_INET), client_id_(""),
        keep_alive_seconds_(60), packet_id_counter_(1) {
        setup_tcp_callbacks();
    }

    mqtt_client::~mqtt_client() noexcept {
        stop();
    }

    mqtt_client& mqtt_client::set_broker(std::string_view host, int port) noexcept {
        try {
            host_ = host;
            port_ = port;
            address_family_ = (host_.find(':') != std::string::npos) ? AF_INET6 : AF_INET;
        }
        catch (...) {}
        return *this;
    }

    mqtt_client& mqtt_client::set_client_id(std::string_view client_id) noexcept {
        try {
            client_id_ = client_id;
        }
        catch (...) {}
        return *this;
    }

    mqtt_client& mqtt_client::set_keep_alive(uint16_t seconds) noexcept {
        keep_alive_seconds_ = seconds;
        return *this;
    }

    mqtt_client& mqtt_client::on_message(message_callback cb) noexcept {
        try {
            on_message_cb_ = std::move(cb);
        }
        catch (...) {}
        return *this;
    }

    mqtt_client& mqtt_client::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) noexcept {
        try {
            logger_ = logger_ptr;
            tcp_client_.set_logger(logger_ptr);
        }
        catch (...) {}
        return *this;
    }

    mqtt_client& mqtt_client::set_reconnect_backoff(
        std::chrono::seconds initial_interval,
        std::chrono::seconds max_interval,
        double multiplier) noexcept {
        backoff_enabled_ = true;
        initial_backoff_ = (initial_interval.count() > 0) ? initial_interval : std::chrono::seconds(1);
        max_backoff_ = (max_interval >= initial_backoff_) ? max_interval : initial_backoff_;
        backoff_multiplier_ = (multiplier >= 1.0) ? multiplier : 2.0;
        current_backoff_ = initial_backoff_;
        return *this;
    }

    mqtt_client& mqtt_client::set_max_reconnect_duration(std::chrono::seconds max_duration) noexcept {
        max_reconnect_duration_ = max_duration;
        return *this;
    }

    bool mqtt_client::is_reconnecting() const noexcept {
        return is_reconnecting_;
    }

    std::chrono::seconds mqtt_client::current_backoff_interval() const noexcept {
        return current_backoff_;
    }

    topic_validation_result mqtt_client::validate_publish_topic(std::string_view topic) noexcept {
        if (topic.empty()) return { false, "topic name cannot be empty" };
        if (topic.size() > 65535) return { false, "topic name exceeds maximum length (65535 bytes)" };
        if (topic.front() == '$') return { false, "publish topic cannot start with reserved system prefix '$'" };

        for (char c : topic) {
            if (c == '\0') return { false, "topic cannot contain null characters (U+0000)" };
            if (c == '+' || c == '#') return { false, "publish topic cannot contain wildcards ('+' or '#')" };
        }
        return { true, "valid publish topic" };
    }

    topic_validation_result mqtt_client::validate_subscribe_topic(std::string_view topic) noexcept {
        if (topic.empty()) return { false, "topic filter cannot be empty" };
        if (topic.size() > 65535) return { false, "topic filter exceeds maximum length (65535 bytes)" };

        for (size_t i = 0; i < topic.size(); ++i) {
            char c = topic[i];
            if (c == '\0') return { false, "topic filter cannot contain null characters (U+0000)" };

            if (c == '#') {
                if (i != topic.size() - 1) return { false, "multi-level wildcard '#' must be the last character" };
                if (i > 0 && topic[i - 1] != '/') return { false, "multi-level wildcard '#' must be preceded by '/'" };
            }
            else if (c == '+') {
                if (i > 0 && topic[i - 1] != '/') return { false, "single-level wildcard '+' must be preceded by '/'" };
                if (i + 1 < topic.size() && topic[i + 1] != '/') return { false, "single-level wildcard '+' must be followed by '/'" };
            }
        }
        return { true, "valid subscribe topic filter" };
    }

    start_result mqtt_client::start(std::chrono::seconds reconnect_interval) noexcept {
        if (worker_running_) return { false, "client is already running" };
        if (host_.empty()) return { false, "broker host is not set" };
        if (port_ <= 0 || port_ > 65535) return { false, "invalid broker port (must be between 1 and 65535)" };
        if (client_id_.empty()) return { false, "client_id is not set" };

        if (reconnect_interval.count() > 0) {
            initial_backoff_ = reconnect_interval;
            if (max_backoff_ < initial_backoff_) max_backoff_ = initial_backoff_ * 10;
        }

        current_backoff_ = initial_backoff_;
        is_reconnecting_ = false;
        reconnect_start_time_ = std::chrono::steady_clock::time_point{};

        worker_running_ = true;
        try {
            worker_thread_ = std::thread(&mqtt_client::supervisor_loop, this);
        }
        catch (...) {
            worker_running_ = false;
            return { false, "failed to spawn supervisor worker thread" };
        }

        if (logger_) logger_->info("[mqtt_client] Started MQTT background worker targeting {}:{}", host_, port_);
        return { true, "success" };
    }

    void mqtt_client::stop() noexcept {
        if (!worker_running_.exchange(false)) return;

        if (is_mqtt_connected_) {
            uint8_t disc[] = { 0xE0, 0x00 };
            send_raw(disc, sizeof(disc));
            if (logger_) logger_->debug("[mqtt_client] Sent MQTT DISCONNECT packet");
        }

        is_mqtt_connected_ = false;
        is_reconnecting_ = false;

        tcp_client_.stop();

        try {
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
        catch (...) {}

        try {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            rx_buffer_.clear();
        }
        catch (...) {}

        if (logger_) logger_->info("[mqtt_client] MQTT client stopped successfully");
    }

    bool mqtt_client::is_connected() const noexcept {
        return is_mqtt_connected_ && tcp_client_.is_connected();
    }

    bool mqtt_client::publish(std::string_view topic, std::string_view payload) noexcept {
        if (!validate_publish_topic(topic)) {
            if (logger_) logger_->warn("[mqtt_client] publish rejected invalid topic: {}", topic);
            return false;
        }
        if (!is_connected()) return false;

        std::vector<uint8_t> packet;
        if (!build_publish_packet(topic, payload, packet)) return false;

        return send_raw(packet.data(), packet.size());
    }

    bool mqtt_client::subscribe(std::string_view topic) noexcept {
        if (!validate_subscribe_topic(topic)) {
            if (logger_) logger_->warn("[mqtt_client] subscribe rejected invalid topic: {}", topic);
            return false;
        }

        try {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscribed_topics_.insert(std::string(topic));
        }
        catch (...) {
            return false;
        }

        if (!is_connected()) return false;

        uint16_t pid = ++packet_id_counter_;
        std::vector<uint8_t> packet;
        if (!build_subscribe_packet(topic, pid, packet)) return false;

        return send_raw(packet.data(), packet.size());
    }

    bool mqtt_client::interruptible_sleep(std::chrono::milliseconds duration) noexcept {
        auto start = std::chrono::steady_clock::now();
        while (worker_running_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
            if (elapsed >= duration) return true;
            auto step = std::min(std::chrono::milliseconds(50), duration - elapsed);
            std::this_thread::sleep_for(step);
        }
        return false;
    }

    void mqtt_client::supervisor_loop() noexcept {
        while (worker_running_) {
            // 1. 정상 연결 및 핸드셰이크 유지 상태
            if (is_mqtt_connected_ && tcp_client_.is_connected()) {
                auto now = std::chrono::steady_clock::now();
                long long elapsed_sec = 0;
                {
                    std::lock_guard<std::mutex> lock(time_mutex_);
                    elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(now - last_sent_time_).count();
                }

                if (keep_alive_seconds_ > 0 && elapsed_sec >= static_cast<long long>(keep_alive_seconds_ * 0.75)) {
                    uint8_t ping_packet[] = { 0xC0, 0x00 };
                    if (send_raw(ping_packet, sizeof(ping_packet))) {
                        if (logger_) logger_->debug("[mqtt_client] Sent PINGREQ");
                    }
                }

                interruptible_sleep(std::chrono::milliseconds(200));
                continue;
            }

            if (!worker_running_) break;

            // 2. 끊김 감지 및 재연결 타이머 시작
            if (!is_reconnecting_) {
                is_reconnecting_ = true;
                reconnect_start_time_ = std::chrono::steady_clock::now();
                current_backoff_ = initial_backoff_;
            }

            // 3. 최대 재연결 시도 시간 검사 (0초가 아닌 경우에만 만료 체크)
            if (max_reconnect_duration_ > std::chrono::seconds(0)) {
                auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - reconnect_start_time_);

                if (total_elapsed >= max_reconnect_duration_) {
                    if (logger_) {
                        logger_->error("[mqtt_client] Reconnection failed: exceeded max allowed duration of {}s. Stopping retry.",
                            max_reconnect_duration_.count());
                    }
                    tcp_client_.stop();
                    is_reconnecting_ = false;
                    worker_running_ = false;
                    break;
                }
            }

            if (logger_) {
                logger_->warn("[mqtt_client] Reconnecting in {}s (Exponential Backoff)...", current_backoff_.count());
            }

            // 백오프 간격만큼 대기 (stop() 호출 시 즉시 탈출)
            if (!interruptible_sleep(current_backoff_)) {
                break;
            }

            // 4. 소켓 연결 시도
            tcp_client_.stop();
            tcp_client_.set_server(host_, static_cast<unsigned short>(port_), address_family_);
            tcp_client_.start(std::chrono::seconds(1));

            // 핸드셰이크(CONNACK) 완료 대기 (최대 3초)
            auto connect_wait_start = std::chrono::steady_clock::now();
            bool connected = false;
            while (worker_running_) {
                if (is_mqtt_connected_) {
                    connected = true;
                    break;
                }
                auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - connect_wait_start);
                if (waited >= std::chrono::milliseconds(3000)) {
                    break;
                }
                interruptible_sleep(std::chrono::milliseconds(100));
            }

            // 5. 연결 결과에 따른 백오프 갱신
            if (connected) {
                if (logger_) {
                    logger_->info("[mqtt_client] Reconnected successfully. Resetting backoff interval to {}s",
                        initial_backoff_.count());
                }
                is_reconnecting_ = false;
                current_backoff_ = initial_backoff_;
            }
            else {
                // 실패: tcp_client 닫고 백오프 시간 지수적 증가 (initial -> x2 -> x4 ... -> max)
                tcp_client_.stop();
                if (backoff_enabled_) {
                    long long next_sec = static_cast<long long>(current_backoff_.count() * backoff_multiplier_);
                    if (next_sec <= current_backoff_.count()) next_sec = current_backoff_.count() + 1;
                    current_backoff_ = std::min(std::chrono::seconds(next_sec), max_backoff_);
                }
            }
        }
    }

    void mqtt_client::setup_tcp_callbacks() noexcept {
        tcp_client_.set_on_connect([this]() noexcept {
            if (logger_) logger_->info("[mqtt_client] TCP connected. Sending MQTT CONNECT...");
            std::vector<uint8_t> packet;
            if (build_connect_packet(client_id_, keep_alive_seconds_, packet)) {
                send_raw(packet.data(), packet.size());
            }
            });

        tcp_client_.set_on_close([this]() noexcept {
            if (logger_) logger_->warn("[mqtt_client] TCP connection closed.");
            is_mqtt_connected_ = false;
            try {
                std::lock_guard<std::mutex> lock(rx_mutex_);
                rx_buffer_.clear();
            }
            catch (...) {}
            });

        tcp_client_.set_on_receive([this](const std::string& data) noexcept {
            handle_tcp_receive(data);
            });
    }

    void mqtt_client::handle_tcp_receive(const std::string& data) noexcept {
        try {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            rx_buffer_.insert(rx_buffer_.end(), data.begin(), data.end());
            parse_incoming_packets();
        }
        catch (...) {
            rx_buffer_.clear();
        }
    }

    void mqtt_client::parse_incoming_packets() noexcept {
        while (rx_buffer_.size() >= 2) {
            uint8_t header = rx_buffer_[0];
            uint8_t packet_type = header & 0xF0;

            size_t multiplier = 1;
            size_t remaining_length = 0;
            size_t len_byte_index = 1;
            bool len_complete = false;

            while (len_byte_index < rx_buffer_.size()) {
                uint8_t encoded_byte = rx_buffer_[len_byte_index];
                remaining_length += (encoded_byte & 127) * multiplier;
                multiplier *= 128;
                len_byte_index++;

                if ((encoded_byte & 128) == 0) {
                    len_complete = true;
                    break;
                }
                if (len_byte_index > 4) break;
            }

            if (!len_complete) return;

            size_t total_packet_size = len_byte_index + remaining_length;
            if (rx_buffer_.size() < total_packet_size) return;

            // CONNACK (0x20)
            if (packet_type == 0x20 && remaining_length >= 2) {
                uint8_t return_code = rx_buffer_[len_byte_index + 1];
                if (return_code == 0x00) {
                    is_mqtt_connected_ = true;
                    update_last_sent();
                    if (logger_) logger_->info("[mqtt_client] Handshake completed (CONNACK accepted).");
                    resubscribe_all();
                }
                else {
                    if (logger_) logger_->error("[mqtt_client] Handshake rejected with code: {}", return_code);
                }
            }
            // PINGRESP (0xD0)
            else if (packet_type == 0xD0) {
                if (logger_) logger_->debug("[mqtt_client] Received PINGRESP");
            }
            // SUBACK (0x90)
            else if (packet_type == 0x90) {
                if (logger_) logger_->debug("[mqtt_client] Received SUBACK");
            }
            // PUBLISH (0x30)
            else if (packet_type == 0x30) {
                parse_publish_packet(rx_buffer_.data() + len_byte_index, remaining_length);
            }

            rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + total_packet_size);
        }
    }

    void mqtt_client::parse_publish_packet(const uint8_t* payload_ptr, size_t length) noexcept {
        if (length < 2) return;

        size_t topic_len = (static_cast<size_t>(payload_ptr[0]) << 8) | payload_ptr[1];
        if (length < 2 + topic_len) return;

        std::string_view topic(reinterpret_cast<const char*>(payload_ptr + 2), topic_len);
        size_t payload_offset = 2 + topic_len;
        size_t payload_len = length - payload_offset;
        std::string_view payload(reinterpret_cast<const char*>(payload_ptr + payload_offset), payload_len);

        if (on_message_cb_) {
            try {
                on_message_cb_(topic, payload);
            }
            catch (...) {}
        }
    }

    void mqtt_client::resubscribe_all() noexcept {
        try {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            for (const auto& topic : subscribed_topics_) {
                uint16_t pid = ++packet_id_counter_;
                std::vector<uint8_t> packet;
                if (build_subscribe_packet(topic, pid, packet)) {
                    send_raw(packet.data(), packet.size());
                }
            }
        }
        catch (...) {}
    }

    bool mqtt_client::send_raw(const uint8_t* data, size_t length) noexcept {
        try {
            std::string binary_str(reinterpret_cast<const char*>(data), length);
            int sent = tcp_client_.send_data(binary_str);
            if (sent > 0) {
                update_last_sent();
                return true;
            }
        }
        catch (...) {}
        return false;
    }

    void mqtt_client::update_last_sent() noexcept {
        std::lock_guard<std::mutex> lock(time_mutex_);
        last_sent_time_ = std::chrono::steady_clock::now();
    }

    bool mqtt_client::encode_remaining_length(std::vector<uint8_t>& buffer, size_t length) noexcept {
        try {
            do {
                uint8_t byte = length % 128;
                length /= 128;
                if (length > 0) byte |= 0x80;
                buffer.push_back(byte);
            } while (length > 0);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool mqtt_client::append_string(std::vector<uint8_t>& buffer, std::string_view str) noexcept {
        try {
            buffer.push_back(static_cast<uint8_t>((str.size() >> 8) & 0xFF));
            buffer.push_back(static_cast<uint8_t>(str.size() & 0xFF));
            buffer.insert(buffer.end(), str.begin(), str.end());
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool mqtt_client::build_connect_packet(std::string_view client_id, uint16_t keep_alive, std::vector<uint8_t>& packet) noexcept {
        try {
            std::vector<uint8_t> payload;
            if (!append_string(payload, "MQTT")) return false;
            payload.push_back(0x04);
            payload.push_back(0x02);
            payload.push_back(static_cast<uint8_t>((keep_alive >> 8) & 0xFF));
            payload.push_back(static_cast<uint8_t>(keep_alive & 0xFF));
            if (!append_string(payload, client_id)) return false;

            packet.clear();
            packet.push_back(0x10);
            if (!encode_remaining_length(packet, payload.size())) return false;
            packet.insert(packet.end(), payload.begin(), payload.end());
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool mqtt_client::build_publish_packet(std::string_view topic, std::string_view payload, std::vector<uint8_t>& packet) noexcept {
        try {
            std::vector<uint8_t> body;
            if (!append_string(body, topic)) return false;
            body.insert(body.end(), payload.begin(), payload.end());

            packet.clear();
            packet.push_back(0x30);
            if (!encode_remaining_length(packet, body.size())) return false;
            packet.insert(packet.end(), body.begin(), body.end());
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool mqtt_client::build_subscribe_packet(std::string_view topic, uint16_t packet_id, std::vector<uint8_t>& packet) noexcept {
        try {
            std::vector<uint8_t> variable_header_and_payload;
            variable_header_and_payload.push_back(static_cast<uint8_t>((packet_id >> 8) & 0xFF));
            variable_header_and_payload.push_back(static_cast<uint8_t>(packet_id & 0xFF));

            if (!append_string(variable_header_and_payload, topic)) return false;
            variable_header_and_payload.push_back(0x00);

            packet.clear();
            packet.push_back(0x82);
            if (!encode_remaining_length(packet, variable_header_and_payload.size())) return false;
            packet.insert(packet.end(), variable_header_and_payload.begin(), variable_header_and_payload.end());
            return true;
        }
        catch (...) {
            return false;
        }
    }

} // namespace mino::network::mqtt
