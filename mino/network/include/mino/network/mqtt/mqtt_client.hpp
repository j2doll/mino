#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <memory>
#include <algorithm>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::mqtt {

    using message_callback = std::function<void(std::string_view topic, std::string_view payload)>;

    struct start_result {
        bool success{ false };
        std::string_view message{};

        constexpr explicit operator bool() const noexcept { return success; }
    };

    struct topic_validation_result {
        bool valid{ false };
        std::string_view reason{};

        constexpr explicit operator bool() const noexcept { return valid; }
    };

    class mqtt_client {
    public:
        static constexpr std::chrono::seconds infinite_reconnect{ 0 };

        mqtt_client() noexcept;
        ~mqtt_client() noexcept;

        mqtt_client(const mqtt_client&) = delete;
        mqtt_client& operator=(const mqtt_client&) = delete;
        mqtt_client(mqtt_client&&) = delete;
        mqtt_client& operator=(mqtt_client&&) = delete;

        // -------------------------------------------------------------
        // 체이닝 설정 함수
        // -------------------------------------------------------------
        mqtt_client& set_broker(std::string_view host, int port = 1883) noexcept;
        mqtt_client& set_client_id(std::string_view client_id) noexcept;
        mqtt_client& set_keep_alive(uint16_t seconds) noexcept;
        mqtt_client& on_message(message_callback cb) noexcept;
        mqtt_client& set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr = nullptr) noexcept;

        // 사용자 인증 정보 설정 (사용자명, 비밀번호)
        mqtt_client& set_credentials(std::string_view username, std::string_view password = "") noexcept;
        mqtt_client& clear_credentials() noexcept;

        // 지수 백오프(Exponential Backoff) 재연결 설정
        mqtt_client& set_reconnect_backoff(
            std::chrono::seconds initial_interval,
            std::chrono::seconds max_interval,
            double multiplier = 2.0) noexcept;

        // 최대 재연결 시도 시간 설정 (0초 또는 infinite_reconnect 전달 시 무한 재시도)
        mqtt_client& set_max_reconnect_duration(std::chrono::seconds max_duration) noexcept;

        // -------------------------------------------------------------
        // 토픽 유효성 검증
        // -------------------------------------------------------------
        static topic_validation_result validate_publish_topic(std::string_view topic) noexcept;
        static topic_validation_result validate_subscribe_topic(std::string_view topic) noexcept;

        // -------------------------------------------------------------
        // 동작 제어
        // -------------------------------------------------------------
        start_result start(std::chrono::seconds reconnect_interval = std::chrono::seconds(1)) noexcept;
        void stop() noexcept;
        bool is_connected() const noexcept;
        bool is_reconnecting() const noexcept;
        std::chrono::seconds current_backoff_interval() const noexcept;

        bool publish(std::string_view topic, std::string_view payload) noexcept;
        bool subscribe(std::string_view topic) noexcept;

    private:
        mino::network::tcp::tcp_client tcp_client_;
        std::string host_;
        int port_;
        int address_family_;
        std::string client_id_;
        uint16_t keep_alive_seconds_;

        // 사용자 계정 인증 필드
        std::string username_{};
        std::string password_{};
        bool has_credentials_{ false };

        // 재연결 및 백오프 파라미터
        bool backoff_enabled_{ true };
        std::chrono::seconds initial_backoff_{ 1 };
        std::chrono::seconds max_backoff_{ 30 };
        double backoff_multiplier_{ 2.0 };
        std::chrono::seconds current_backoff_{ 1 };
        std::chrono::seconds max_reconnect_duration_{ 0 };
        std::atomic<bool> is_reconnecting_{ false };
        std::chrono::steady_clock::time_point reconnect_start_time_{};

        std::atomic<bool> is_mqtt_connected_{ false };
        std::atomic<bool> worker_running_{ false };
        std::thread worker_thread_;

        std::chrono::steady_clock::time_point last_sent_time_;
        std::mutex time_mutex_;

        message_callback on_message_cb_;
        std::shared_ptr<mino::core::log::tinylog::logger> logger_{ nullptr };

        std::mutex subscriptions_mutex_;
        std::unordered_set<std::string> subscribed_topics_;
        std::atomic<uint16_t> packet_id_counter_;

        std::mutex rx_mutex_;
        std::vector<uint8_t> rx_buffer_;

        void setup_tcp_callbacks() noexcept;
        void handle_tcp_receive(const std::string& data) noexcept;
        void parse_incoming_packets() noexcept;
        void parse_publish_packet(const uint8_t* payload_ptr, size_t length) noexcept;
        void resubscribe_all() noexcept;
        void supervisor_loop() noexcept;
        bool interruptible_sleep(std::chrono::milliseconds duration) noexcept;

        bool send_raw(const uint8_t* data, size_t length) noexcept;
        void update_last_sent() noexcept;

        static bool encode_remaining_length(std::vector<uint8_t>& buffer, size_t length) noexcept;
        static bool append_string(std::vector<uint8_t>& buffer, std::string_view str) noexcept;

        bool build_connect_packet(std::string_view client_id, uint16_t keep_alive, std::vector<uint8_t>& packet) noexcept;
        bool build_publish_packet(std::string_view topic, std::string_view payload, std::vector<uint8_t>& packet) noexcept;
        bool build_subscribe_packet(std::string_view topic, uint16_t packet_id, std::vector<uint8_t>& packet) noexcept;
    };

} // namespace mino::network::mqtt
