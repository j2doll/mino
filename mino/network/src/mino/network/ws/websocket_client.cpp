#ifdef USE_CURL

#include <curl/curl.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "mino/network/ws/websocket_client.hpp"

#include <random>
#include <sstream>
#include <chrono>

namespace mino::network::ws {

    // -----------------------------------------------------------------------------
    // websocket_client_base 구현
    // -----------------------------------------------------------------------------

    websocket_client_base::websocket_client_base()
        : curl_handle(nullptr),
        is_running(false),
        event_listener(nullptr) {
    }

    websocket_client_base::~websocket_client_base() {
        disconnect();
    }

    void websocket_client_base::set_url(std::string url) {
        if (is_running) {
            dispatch_error("cannot change url while connected");
            return;
        }
        target_url = std::move(url);
    }

    void websocket_client_base::configure_connection(const ws_connection_config& config) {
        if (is_running) {
            dispatch_error("cannot change configuration while connected");
            return;
        }
        base_config = config;
    }

    void websocket_client_base::set_listener(ws_event_listener* listener) {
        event_listener = listener;
    }

    void websocket_client_base::set_callbacks(const ws_callbacks& callbacks) {
        cb_handlers = callbacks;
    }

    bool websocket_client_base::connect() {
        if (is_running) return false;
        if (target_url.empty()) {
            dispatch_error("target url is not set");
            return false;
        }

        curl_handle = curl_easy_init();
        if (!curl_handle) {
            dispatch_error("failed to initialize curl handle");
            return false;
        }

        std::string http_url = target_url;
        if (http_url.rfind("ws://", 0) == 0) {
            http_url.replace(0, 5, "http://");
        }
        else if (http_url.rfind("wss://", 0) == 0) {
            http_url.replace(0, 6, "https://");
        }

        std::string host_name;
        size_t start = http_url.find("://");
        if (start != std::string::npos) {
            size_t slash_pos = http_url.find('/', start + 3);
            host_name = (slash_pos == std::string::npos)
                ? http_url.substr(start + 3)
                : http_url.substr(start + 3, slash_pos - (start + 3));
        }

        curl_easy_setopt(curl_handle, CURLOPT_URL, http_url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, base_config.connect_timeout_sec);

        apply_custom_options();

        CURLcode res = curl_easy_perform(curl_handle);
        if (res != CURLE_OK) {
            dispatch_error(std::string("tcp or tls connection failed: ") + curl_easy_strerror(res));
            curl_easy_cleanup(curl_handle);
            curl_handle = nullptr;
            return false;
        }

        if (!perform_handshake(http_url, host_name)) {
            dispatch_error("websocket handshake failed");
            curl_easy_cleanup(curl_handle);
            curl_handle = nullptr;
            return false;
        }

        is_running = true;
        dispatch_connected();

        worker_thread = std::thread(&websocket_client_base::receive_loop, this);
        return true;
    }

    void websocket_client_base::disconnect(int status_code, const std::string& reason) {
        if (!is_running) return;

        is_running = false;

        std::vector<uint8_t> payload(2);
        payload[0] = static_cast<uint8_t>((status_code >> 8) & 0xFF);
        payload[1] = static_cast<uint8_t>(status_code & 0xFF);
        payload.insert(payload.end(), reason.begin(), reason.end());
        send_ws_frame(0x08, payload.data(), payload.size());

        if (worker_thread.joinable()) {
            worker_thread.join();
        }

        if (curl_handle) {
            curl_easy_cleanup(curl_handle);
            curl_handle = nullptr;
        }

        dispatch_disconnected(status_code, reason);
    }

    bool websocket_client_base::send_text(const std::string& message) {
        return send_ws_frame(0x01, message.data(), message.size());
    }

    bool websocket_client_base::send_binary(const std::vector<uint8_t>& buffer) {
        return send_ws_frame(0x02, buffer.data(), buffer.size());
    }

    bool websocket_client_base::is_connected() const {
        return is_running;
    }

    const std::string& websocket_client_base::get_url() const {
        return target_url;
    }

    CURL* websocket_client_base::get_curl_handle() {
        return curl_handle;
    }

    bool websocket_client_base::raw_send_all(const void* data, size_t length) {
        const char* ptr = static_cast<const char*>(data);
        size_t total_sent = 0;
        while (total_sent < length) {
            size_t sent = 0;
            CURLcode res = curl_easy_send(curl_handle, ptr + total_sent, length - total_sent, &sent);
            if (res == CURLE_OK) {
                total_sent += sent;
            }
            else if (res == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else {
                return false;
            }
        }
        return true;
    }

    bool websocket_client_base::raw_recv_exact(void* buffer, size_t length) {
        char* ptr = static_cast<char*>(buffer);
        size_t total_read = 0;
        while (total_read < length && is_running) {
            size_t nread = 0;
            CURLcode res = curl_easy_recv(curl_handle, ptr + total_read, length - total_read, &nread);
            if (res == CURLE_OK) {
                total_read += nread;
            }
            else if (res == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else {
                return false;
            }
        }
        return total_read == length;
    }

    bool websocket_client_base::perform_handshake(const std::string& http_url, const std::string& host) {
        size_t path_pos = http_url.find('/', http_url.find("://") + 3);
        std::string path = (path_pos == std::string::npos) ? "/" : http_url.substr(path_pos);

        std::string ws_key = generate_ws_key_internal();
        std::ostringstream req;
        req << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << ws_key << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n\r\n";

        std::string req_str = req.str();
        if (!raw_send_all(req_str.data(), req_str.size())) {
            return false;
        }

        std::string res_header;
        char ch;
        while (res_header.find("\r\n\r\n") == std::string::npos) {
            size_t nread = 0;
            CURLcode res = curl_easy_recv(curl_handle, &ch, 1, &nread);
            if (res == CURLE_OK && nread == 1) {
                res_header += ch;
            }
            else if (res == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else {
                return false;
            }
        }

        return res_header.find("101") != std::string::npos;
    }

    bool websocket_client_base::send_ws_frame(uint8_t opcode, const void* data, size_t length) {
        if (!curl_handle || !is_running) return false;

        std::vector<uint8_t> frame;
        frame.push_back(0x80 | (opcode & 0x0F));

        if (length <= 125) {
            frame.push_back(0x80 | static_cast<uint8_t>(length));
        }
        else if (length <= 65535) {
            frame.push_back(0x80 | 126);
            frame.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>(length & 0xFF));
        }
        else {
            frame.push_back(0x80 | 127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<uint8_t>((length >> (i * 8)) & 0xFF));
            }
        }

        uint8_t mask[4];
        std::random_device rd;
        for (int i = 0; i < 4; ++i) {
            mask[i] = static_cast<uint8_t>(rd() % 256);
        }
        frame.insert(frame.end(), mask, mask + 4);

        const uint8_t* payload = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < length; ++i) {
            frame.push_back(payload[i] ^ mask[i % 4]);
        }

        return raw_send_all(frame.data(), frame.size());
    }

    void websocket_client_base::receive_loop() {
        while (is_running) {
            uint8_t header[2];
            if (!raw_recv_exact(header, 2)) {
                if (is_running) {
                    dispatch_error("connection broken while reading frame header");
                    is_running = false;
                }
                break;
            }

            uint8_t opcode = header[0] & 0x0F;
            bool is_masked = (header[1] & 0x80) != 0;
            uint64_t payload_len = header[1] & 0x7F;

            if (payload_len == 126) {
                uint8_t ext[2];
                if (!raw_recv_exact(ext, 2)) break;
                payload_len = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
            }
            else if (payload_len == 127) {
                uint8_t ext[8];
                if (!raw_recv_exact(ext, 8)) break;
                payload_len = 0;
                for (int i = 0; i < 8; ++i) {
                    payload_len = (payload_len << 8) | ext[i];
                }
            }

            uint8_t mask_key[4] = { 0 };
            if (is_masked && !raw_recv_exact(mask_key, 4)) break;

            std::vector<uint8_t> payload(payload_len);
            if (payload_len > 0 && !raw_recv_exact(payload.data(), payload_len)) break;

            if (is_masked) {
                for (size_t i = 0; i < payload_len; ++i) {
                    payload[i] ^= mask_key[i % 4];
                }
            }

            if (opcode == 0x01) {
                std::string msg(payload.begin(), payload.end());
                dispatch_message(msg, ws_frame_type::text);
            }
            else if (opcode == 0x02) {
                dispatch_binary(payload);
            }
            else if (opcode == 0x08) {
                is_running = false;
                int code = 1000;
                std::string reason = "";
                if (payload.size() >= 2) {
                    code = (payload[0] << 8) | payload[1];
                    reason = std::string(payload.begin() + 2, payload.end());
                }
                dispatch_disconnected(code, reason);
                break;
            }
            else if (opcode == 0x09) {
                send_ws_frame(0x0A, payload.data(), payload.size());
            }
        }
    }

    void websocket_client_base::dispatch_connected() {
        if (event_listener) event_listener->on_connected();
        if (cb_handlers.on_connected) cb_handlers.on_connected();
    }

    void websocket_client_base::dispatch_message(const std::string& msg, ws_frame_type type) {
        if (event_listener) event_listener->on_message(msg, type);
        if (cb_handlers.on_message) cb_handlers.on_message(msg, type);
    }

    void websocket_client_base::dispatch_binary(const std::vector<uint8_t>& data) {
        if (event_listener) event_listener->on_binary(data);
        if (cb_handlers.on_binary) cb_handlers.on_binary(data);
    }

    void websocket_client_base::dispatch_disconnected(int code, const std::string& reason) {
        if (event_listener) event_listener->on_disconnected(code, reason);
        if (cb_handlers.on_disconnected) cb_handlers.on_disconnected(code, reason);
    }

    void websocket_client_base::dispatch_error(const std::string& err) {
        if (event_listener) event_listener->on_error(err);
        if (cb_handlers.on_error) cb_handlers.on_error(err);
    }

    std::string websocket_client_base::base64_encode_internal(const unsigned char* input, int length) {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO* bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, input, length);
        BIO_flush(bio);

        BUF_MEM* buffer_ptr = nullptr;
        BIO_get_mem_ptr(bio, &buffer_ptr);

        std::string result(buffer_ptr->data, buffer_ptr->length);
        BIO_free_all(bio);
        return result;
    }

    std::string websocket_client_base::generate_ws_key_internal() {
        unsigned char bytes[16];
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);
        for (int i = 0; i < 16; ++i) {
            bytes[i] = static_cast<unsigned char>(dis(gen));
        }
        return base64_encode_internal(bytes, 16);
    }

    // -----------------------------------------------------------------------------
    // ws_client 구현
    // -----------------------------------------------------------------------------

    void ws_client::apply_custom_options() {
        // ws:// 기본 통신 설정
    }

    // -----------------------------------------------------------------------------
    // wss_client 구현
    // -----------------------------------------------------------------------------

    wss_client::wss_client()
        : ssl_verification_enabled(true),
        ssl_verify_host_enabled(true) {
    }

    void wss_client::configure_ssl(bool verify_peer, bool verify_host, const std::string& ca_path) {
        ssl_verification_enabled = verify_peer;
        ssl_verify_host_enabled = verify_host;
        ca_bundle_path = ca_path;
    }

    void wss_client::apply_custom_options() {
        CURL* handle = get_curl_handle();
        if (!handle) return;

        if (ssl_verification_enabled) {
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, ssl_verify_host_enabled ? 2L : 0L);
            if (!ca_bundle_path.empty()) {
                curl_easy_setopt(handle, CURLOPT_CAINFO, ca_bundle_path.c_str());
            }
        }
        else {
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        }
    }



} // namespace mino::network::ws

#endif // USE_CURL
