#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>

#include "mino/core/encoding/base64.hpp"
#include "mino/network/ssh/ssh_client.hpp"

namespace mino::network::ssh {

    ssh_client::ssh_client()
        : sock_fd_(
#ifdef _WIN32
            INVALID_SOCKET
#else
            - 1
#endif
        ) {
    }

    ssh_client::~ssh_client() {
        stop();
    }

    void ssh_client::set_server(const std::string& host, uint16_t port) {
        host_ = host;
        port_ = port;
    }

    void ssh_client::set_host(const std::string& host) { host_ = host; }
    void ssh_client::set_port(uint16_t port) { port_ = port; }
    void ssh_client::set_user(const std::string& username) { username_ = username; }
    void ssh_client::set_password(const std::string& password) { password_ = password; }
    void ssh_client::set_reconnect_policy(const reconnect_policy& policy) { policy_ = policy; }
    void ssh_client::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger_ = std::move(logger_ptr);
    }

    void ssh_client::set_on_stdout(data_callback cb) { on_stdout_cb_ = std::move(cb); }
    void ssh_client::set_on_stderr(data_callback cb) { on_stderr_cb_ = std::move(cb); }
    void ssh_client::set_on_authenticated(event_callback cb) { on_authenticated_cb_ = std::move(cb); }
    void ssh_client::set_on_disconnect(disconnect_callback cb) { on_disconnect_cb_ = std::move(cb); }
    void ssh_client::set_host_key_verifier(host_key_verifier cb) { host_key_verifier_cb_ = std::move(cb); }
    void ssh_client::set_on_state_changed(state_callback cb) { on_state_changed_cb_ = std::move(cb); }

    session_state ssh_client::get_state() const { return state_.load(); }
    bool ssh_client::is_connected() const { return state_.load() != session_state::disconnected; }
    bool ssh_client::is_authenticated() const { return state_.load() == session_state::authenticated; }

    void ssh_client::set_state(session_state s) {
        state_.store(s);
        if (on_state_changed_cb_) on_state_changed_cb_(s);
    }

    void ssh_client::clear_rx_queue() {
        std::lock_guard<std::mutex> lock(rx_queue_mutex_);
        std::queue<rx_event> empty_q;
        std::swap(rx_queue_, empty_q);
    }

    void ssh_client::send_raw(const uint8_t* buf, size_t len) {
        size_t sent = 0;
        while (sent < len) {
#ifdef _WIN32
            int n = ::send(sock_fd_, reinterpret_cast<const char*>(buf + sent), static_cast<int>(len - sent), 0);
#else
            ssize_t n = ::send(sock_fd_, buf + sent, len - sent, MSG_NOSIGNAL);
#endif
            if (n <= 0) throw std::runtime_error("소켓 데이터 송신 실패");
            sent += n;
        }
    }

    void ssh_client::recv_raw(uint8_t* buf, size_t len) {
        size_t recvd = 0;
        while (recvd < len) {
#ifdef _WIN32
            int n = ::recv(sock_fd_, reinterpret_cast<char*>(buf + recvd), static_cast<int>(len - recvd), 0);
#else
            ssize_t n = ::recv(sock_fd_, buf + recvd, len - recvd, 0);
#endif
            if (n <= 0) throw std::runtime_error("원격 호스트가 소켓을 닫았습니다");
            recvd += n;
        }
    }

    std::vector<uint8_t> ssh_client::derive_key(const std::vector<uint8_t>& K, const std::array<uint8_t, 32>& H, char letter, size_t need_len) {
        ssh_buffer b;
        b.write_mpint(K.data(), K.size());
        b.write_raw(H.data(), 32);
        b.write_byte(static_cast<uint8_t>(letter));
        b.write_raw(session_id_.data(), session_id_.size());

        auto k1 = mino::core::crypt::sha256::hash(b.data().data(), b.size());
        std::vector<uint8_t> result(k1.begin(), k1.end());

        while (result.size() < need_len) {
            ssh_buffer b_ext;
            b_ext.write_mpint(K.data(), K.size());
            b_ext.write_raw(H.data(), 32);
            b_ext.write_raw(result.data(), result.size());
            auto kn = mino::core::crypt::sha256::hash(b_ext.data().data(), b_ext.size());
            result.insert(result.end(), kn.begin(), kn.end());
        }
        result.resize(need_len);
        return result;
    }

    void ssh_client::reset_session_state() {
        keys_activated_ = false;
        seq_out_ = 0;
        seq_in_ = 0;
        session_id_.clear();
        server_banner_.clear();
        client_kexinit_payload_.clear();
        server_kexinit_payload_.clear();

#ifdef _WIN32
        if (sock_fd_ != INVALID_SOCKET) { ::closesocket(sock_fd_); sock_fd_ = INVALID_SOCKET; }
#else
        if (sock_fd_ >= 0) { ::close(sock_fd_); sock_fd_ = -1; }
#endif

        set_state(session_state::disconnected);
        clear_rx_queue();
    }

    bool ssh_client::establish_tcp() {
        set_state(session_state::connecting);
        struct addrinfo hints {}, * res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        std::string p_str = std::to_string(port_);

        if (getaddrinfo(host_.c_str(), p_str.c_str(), &hints, &res) != 0) {
            if (logger_) logger_->error("[ssh_client] getaddrinfo 실패: {}:{}", host_, port_);
            return false;
        }

        bool ok = false;
        for (auto* p = res; p != nullptr; p = p->ai_next) {
            sock_fd_ = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#ifdef _WIN32
            if (sock_fd_ == INVALID_SOCKET) continue;
#else
            if (sock_fd_ < 0) continue;
#endif
            if (::connect(sock_fd_, p->ai_addr, static_cast<socklen_t>(p->ai_addrlen)) == 0) {
                ok = true;
                break;
            }
#ifdef _WIN32
            ::closesocket(sock_fd_); sock_fd_ = INVALID_SOCKET;
#else
            ::close(sock_fd_); sock_fd_ = -1;
#endif
        }
        freeaddrinfo(res);
        return ok;
    }

    void ssh_client::do_handshake() {
        set_state(session_state::handshaking);
        if (logger_) logger_->info("[ssh_client] 프로토콜 식별자 배너 교환 중...");
        send_raw(reinterpret_cast<const uint8_t*>(client_banner_.data()), client_banner_.size());

        std::string line;
        char ch;
        while (true) {
            recv_raw(reinterpret_cast<uint8_t*>(&ch), 1);
            line += ch;
            if (ch == '\n') {
                if (line.rfind("SSH-", 0) == 0) {
                    server_banner_ = line;
                    while (!server_banner_.empty() && (server_banner_.back() == '\r' || server_banner_.back() == '\n')) {
                        server_banner_.pop_back();
                    }
                    break;
                }
                line.clear();
            }
        }
        if (logger_) logger_->info("[ssh_client] 서버 배너 수신: <bright_cyan>{}</bright_cyan>", server_banner_);

        // 1. KEXINIT 송신
        ssh_buffer kex;
        kex.write_byte(20);
        for (int i = 0; i < 16; ++i) kex.write_byte(0x55);
        kex.write_string("curve25519-sha256,curve25519-sha256@libssh.org");
        kex.write_string("ssh-ed25519,rsa-sha2-256,rsa-sha2-512,ssh-rsa");
        kex.write_string("aes128-ctr");
        kex.write_string("aes128-ctr");
        kex.write_string("hmac-sha2-256");
        kex.write_string("hmac-sha2-256");
        kex.write_string("none");
        kex.write_string("none");
        kex.write_string("");
        kex.write_string("");
        kex.write_byte(0);
        kex.write_uint32(0);

        client_kexinit_payload_ = kex.data();
        send_packet(kex);

        // 2. KEXINIT 수신
        auto kex_recv = recv_packet();
        server_kexinit_payload_ = kex_recv.data();

        // 3. X25519 ECDH 키 생성 및 송신
        uint8_t priv_c[32], pub_c[32], base[32] = { 9 };
        std::mt19937_64 rng(1337);
        for (int i = 0; i < 32; ++i) priv_c[i] = static_cast<uint8_t>(rng());
        mino::core::crypt::x25519::curve25519(pub_c, priv_c, base);

        ssh_buffer ecdh_init;
        ecdh_init.write_byte(30);
        ecdh_init.write_bytes(pub_c, 32);
        send_packet(ecdh_init);

        auto reply = recv_packet();
        if (reply.read_byte() != 31) throw std::runtime_error("비정상적인 ECDH 응답");

        auto k_s = reply.read_bytes();
        auto pub_s = reply.read_bytes();
        auto sig = reply.read_bytes();

        auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
        std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
        std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
        if (logger_) logger_->info("[ssh_client] 호스트 지문 검증: <bright_yellow>{}</bright_yellow>", fingerprint);

        if (host_key_verifier_cb_) {
            if (!host_key_verifier_cb_(host_, fingerprint)) {
                throw std::runtime_error("호스트 키 핑거프린트 검증이 거부되었습니다.");
            }
        }

        // 4. 공유 비밀키 K 계산 (RFC 8731: X25519 32바이트 출력값을 역전 없이 그대로 Big-Endian mpint로 인코딩)
        std::vector<uint8_t> K(32);
        mino::core::crypt::x25519::curve25519(K.data(), priv_c, pub_s.data());

        ssh_buffer h_buf;
        std::string client_id = client_banner_;
        while (!client_id.empty() && (client_id.back() == '\r' || client_id.back() == '\n')) client_id.pop_back();
        h_buf.write_string(client_id);
        h_buf.write_string(server_banner_);
        h_buf.write_bytes(client_kexinit_payload_.data(), client_kexinit_payload_.size());
        h_buf.write_bytes(server_kexinit_payload_.data(), server_kexinit_payload_.size());
        h_buf.write_bytes(k_s.data(), k_s.size());
        h_buf.write_bytes(pub_c, 32);
        h_buf.write_bytes(pub_s.data(), pub_s.size());
        h_buf.write_mpint(K.data(), K.size());

        auto H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
        if (session_id_.empty()) session_id_.assign(H.begin(), H.end());

        // 5. NEWKEYS 교환
        ssh_buffer newkeys;
        newkeys.write_byte(21);
        send_packet(newkeys);

        auto resp_nk = recv_packet();
        if (resp_nk.read_byte() != 21) throw std::runtime_error("NEWKEYS 패킷 수신 실패");

        auto iv_c2s = derive_key(K, H, 'A', 16);
        auto iv_s2c = derive_key(K, H, 'B', 16);
        auto key_c2s = derive_key(K, H, 'C', 16);
        auto key_s2c = derive_key(K, H, 'D', 16);
        auto mac_c2s = derive_key(K, H, 'E', 32);
        auto mac_s2c = derive_key(K, H, 'F', 32);

        enc_out_.init(key_c2s.data(), iv_c2s.data());
        enc_in_.init(key_s2c.data(), iv_s2c.data());
        std::memcpy(mac_key_out_.data(), mac_c2s.data(), 32);
        std::memcpy(mac_key_in_.data(), mac_s2c.data(), 32);

        keys_activated_ = true;
        if (logger_) logger_->info("[ssh_client] 암호화 터널 활성화 완료 <bright_green>(AES-128-CTR / HMAC-SHA256)</bright_green>");

        // 6. Userauth
        ssh_buffer srv;
        srv.write_byte(5);
        srv.write_string("ssh-userauth");
        send_packet(srv);

        auto srv_resp = recv_packet();
        if (srv_resp.read_byte() != 6) throw std::runtime_error("인증 서비스 거절");

        ssh_buffer auth;
        auth.write_byte(50);
        auth.write_string(username_);
        auth.write_string("ssh-connection");
        auth.write_string("password");
        auth.write_byte(0);
        auth.write_string(password_);
        send_packet(auth);

        auto auth_resp = recv_packet();
        if (auth_resp.read_byte() != 52) throw std::runtime_error("사용자 패스워드 인증 실패");

        // 7. Session Channel Open
        ssh_buffer ch_open;
        ch_open.write_byte(90);
        ch_open.write_string("session");
        ch_open.write_uint32(0);
        ch_open.write_uint32(2 * 1024 * 1024);
        ch_open.write_uint32(32768);
        send_packet(ch_open);

        auto open_resp = recv_packet();
        if (open_resp.read_byte() != 91) throw std::runtime_error("세션 채널 열기 실패");
        open_resp.read_uint32();
        remote_channel_id_ = open_resp.read_uint32();

        set_state(session_state::authenticated);
        if (logger_) logger_->info("[ssh_client] SSH 터널 및 세션 채널 개방 성공. 사용자: <bright_green>{}</bright_green>", username_);
    }

    void ssh_client::send_packet(const ssh_buffer& payload_buf) {
        std::lock_guard<std::mutex> lock(send_mutex_);
        const auto& payload = payload_buf.data();
        size_t block_size = keys_activated_ ? 16 : 8;

        // RFC 4253 Section 6: (4 [packet_length] + 1 [padding_length] + payload + pad_len) % block_size == 0
        size_t unpadded = 4 + 1 + payload.size();
        size_t pad_len = block_size - (unpadded % block_size);
        if (pad_len < 4) {
            pad_len += block_size;
        }

        uint32_t packet_len = static_cast<uint32_t>(1 + payload.size() + pad_len);
        std::vector<uint8_t> plain;
        plain.reserve(4 + packet_len);

        plain.push_back((packet_len >> 24) & 0xFF);
        plain.push_back((packet_len >> 16) & 0xFF);
        plain.push_back((packet_len >> 8) & 0xFF);
        plain.push_back(packet_len & 0xFF);
        plain.push_back(static_cast<uint8_t>(pad_len));
        plain.insert(plain.end(), payload.begin(), payload.end());

        static thread_local std::mt19937 rng(1337);
        for (size_t i = 0; i < pad_len; ++i) {
            plain.push_back(static_cast<uint8_t>(rng() & 0xFF));
        }

        if (!keys_activated_) {
            send_raw(plain.data(), plain.size());
        }
        else {
            ssh_buffer mac_buf;
            mac_buf.write_uint32(seq_out_);
            mac_buf.write_raw(plain.data(), plain.size());
            auto mac = mino::core::crypt::hmac_sha256(mac_key_out_.data(), 32, mac_buf.data().data(), mac_buf.size());

            std::vector<uint8_t> enc(plain.size());
            enc_out_.process(plain.data(), enc.data(), plain.size());

            send_raw(enc.data(), enc.size());
            send_raw(mac.data(), mac.size());
        }
        seq_out_++;
    }

    ssh_buffer ssh_client::recv_packet() {
        uint8_t head[4];
        recv_raw(head, 4);

        if (keys_activated_) enc_in_.process(head, head, 4);

        uint32_t packet_len = (head[0] << 24) | (head[1] << 16) | (head[2] << 8) | head[3];
        if (packet_len > 35000) throw std::runtime_error("허용 패킷 크기 초과");

        std::vector<uint8_t> body(packet_len);
        recv_raw(body.data(), packet_len);

        if (keys_activated_) {
            enc_in_.process(body.data(), body.data(), packet_len);
            uint8_t mac_remote[32];
            recv_raw(mac_remote, 32);

            ssh_buffer mac_buf;
            mac_buf.write_uint32(seq_in_);
            mac_buf.write_raw(head, 4);
            mac_buf.write_raw(body.data(), body.size());
            auto mac_local = mino::core::crypt::hmac_sha256(mac_key_in_.data(), 32, mac_buf.data().data(), mac_buf.size());
            if (std::memcmp(mac_remote, mac_local.data(), 32) != 0) {
                throw std::runtime_error("HMAC 무결성 검증 실패");
            }
        }
        seq_in_++;

        uint8_t pad_len = body[0];
        if (pad_len + 1 > packet_len) {
            throw std::runtime_error("패킷의 패딩 길이가 올바르지 않습니다.");
        }
        size_t payload_len = packet_len - 1 - pad_len;
        return ssh_buffer(std::vector<uint8_t>(body.begin() + 1, body.begin() + 1 + payload_len));
    }

    void ssh_client::receive_loop() {
        while (!stop_flag_ && is_connected()) {
            try {
                auto p = recv_packet();
                uint8_t type = p.read_byte();

                switch (type) {
                case 94: { // SSH_MSG_CHANNEL_DATA (stdout)
                    uint32_t ch_id = p.read_uint32();
                    std::string data = p.read_string();
                    {
                        std::lock_guard<std::mutex> lock(rx_queue_mutex_);
                        rx_queue_.push({ rx_event_type::stdout_data, ch_id, std::move(data) });
                    }
                    rx_cv_.notify_one();
                    break;
                }
                case 95: { // SSH_MSG_CHANNEL_EXTENDED_DATA (stderr)
                    uint32_t ch_id = p.read_uint32();
                    uint32_t code = p.read_uint32();
                    std::string data = p.read_string();
                    {
                        std::lock_guard<std::mutex> lock(rx_queue_mutex_);
                        rx_queue_.push({ rx_event_type::stderr_data, ch_id, std::move(data) });
                    }
                    rx_cv_.notify_one();
                    break;
                }
                case 80: { // SSH_MSG_GLOBAL_REQUEST (Keepalive ping)
                    std::string req_name = p.read_string();
                    uint8_t want_reply = p.read_byte();
                    if (want_reply) {
                        ssh_buffer reply;
                        reply.write_byte(82);
                        send_packet(reply);
                    }
                    break;
                }
                case 1: { // SSH_MSG_DISCONNECT
                    uint32_t reason = p.read_uint32();
                    std::string desc = p.read_string();
                    if (logger_) logger_->warn("[ssh_client] 서버로부터 세션 종료 통보 수신: <bright_yellow>{}</bright_yellow>", desc);
                    reset_session_state();
                    if (on_disconnect_cb_) on_disconnect_cb_(reason, desc);
                    break;
                }
                default:
                    break;
                }
            }
            catch (const std::exception& e) {
                if (!stop_flag_) {
                    if (logger_) logger_->warn("[ssh_client] 수신 에러 발생: <bright_yellow>{}</bright_yellow>", e.what());
                    reset_session_state();
                    if (on_disconnect_cb_) on_disconnect_cb_(0, e.what());
                }
                break;
            }
        }
    }

    void ssh_client::rx_dispatch_loop() {
        while (!stop_flag_) {
            rx_event ev;
            {
                std::unique_lock<std::mutex> lock(rx_queue_mutex_);
                rx_cv_.wait(lock, [this] {
                    return stop_flag_ || !rx_queue_.empty();
                    });

                if (stop_flag_ && rx_queue_.empty()) break;
                if (rx_queue_.empty()) continue;

                ev = std::move(rx_queue_.front());
                rx_queue_.pop();
            }

            try {
                if (ev.type == rx_event_type::stdout_data) {
                    if (on_stdout_cb_) on_stdout_cb_(ev.channel_id, ev.data);
                }
                else if (ev.type == rx_event_type::stderr_data) {
                    if (on_stderr_cb_) on_stderr_cb_(ev.channel_id, ev.data);
                }
            }
            catch (const std::exception& ex) {
                if (logger_) logger_->error("[ssh_client] 수신 디스패치 콜백 예외: {}", ex.what());
            }
            catch (...) {
                if (logger_) logger_->error("[ssh_client] 수신 디스패치 콜백 알 수 없는 예외 발생");
            }
        }
    }

    void ssh_client::connection_supervisor() {
        int attempts = 0;
        while (!stop_flag_) {
            reset_session_state();
            if (logger_) logger_->info("[ssh_client] 서버 접속 시도: {}:{}", host_, port_);

            if (establish_tcp()) {
                try {
                    do_handshake();
                    attempts = 0;
                    if (on_authenticated_cb_) on_authenticated_cb_();

                    receive_loop();
                }
                catch (const std::exception& e) {
                    if (logger_) logger_->error("[ssh_client] 핸드셰이크 실패: <bright_red>{}</bright_red>", e.what());
                    reset_session_state();
                }
            }
            else {
                if (logger_) logger_->warn("[ssh_client] TCP 소켓 연결 실패: {}:{}", host_, port_);
            }

            if (stop_flag_ || !policy_.enabled) break;

            attempts++;
            if (policy_.max_retries >= 0 && attempts > policy_.max_retries) {
                if (logger_) logger_->critical("[ssh_client] 최대 재시도 횟수 초과로 재연결을 중단합니다. (최대: {})", policy_.max_retries);
                break;
            }

            double factor = std::pow(policy_.backoff_multiplier, attempts - 1);
            auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(policy_.initial_delay * factor);
            if (delay_ms > policy_.max_delay) delay_ms = policy_.max_delay;

            if (logger_) logger_->warn("[ssh_client] {}ms 후 재연결을 시도합니다. ({}번째 시도)", delay_ms.count(), attempts);

            auto start_wait = std::chrono::steady_clock::now();
            while (!stop_flag_ && (std::chrono::steady_clock::now() - start_wait < delay_ms)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    bool ssh_client::start() {
        stop_flag_ = false;

        if (rx_dispatch_thread_.joinable()) rx_dispatch_thread_.join();
        rx_dispatch_thread_ = std::thread(&ssh_client::rx_dispatch_loop, this);

        if (worker_thread_.joinable()) worker_thread_.join();
        worker_thread_ = std::thread(&ssh_client::connection_supervisor, this);

        return true;
    }

    void ssh_client::stop() {
        stop_flag_ = true;

        clear_rx_queue();
        rx_cv_.notify_all();
        if (rx_dispatch_thread_.joinable() && rx_dispatch_thread_.get_id() != std::this_thread::get_id()) {
            rx_dispatch_thread_.join();
        }

        reset_session_state();

        if (worker_thread_.joinable() && worker_thread_.get_id() != std::this_thread::get_id()) {
            worker_thread_.join();
        }
    }

    void ssh_client::execute_command(const std::string& cmd) {
        if (!is_authenticated()) return;
        ssh_buffer req;
        req.write_byte(98);
        req.write_uint32(remote_channel_id_);
        req.write_string("exec");
        req.write_byte(1);
        req.write_string(cmd);
        send_packet(req);
    }

    void ssh_client::send_channel_data(const std::string& data) {
        if (!is_authenticated()) return;
        ssh_buffer req;
        req.write_byte(94);
        req.write_uint32(remote_channel_id_);
        req.write_string(data);
        send_packet(req);
    }

} // namespace mino::network::ssh
