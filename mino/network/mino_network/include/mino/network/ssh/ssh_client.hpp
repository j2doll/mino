#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/crypt/ssh_crypto.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/ssh/ssh_buffer.hpp"

namespace mino::network::ssh {

    enum class session_state {
        disconnected,
        connecting,
        handshaking,
        authenticated
    };

    struct reconnect_policy {
        bool enabled{ true };
        int max_retries{ -1 };
        std::chrono::milliseconds initial_delay{ 1000 };
        std::chrono::milliseconds max_delay{ 30000 };
        double backoff_multiplier{ 2.0 };
    };

    enum class rx_event_type {
        stdout_data,
        stderr_data
    };

    struct rx_event {
        rx_event_type type;
        uint32_t channel_id;
        std::string data;
    };

    class ssh_client {
    public:
        using data_callback = std::function<void(uint32_t channel_id, const std::string& data)>;
        using event_callback = std::function<void()>;
        using disconnect_callback = std::function<void(uint32_t reason, const std::string& msg)>;
        using host_key_verifier = std::function<bool(const std::string& host, const std::string& fingerprint)>;
        using state_callback = std::function<void(session_state state)>;

    private:
        socket_t sock_fd_;

        std::string host_{ "127.0.0.1" };
        uint16_t port_{ 22 };
        std::string username_;
        std::string password_;

        reconnect_policy policy_;

        std::atomic<session_state> state_{ session_state::disconnected };
        std::atomic<bool> stop_flag_{ false };
        std::atomic<bool> keys_activated_{ false };

        uint32_t seq_out_{ 0 };
        uint32_t seq_in_{ 0 };

        std::string server_banner_;
        const std::string client_banner_{ "SSH-2.0-MinoSSH_1.0\r\n" };

        std::vector<uint8_t> client_kexinit_payload_;
        std::vector<uint8_t> server_kexinit_payload_;

        mino::core::crypt::aes128_ctr enc_out_;
        mino::core::crypt::aes128_ctr enc_in_;
        std::array<uint8_t, 32> mac_key_out_{};
        std::array<uint8_t, 32> mac_key_in_{};
        std::vector<uint8_t> session_id_;

        uint32_t remote_channel_id_{ 0 };
        std::mutex send_mutex_;

        std::queue<rx_event> rx_queue_;
        std::mutex rx_queue_mutex_;
        std::condition_variable rx_cv_;
        std::thread rx_dispatch_thread_;
        std::thread worker_thread_;

        // Binary channel RX stream buffer (for SFTP)
        std::vector<uint8_t> channel_rx_buf_;
        std::mutex channel_rx_mutex_;
        std::condition_variable channel_rx_cv_;

        // Subsystem request synchronization
        std::atomic<bool> channel_req_done_{ false };
        std::atomic<bool> channel_req_success_{ false };
        std::mutex channel_req_mutex_;
        std::condition_variable channel_req_cv_;

        std::shared_ptr<mino::core::log::tinylog::logger> logger_;

        data_callback on_stdout_cb_;
        data_callback on_stderr_cb_;
        event_callback on_authenticated_cb_;
        disconnect_callback on_disconnect_cb_;
        host_key_verifier host_key_verifier_cb_;
        state_callback on_state_changed_cb_;

        void set_state(session_state s);

        void send_raw(const uint8_t* buf, size_t len);
        void recv_raw(uint8_t* buf, size_t len);

        std::vector<uint8_t> derive_key(const std::vector<uint8_t>& K, const std::array<uint8_t, 32>& H, char letter, size_t need_len);
        void reset_session_state();
        void clear_rx_queue();
        bool establish_tcp();
        void do_handshake();
        void send_packet(const ssh_buffer& payload_buf);
        ssh_buffer recv_packet();

        void receive_loop();
        void rx_dispatch_loop();
        void connection_supervisor();

    public:
        ssh_client();
        ~ssh_client();

        void set_server(const std::string& host, uint16_t port);
        void set_host(const std::string& host);
        void set_port(uint16_t port);
        void set_user(const std::string& username);
        void set_password(const std::string& password);
        void set_reconnect_policy(const reconnect_policy& policy);
        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr);

        void set_on_stdout(data_callback cb);
        void set_on_stderr(data_callback cb);
        void set_on_authenticated(event_callback cb);
        void set_on_disconnect(disconnect_callback cb);
        void set_host_key_verifier(host_key_verifier cb);
        void set_on_state_changed(state_callback cb);

        session_state get_state() const;
        bool is_connected() const;
        bool is_authenticated() const;

        bool start();
        void stop();

        // Synchronous interface for FTP/SFTP clients
        bool connect_sync(const std::string& host, uint16_t port,
            const std::string& username, const std::string& password,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));

        bool request_subsystem(const std::string& subsystem,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

        void send_window_adjust(uint32_t bytes_to_add);
        void send_channel_data(const uint8_t* data, size_t len);
        void send_channel_data(const std::string& data);
        bool recv_channel_exact(uint8_t* out, size_t len,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));

        void execute_command(const std::string& cmd);
    };

} // namespace mino::network::ssh
