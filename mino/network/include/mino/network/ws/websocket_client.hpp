#pragma once

#ifdef USE_CURL

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdint>

// libcurl 내부 타입 전방 선언
typedef void CURL;

namespace mino::network::ws {

    // 웹소켓 프레임 타입
    enum class ws_frame_type {
        text,
        binary,
        ping,
        pong,
        close
    };

    // 1. 클래스 상속 기반 이벤트 리스너 인터페이스
    class ws_event_listener {
    public:
        virtual ~ws_event_listener() = default;
        virtual void on_connected() {}
        virtual void on_message(const std::string& message, ws_frame_type type) {}
        virtual void on_binary(const std::vector<uint8_t>& data) {}
        virtual void on_disconnected(int status_code, const std::string& reason) {}
        virtual void on_error(const std::string& error_message) {}
    };

    // 2. 함수 콜백 컨테이너
    struct ws_callbacks {
        std::function<void()> on_connected;
        std::function<void(const std::string&, ws_frame_type)> on_message;
        std::function<void(const std::vector<uint8_t>&)> on_binary;
        std::function<void(int, const std::string&)> on_disconnected;
        std::function<void(const std::string&)> on_error;
    };

    // 공통 연결 설정 옵션
    struct ws_connection_config {
        long connect_timeout_sec = 10;
    };

    // 베이스 클라이언트 클래스
    class websocket_client_base {
    public:
        websocket_client_base();
        virtual ~websocket_client_base();

        void set_url(std::string url);
        void configure_connection(const ws_connection_config& config);
        void set_listener(ws_event_listener* listener);
        void set_callbacks(const ws_callbacks& callbacks);

        bool connect();
        void disconnect(int status_code = 1000, const std::string& reason = "normal closure");

        bool send_text(const std::string& message);
        bool send_binary(const std::vector<uint8_t>& buffer);

        bool is_connected() const;
        const std::string& get_url() const;

    protected:
        virtual void apply_custom_options() = 0;
        CURL* get_curl_handle();

    private:
        std::string target_url;
        ws_connection_config base_config;
        CURL* curl_handle;
        std::atomic<bool> is_running;
        std::thread worker_thread;

        ws_event_listener* event_listener;
        ws_callbacks cb_handlers;

        bool raw_send_all(const void* data, size_t length);
        bool raw_recv_exact(void* buffer, size_t length);
        bool perform_handshake(const std::string& http_url, const std::string& host);
        bool send_ws_frame(uint8_t opcode, const void* data, size_t length);
        void receive_loop();

        void dispatch_connected();
        void dispatch_message(const std::string& msg, ws_frame_type type);
        void dispatch_binary(const std::vector<uint8_t>& data);
        void dispatch_disconnected(int code, const std::string& reason);
        void dispatch_error(const std::string& err);

        std::string generate_ws_key_internal();
        std::string base64_encode_internal(const unsigned char* input, int length);
    };

    // ws:// 전용 파생 클래스
    class ws_client : public websocket_client_base {
    public:
        ws_client() = default;

    protected:
        void apply_custom_options() override;
    };

    // wss:// 전용 파생 클래스
    class wss_client : public websocket_client_base {
    public:
        wss_client();

        void configure_ssl(bool verify_peer, bool verify_host = true, const std::string& ca_path = "");

    protected:
        void apply_custom_options() override;

    private:
        bool ssl_verification_enabled;
        bool ssl_verify_host_enabled;
        std::string ca_bundle_path;
    };

} // namespace mino::network::ws

#endif // USE_CURL
