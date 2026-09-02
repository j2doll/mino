#pragma once

#ifdef USE_CURL

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <cstdint>
#include <unordered_map>

#include <curl/curl.h>

namespace mino::network::socket_io {

    // -------------------------------------------------------------
    // 이벤트 리스너 인터페이스
    // -------------------------------------------------------------
    class socketio_event_listener {
    public:
        virtual ~socketio_event_listener() = default;
        virtual void on_open() {}
        virtual void on_close(const std::string& reason) {}
        virtual void on_event(const std::string& event_payload) {}
        virtual void on_error(const std::string& error_message) {}
    };

    // -------------------------------------------------------------
    // 베이스 클래스: socketio_client_base
    // -------------------------------------------------------------
    class socketio_client_base {
    public:
        socketio_client_base();
        virtual ~socketio_client_base();

        void set_server_endpoint(std::string host, int port);
        void set_connect_timeout_seconds(long timeout_sec);
        void set_event_listener(std::shared_ptr<socketio_event_listener> listener);

        // 네임스페이스 설정 (예: "/", "/chat", "/admin")
        void set_namespace(std::string ns);
        const std::string& get_namespace() const { return target_namespace; }

        void on_open(std::function<void()> handler);
        void on_close(std::function<void(const std::string&)> handler);
        void on_event(std::function<void(const std::string&)> handler);
        void on_error(std::function<void(const std::string&)> handler);

        // 1) 특정 이벤트명만 선택하여 수신
        void on(const std::string& event_name, std::function<void(const std::string&)> handler);

        // 2) 모든 이벤트를 일괄 수신 (오버로딩)
        void on(std::function<void(const std::string&)> handler) {
            on_event(std::move(handler));
        }

        // 재연결을 위해 소켓 자원을 정리하고 핸들을 새로 발급
        void reset();

        bool connect();
        void emit(const std::string& event_name, const std::string& json_payload);

        // Room 입장 및 퇴장 편의 함수 (빈 문자열 입력 시 자동 무시)
        void join_room(const std::string& room_name);
        void leave_room(const std::string& room_name);

        void run_event_loop();
        void close(const std::string& reason = "Normal closure");

    protected:
        virtual std::string get_engine_io_version() const = 0;
        virtual void on_handshake_open(const std::string& payload) = 0;
        virtual void handle_ping() = 0;
        virtual void handle_pong() = 0;
        virtual void on_heartbeat_tick() {}

        void send_engineio_packet(const std::string& packet_data);
        void notify_error(const std::string& err_msg);

        std::string target_namespace{ "/" };

    private:
        void dispatch_packet(const std::string& packet);
        void handle_socketio_message(const std::string& message);
        bool raw_send(const std::string& data);

        CURL* curl_handle{ nullptr };
        std::string server_host;
        int server_port{ 0 };
        long connect_timeout_seconds{ 5 };
        bool is_running{ false };

        std::shared_ptr<socketio_event_listener> custom_listener;
        std::function<void()> on_open_callback;
        std::function<void(const std::string&)> on_close_callback;
        std::function<void(const std::string&)> on_event_callback;
        std::function<void(const std::string&)> on_error_callback;

        std::unordered_map<std::string, std::function<void(const std::string&)>> event_handlers;
    };

    // -------------------------------------------------------------
    // 파생 클래스: Socket.IO v0.9 ~ v1.x (Engine.IO v2)
    // -------------------------------------------------------------
    class socketio_client_v1 : public socketio_client_base {
    public:
        socketio_client_v1() = default;

    protected:
        std::string get_engine_io_version() const override;
        void on_handshake_open(const std::string& payload) override;
        void handle_ping() override;
        void handle_pong() override;
        void on_heartbeat_tick() override;

    private:
        std::chrono::steady_clock::time_point last_ping_time{ std::chrono::steady_clock::now() };
    };

    // -------------------------------------------------------------
    // 파생 클래스: Socket.IO v2.x (Engine.IO v3)
    // -------------------------------------------------------------
    class socketio_client_v2 : public socketio_client_base {
    public:
        socketio_client_v2() = default;

    protected:
        std::string get_engine_io_version() const override;
        void on_handshake_open(const std::string& payload) override;
        void handle_ping() override;
        void handle_pong() override;
        void on_heartbeat_tick() override;

    private:
        std::chrono::steady_clock::time_point last_ping_time{ std::chrono::steady_clock::now() };
    };

    // -------------------------------------------------------------
    // 파생 클래스: Socket.IO v3.x ~ v4.x (Engine.IO v4)
    // -------------------------------------------------------------
    class socketio_client_v4 : public socketio_client_base {
    public:
        socketio_client_v4() = default;

    protected:
        std::string get_engine_io_version() const override;
        void on_handshake_open(const std::string& payload) override;
        void handle_ping() override;
        void handle_pong() override;
    };

    // -------------------------------------------------------------
    // 팩토리 헬퍼 선언
    // -------------------------------------------------------------
    enum class socketio_version { v1, v2, v4 };

    std::unique_ptr<socketio_client_base> create_socketio_client(socketio_version ver);

} // namespace mino::network::socket_io

#endif // USE_CURL
