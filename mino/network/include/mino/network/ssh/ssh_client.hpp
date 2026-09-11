#pragma once

#include <string>
#include <functional>
#include <optional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <libssh2.h>

#include "mino/network/tcp/tcp_client.hpp"
#include "ssh_common.hpp"

namespace mino::network::ssh2 {

    enum class auth_type {
        password,
        public_key
    };

    struct ssh_credentials {
        auth_type type{ auth_type::password };
        std::string username;
        std::string password;
        std::string public_key_path;
        std::string private_key_path;
        std::string passphrase;
    };

    struct reconnect_config {
        std::chrono::milliseconds initial_interval{ 1000 };      // 최초 재시도 대기 시간
        std::chrono::milliseconds max_interval{ 60000 };          // 백오프 상한선 (최대 대기 시간)
        double backoff_multiplier{ 2.0 };                        // 지수 증가 배수
        std::optional<std::chrono::milliseconds> max_duration{ std::nullopt }; // nullopt일 경우 무한대 시도
    };

    class ssh_client : public mino::network::tcp::tcp_client {
    public:
        using connect_callback = std::function<void()>;
        using disconnect_callback = std::function<void(const std::string& reason)>;
        using error_callback = std::function<void(int error_code, const std::string& message)>;
        using receive_callback = std::function<void(const std::string& data)>;

        ssh_client();
        ~ssh_client();

        void set_on_connect(connect_callback cb);
        void set_on_disconnect(disconnect_callback cb);
        void set_on_error(error_callback cb);
        void set_on_receive(receive_callback cb);

        void set_credentials(const ssh_credentials& creds);
        void set_expected_fingerprint(const std::string& fingerprint,
            fingerprint_type type = fingerprint_type::sha256);

        std::string get_server_fingerprint_hex(fingerprint_type type = fingerprint_type::sha256) const;
        std::string get_server_fingerprint_base64(fingerprint_type type = fingerprint_type::sha256) const;

        bool connect_ssh(const std::string& host, unsigned short port);
        void disconnect_ssh();

        bool start_auto_reconnect(const std::string& host,
            unsigned short port,
            const reconnect_config& config = {});
        void stop_auto_reconnect();

        bool send_json(const std::string& json_payload);
        std::optional<std::string> execute_command(const std::string& command);

    private:
        bool connect_ssh_internal();
        bool perform_handshake();
        bool verify_fingerprint();
        bool authenticate();
        bool open_channel_internal();
        void cleanup_ssh_session();
        void worker_loop();

        void notify_error(int code, const std::string& message);
        void notify_disconnect(const std::string& reason);

        ssh_credentials credentials_;
        std::string expected_fingerprint_;
        fingerprint_type expected_fingerprint_type_{ fingerprint_type::sha256 };

        LIBSSH2_SESSION* session_{ nullptr };
        LIBSSH2_CHANNEL* channel_{ nullptr };
        std::mutex ssh_mutex_;

        reconnect_config reconnect_cfg_;
        std::atomic<bool> auto_reconnect_enabled_{ false };
        std::thread worker_thread_;
        std::condition_variable reconnect_cv_;
        std::mutex reconnect_cv_mutex_;

        connect_callback on_ssh_connect_;
        disconnect_callback on_ssh_disconnect_;
        error_callback on_ssh_error_;
        receive_callback on_ssh_receive_;

        std::string recv_stream_buffer_;
    };

} // namespace mino::network::ssh
