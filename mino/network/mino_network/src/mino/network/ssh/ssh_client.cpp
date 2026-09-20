#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <vector>

#include "mino/core/encoding/base64.hpp"
#include "mino/network/ssh/ssh_client.hpp"

namespace {

    // RFC 3526 Section 3: 2048-bit MODP Group 14 Prime
    const uint8_t DH_GROUP14_P[256] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
        0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
        0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
        0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
        0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
        0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
        0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
        0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
        0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
        0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
        0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
        0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
        0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
        0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A,
        0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
        0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96,
        0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
        0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
        0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
        0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C,
        0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
        0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03,
        0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
        0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
        0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
        0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5,
        0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
        0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    struct bn {
        std::vector<uint32_t> words;

        bn() = default;
        bn(uint64_t v) {
            if (v & 0xFFFFFFFF00000000ULL) {
                words.push_back(static_cast<uint32_t>(v));
                words.push_back(static_cast<uint32_t>(v >> 32));
            }
            else if (v) {
                words.push_back(static_cast<uint32_t>(v));
            }
        }

        void trim() {
            while (!words.empty() && words.back() == 0) words.pop_back();
        }

        bool is_zero() const { return words.empty(); }

        static int cmp(const bn& a, const bn& b) {
            if (a.words.size() != b.words.size())
                return a.words.size() < b.words.size() ? -1 : 1;
            for (size_t i = a.words.size(); i-- > 0;) {
                if (a.words[i] != b.words[i])
                    return a.words[i] < b.words[i] ? -1 : 1;
            }
            return 0;
        }

        bool operator<(const bn& o) const { return cmp(*this, o) < 0; }
        bool operator<=(const bn& o) const { return cmp(*this, o) <= 0; }
        bool operator>=(const bn& o) const { return cmp(*this, o) >= 0; }

        static bn sub(const bn& a, const bn& b) {
            bn r;
            r.words.resize(a.words.size(), 0);
            int64_t borrow = 0;
            for (size_t i = 0; i < a.words.size(); ++i) {
                int64_t w1 = a.words[i];
                int64_t w2 = i < b.words.size() ? b.words[i] : 0;
                int64_t diff = w1 - w2 - borrow;
                if (diff < 0) {
                    diff += 0x100000000LL;
                    borrow = 1;
                }
                else {
                    borrow = 0;
                }
                r.words[i] = static_cast<uint32_t>(diff);
            }
            r.trim();
            return r;
        }

        static bn mul(const bn& a, const bn& b) {
            if (a.is_zero() || b.is_zero()) return bn();
            bn r;
            r.words.resize(a.words.size() + b.words.size(), 0);
            for (size_t i = 0; i < a.words.size(); ++i) {
                uint64_t carry = 0;
                for (size_t j = 0; j < b.words.size(); ++j) {
                    uint64_t cur = static_cast<uint64_t>(r.words[i + j]) +
                        static_cast<uint64_t>(a.words[i]) * b.words[j] + carry;
                    r.words[i + j] = static_cast<uint32_t>(cur);
                    carry = cur >> 32;
                }
                size_t k = i + b.words.size();
                while (carry > 0 && k < r.words.size()) {
                    uint64_t cur = static_cast<uint64_t>(r.words[k]) + carry;
                    r.words[k] = static_cast<uint32_t>(cur);
                    carry = cur >> 32;
                    k++;
                }
            }
            r.trim();
            return r;
        }

        // Aliasing-safe div_mod implementation
        static void div_mod(const bn& num, const bn& den, bn& q_out, bn& rem_out) {
            if (den.is_zero()) throw std::runtime_error("Division by zero in bn");
            int c = cmp(num, den);
            if (c < 0) {
                q_out = bn();
                rem_out = num;
                return;
            }
            if (c == 0) {
                q_out = bn(1);
                rem_out = bn();
                return;
            }

            int num_bits = static_cast<int>(num.words.size()) * 32;
            while (num_bits > 0 && !((num.words[(num_bits - 1) / 32] >> ((num_bits - 1) % 32)) & 1)) {
                num_bits--;
            }

            bn local_q, local_rem;
            local_q.words.resize((num_bits + 31) / 32, 0);

            for (int i = num_bits - 1; i >= 0; --i) {
                uint32_t bit = (num.words[i / 32] >> (i % 32)) & 1;
                uint32_t carry = bit;
                for (size_t j = 0; j < local_rem.words.size(); ++j) {
                    uint64_t w = (static_cast<uint64_t>(local_rem.words[j]) << 1) | carry;
                    local_rem.words[j] = static_cast<uint32_t>(w);
                    carry = static_cast<uint32_t>(w >> 32);
                }
                if (carry) local_rem.words.push_back(carry);
                local_rem.trim();

                if (local_rem >= den) {
                    local_rem = sub(local_rem, den);
                    local_q.words[i / 32] |= (1U << (i % 32));
                }
            }
            local_q.trim();
            q_out = std::move(local_q);
            rem_out = std::move(local_rem);
        }

        static bn mod_exp(bn base, bn exp, const bn& mod) {
            bn res(1);
            bn dummy_q, rem_base;
            div_mod(base, mod, dummy_q, rem_base);
            base = rem_base;

            while (!exp.is_zero()) {
                if (!exp.words.empty() && (exp.words[0] & 1)) {
                    bn prod = mul(res, base);
                    bn rem_res;
                    div_mod(prod, mod, dummy_q, rem_res);
                    res = rem_res;
                }
                bn sq = mul(base, base);
                bn rem_sq;
                div_mod(sq, mod, dummy_q, rem_sq);
                base = rem_sq;

                uint32_t carry = 0;
                for (size_t i = exp.words.size(); i-- > 0;) {
                    uint32_t next_carry = (exp.words[i] & 1) ? 0x80000000U : 0;
                    exp.words[i] = (exp.words[i] >> 1) | carry;
                    carry = next_carry;
                }
                exp.trim();
            }
            return res;
        }

        static bn from_bytes(const uint8_t* b, size_t len) {
            bn r;
            if (len == 0) return r;
            r.words.resize((len + 3) / 4, 0);
            for (size_t i = 0; i < len; ++i) {
                size_t byte_idx_from_end = len - 1 - i;
                size_t word_idx = byte_idx_from_end / 4;
                size_t shift = (byte_idx_from_end % 4) * 8;
                r.words[word_idx] |= (static_cast<uint32_t>(b[i]) << shift);
            }
            r.trim();
            return r;
        }

        static std::vector<uint8_t> to_bytes(const bn& a) {
            if (a.is_zero()) return { 0 };
            std::vector<uint8_t> out;
            out.reserve(a.words.size() * 4);
            for (size_t i = a.words.size(); i-- > 0;) {
                uint32_t w = a.words[i];
                out.push_back(static_cast<uint8_t>((w >> 24) & 0xFF));
                out.push_back(static_cast<uint8_t>((w >> 16) & 0xFF));
                out.push_back(static_cast<uint8_t>((w >> 8) & 0xFF));
                out.push_back(static_cast<uint8_t>(w & 0xFF));
            }
            size_t start = 0;
            while (start + 1 < out.size() && out[start] == 0) start++;
            if (start > 0) out.erase(out.begin(), out.begin() + start);
            return out;
        }
    };

    std::vector<std::string> split_csv(const std::string& s) {
        std::vector<std::string> res;
        std::string item;
        std::istringstream ss(s);
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) res.push_back(item);
        }
        return res;
    }

} // anonymous namespace

namespace mino::network::ssh {

    ssh_client::ssh_client()
        : sock_fd_(
#ifdef _WIN32
            INVALID_SOCKET
#else
            - 1
#endif
        ) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }

    ssh_client::~ssh_client() {
        stop();
    }

    void ssh_client::set_server(const std::string& host, uint16_t port) { host_ = host; port_ = port; }
    void ssh_client::set_host(const std::string& host) { host_ = host; }
    void ssh_client::set_port(uint16_t port) { port_ = port; }
    void ssh_client::set_user(const std::string& username) { username_ = username; }
    void ssh_client::set_password(const std::string& password) { password_ = password; }
    void ssh_client::set_reconnect_policy(const reconnect_policy& policy) { policy_ = policy; }
    void ssh_client::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) { logger_ = std::move(logger_ptr); }

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
            if (n <= 0) throw std::runtime_error("Socket data transmission failed");
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
            if (n <= 0) throw std::runtime_error("The remote host closed the socket");
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

        {
            std::lock_guard<std::mutex> lock(channel_rx_mutex_);
            channel_rx_buf_.clear();
        }
        channel_rx_cv_.notify_all();

        {
            std::lock_guard<std::mutex> lock(channel_req_mutex_);
            channel_req_done_ = true;
            channel_req_success_ = false;
        }
        channel_req_cv_.notify_all();
    }

    bool ssh_client::establish_tcp() {
        set_state(session_state::connecting);
        struct addrinfo hints {}, * res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        std::string p_str = std::to_string(port_);

        if (getaddrinfo(host_.c_str(), p_str.c_str(), &hints, &res) != 0) {
            if (logger_) logger_->error("[ssh_client] getaddrinfo failed: {}:{}", host_, port_);
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

    /*
    void ssh_client::do_handshake() {
        set_state(session_state::handshaking);
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
        if (logger_) logger_->info("[ssh_client] Receive server banner: {}", server_banner_);

        // 1. KEXINIT
        const std::string supported_kex = "diffie-hellman-group-exchange-sha256,diffie-hellman-group14-sha256,curve25519-sha256,curve25519-sha256@libssh.org";
        ssh_buffer kex;
        kex.write_byte(20);
        for (int i = 0; i < 16; ++i) kex.write_byte(0x55);
        kex.write_string(supported_kex);
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

        // 2. KEXINIT 확인
        auto kex_recv = recv_packet();
        server_kexinit_payload_ = kex_recv.data();

        ssh_buffer srv_kex_parse(server_kexinit_payload_);
        srv_kex_parse.read_byte();
        for (int i = 0; i < 16; ++i) srv_kex_parse.read_byte();
        std::string server_kex_algos = srv_kex_parse.read_string();

        auto c_list = split_csv(supported_kex);
        auto s_list = split_csv(server_kex_algos);
        std::string chosen_kex;
        for (const auto& c_algo : c_list) {
            if (std::find(s_list.begin(), s_list.end(), c_algo) != s_list.end()) {
                chosen_kex = c_algo;
                break;
            }
        }
        if (chosen_kex.empty()) throw std::runtime_error("No matching KEX algorithm found.");
        if (logger_) logger_->info("[ssh_client] Negotiated KEX algorithm: {}", chosen_kex);

        std::vector<uint8_t> K;
        std::vector<uint8_t> k_s;
        std::array<uint8_t, 32> H{};

        if (chosen_kex == "diffie-hellman-group-exchange-sha256") {
            uint32_t min_bits = 1024;
            uint32_t n_bits = 2048;
            uint32_t max_bits = 4096;

            ssh_buffer gex_req;
            gex_req.write_byte(34); // SSH_MSG_KEX_DH_GEX_REQUEST
            gex_req.write_uint32(min_bits);
            gex_req.write_uint32(n_bits);
            gex_req.write_uint32(max_bits);
            send_packet(gex_req);

            auto gex_group = recv_packet();
            if (gex_group.read_byte() != 31)
                throw std::runtime_error("Abnormal GEX Group response (Expected 31)");

            auto p_bytes = gex_group.read_bytes();
            auto g_bytes = gex_group.read_bytes();

            bn p = bn::from_bytes(p_bytes.data(), p_bytes.size());
            bn g = bn::from_bytes(g_bytes.data(), g_bytes.size());

            uint8_t x_bytes[32];
            std::random_device rd;
            std::mt19937_64 rng(rd());
            for (int i = 0; i < 32; ++i) x_bytes[i] = static_cast<uint8_t>(rng());
            x_bytes[0] |= 0x80;
            bn x = bn::from_bytes(x_bytes, 32);

            // Correct mod_exp calculation without aliasing
            bn e_bn = bn::mod_exp(g, x, p);
            std::vector<uint8_t> e_bytes = bn::to_bytes(e_bn);

            // Send packet 32: SSH_MSG_KEX_DH_GEX_INIT
            ssh_buffer gex_init;
            gex_init.write_byte(32);
            gex_init.write_mpint(e_bytes.data(), e_bytes.size());
            send_packet(gex_init);

            // Receive packet 33: SSH_MSG_KEX_DH_GEX_REPLY
            auto reply = recv_packet();
            if (reply.read_byte() != 33)
                throw std::runtime_error("Abnormal GEX Reply response (Expected 33)");

            k_s = reply.read_bytes();
            auto f_bytes = reply.read_bytes();
            auto sig = reply.read_bytes();

            auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
            std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
            std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
            if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

            if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
                throw std::runtime_error("Host key fingerprint verification was rejected.");
            }

            bn f_bn = bn::from_bytes(f_bytes.data(), f_bytes.size());
            bn one(1);
            bn p_minus_one = bn::sub(p, one);
            if (f_bn <= one || f_bn >= p_minus_one) {
                throw std::runtime_error("Invalid server public key (f)");
            }

            bn K_bn = bn::mod_exp(f_bn, x, p);
            K = bn::to_bytes(K_bn);

            ssh_buffer h_buf;
            std::string client_id = client_banner_;
            while (!client_id.empty() && (client_id.back() == '\r' || client_id.back() == '\n')) client_id.pop_back();
            h_buf.write_string(client_id);
            h_buf.write_string(server_banner_);
            h_buf.write_bytes(client_kexinit_payload_.data(), client_kexinit_payload_.size());
            h_buf.write_bytes(server_kexinit_payload_.data(), server_kexinit_payload_.size());
            h_buf.write_bytes(k_s.data(), k_s.size());
            h_buf.write_uint32(min_bits);
            h_buf.write_uint32(n_bits);
            h_buf.write_uint32(max_bits);
            h_buf.write_mpint(p_bytes.data(), p_bytes.size());
            h_buf.write_mpint(g_bytes.data(), g_bytes.size());
            h_buf.write_mpint(e_bytes.data(), e_bytes.size());
            h_buf.write_mpint(f_bytes.data(), f_bytes.size());
            h_buf.write_mpint(K.data(), K.size());

            H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
        }
        else if (chosen_kex == "diffie-hellman-group14-sha256") {
            uint8_t x_bytes[32];
            std::random_device rd;
            std::mt19937_64 rng(rd());
            for (int i = 0; i < 32; ++i) x_bytes[i] = static_cast<uint8_t>(rng());
            x_bytes[0] |= 0x80;

            bn p = bn::from_bytes(DH_GROUP14_P, 256);
            bn g(2);
            bn x = bn::from_bytes(x_bytes, 32);

            bn e_bn = bn::mod_exp(g, x, p);
            std::vector<uint8_t> e_bytes = bn::to_bytes(e_bn);

            ssh_buffer dh_init;
            dh_init.write_byte(30);
            dh_init.write_mpint(e_bytes.data(), e_bytes.size());
            send_packet(dh_init);

            auto reply = recv_packet();
            if (reply.read_byte() != 31)
                throw std::runtime_error("Abnormal DH Group14 response (Expected 31)");

            k_s = reply.read_bytes();
            auto f_bytes = reply.read_bytes();
            auto sig = reply.read_bytes();

            auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
            std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
            std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
            if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

            if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
                throw std::runtime_error("Host key fingerprint verification was rejected.");
            }

            bn f_bn = bn::from_bytes(f_bytes.data(), f_bytes.size());
            bn one(1);
            bn p_minus_one = bn::sub(p, one);
            if (f_bn <= one || f_bn >= p_minus_one) {
                throw std::runtime_error("Invalid server public key (f)");
            }

            bn K_bn = bn::mod_exp(f_bn, x, p);
            K = bn::to_bytes(K_bn);

            ssh_buffer h_buf;
            std::string client_id = client_banner_;
            while (!client_id.empty() && (client_id.back() == '\r' || client_id.back() == '\n')) client_id.pop_back();
            h_buf.write_string(client_id);
            h_buf.write_string(server_banner_);
            h_buf.write_bytes(client_kexinit_payload_.data(), client_kexinit_payload_.size());
            h_buf.write_bytes(server_kexinit_payload_.data(), server_kexinit_payload_.size());
            h_buf.write_bytes(k_s.data(), k_s.size());
            h_buf.write_mpint(e_bytes.data(), e_bytes.size());
            h_buf.write_mpint(f_bytes.data(), f_bytes.size());
            h_buf.write_mpint(K.data(), K.size());

            H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
        }
        else {
            uint8_t priv_c[32], pub_c[32], base[32] = { 9 };
            std::random_device rd;
            std::mt19937_64 rng(rd());
            for (int i = 0; i < 32; ++i) priv_c[i] = static_cast<uint8_t>(rng());
            mino::core::crypt::x25519::curve25519(pub_c, priv_c, base);

            ssh_buffer ecdh_init;
            ecdh_init.write_byte(30);
            ecdh_init.write_bytes(pub_c, 32);
            send_packet(ecdh_init);

            auto reply = recv_packet();
            if (reply.read_byte() != 31)
                throw std::runtime_error("Abnormal ECDH response (Expected 31)");

            k_s = reply.read_bytes();
            auto pub_s = reply.read_bytes();
            auto sig = reply.read_bytes();

            auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
            std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
            std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
            if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

            if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
                throw std::runtime_error("Host key fingerprint verification was rejected.");
            }

            K.resize(32);
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

            H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
        }

        if (session_id_.empty()) session_id_.assign(H.begin(), H.end());

        // 3. NEWKEYS
        ssh_buffer newkeys;
        newkeys.write_byte(21);
        send_packet(newkeys);

        auto resp_nk = recv_packet();
        if (resp_nk.read_byte() != 21)
            throw std::runtime_error("Failed to receive the NEWKEYS packet");

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

        // 4. Userauth
        ssh_buffer srv;
        srv.write_byte(5);
        srv.write_string("ssh-userauth");
        send_packet(srv);

        auto srv_resp = recv_packet();
        if (srv_resp.read_byte() != 6)
            throw std::runtime_error("Authentication service rejection");

        ssh_buffer auth;
        auth.write_byte(50);
        auth.write_string(username_);
        auth.write_string("ssh-connection");
        auth.write_string("password");
        auth.write_byte(0);
        auth.write_string(password_);
        send_packet(auth);

        auto auth_resp = recv_packet();
        if (auth_resp.read_byte() != 52)
            throw std::runtime_error("User password authentication failed");

        // 5. Session Channel Open
        ssh_buffer ch_open;
        ch_open.write_byte(90);
        ch_open.write_string("session");
        ch_open.write_uint32(0);
        ch_open.write_uint32(2 * 1024 * 1024);
        ch_open.write_uint32(32768);
        send_packet(ch_open);



        auto open_resp = recv_packet();
        if (open_resp.read_byte() != 91)
            throw std::runtime_error("Session channel open failure");
        open_resp.read_uint32();
        remote_channel_id_ = open_resp.read_uint32();

        set_state(session_state::authenticated);
    }
    //*/
void ssh_client::do_handshake() {
    set_state(session_state::handshaking);

    // --- 배너 교환 ---
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
    if (logger_) logger_->info("[ssh_client] Receive server banner: {}", server_banner_);

    // --- 1. KEXINIT 송신 ---
    const std::string supported_kex = "diffie-hellman-group-exchange-sha256,diffie-hellman-group14-sha256,curve25519-sha256,curve25519-sha256@libssh.org";
    ssh_buffer kex;
    kex.write_byte(20); // SSH_MSG_KEXINIT
    for (int i = 0; i < 16; ++i) kex.write_byte(0x55);
    kex.write_string(supported_kex);
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

    // --- 2. KEXINIT 수신 및 알고리즘 결정 ---
    auto kex_recv = recv_packet();
    server_kexinit_payload_ = kex_recv.data();

    ssh_buffer srv_kex_parse(server_kexinit_payload_);
    srv_kex_parse.read_byte();
    for (int i = 0; i < 16; ++i) srv_kex_parse.read_byte();
    std::string server_kex_algos = srv_kex_parse.read_string();

    auto c_list = split_csv(supported_kex);
    auto s_list = split_csv(server_kex_algos);
    std::string chosen_kex;
    for (const auto& c_algo : c_list) {
        if (std::find(s_list.begin(), s_list.end(), c_algo) != s_list.end()) {
            chosen_kex = c_algo;
            break;
        }
    }
    if (chosen_kex.empty()) {
        throw std::runtime_error("No matching KEX algorithm found.");
    }
    if (logger_) logger_->info("[ssh_client] Negotiated KEX algorithm: {}", chosen_kex);

    std::vector<uint8_t> K;
    std::vector<uint8_t> k_s;
    std::array<uint8_t, 32> H{};

    if (chosen_kex == "diffie-hellman-group-exchange-sha256") {
        // [RFC 4419 DH-GEX] 34번 패킷으로 min=1024, n=2048, max=4096 비트 요청
        uint32_t min_bits = 1024;
        uint32_t n_bits = 2048;
        uint32_t max_bits = 4096;

        ssh_buffer gex_req;
        gex_req.write_byte(34); // SSH_MSG_KEX_DH_GEX_REQUEST
        gex_req.write_uint32(min_bits);
        gex_req.write_uint32(n_bits);
        gex_req.write_uint32(max_bits);
        send_packet(gex_req);

        // 31번 수신: SSH_MSG_KEX_DH_GEX_GROUP (p, g 소수 그룹 수신)
        auto gex_group = recv_packet();
        if (gex_group.read_byte() != 31)
            throw std::runtime_error("Abnormal GEX Group response (Expected 31)");

        auto p_bytes = gex_group.read_bytes();
        auto g_bytes = gex_group.read_bytes();

        bn p = bn::from_bytes(p_bytes.data(), p_bytes.size());
        bn g = bn::from_bytes(g_bytes.data(), g_bytes.size());

        // 256비트 개인키 x 생성 및 e = g^x mod p 계산
        uint8_t x_bytes[32];
        std::random_device rd;
        std::mt19937_64 rng(rd());
        for (int i = 0; i < 32; ++i) x_bytes[i] = static_cast<uint8_t>(rng());
        x_bytes[0] |= 0x80;
        bn x = bn::from_bytes(x_bytes, 32);

        bn e_bn = bn::mod_exp(g, x, p);
        std::vector<uint8_t> e_bytes = bn::to_bytes(e_bn);

        // 32번 송신: SSH_MSG_KEX_DH_GEX_INIT (e 전송)
        ssh_buffer gex_init;
        gex_init.write_byte(32);
        gex_init.write_mpint(e_bytes.data(), e_bytes.size());
        send_packet(gex_init);

        // 33번 수신: SSH_MSG_KEX_DH_GEX_REPLY (K_S, f, sig 수신)
        auto reply = recv_packet();
        if (reply.read_byte() != 33)
            throw std::runtime_error("Abnormal GEX Reply response (Expected 33)");

        k_s = reply.read_bytes();
        auto f_bytes = reply.read_bytes();
        auto sig = reply.read_bytes();

        auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
        std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
        std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
        if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

        if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
            throw std::runtime_error("Host key fingerprint verification was rejected.");
        }

        bn f_bn = bn::from_bytes(f_bytes.data(), f_bytes.size());
        bn one(1);
        bn p_minus_one = bn::sub(p, one);
        if (f_bn <= one || f_bn >= p_minus_one) {
            throw std::runtime_error("Invalid server public key (f)");
        }

        bn K_bn = bn::mod_exp(f_bn, x, p);
        K = bn::to_bytes(K_bn);

        // 교환 해시 H 계산 (RFC 4419 Section 3)
        ssh_buffer h_buf;
        std::string client_id = client_banner_;
        while (!client_id.empty() && (client_id.back() == '\r' || client_id.back() == '\n')) client_id.pop_back();
        h_buf.write_string(client_id);
        h_buf.write_string(server_banner_);
        h_buf.write_bytes(client_kexinit_payload_.data(), client_kexinit_payload_.size());
        h_buf.write_bytes(server_kexinit_payload_.data(), server_kexinit_payload_.size());
        h_buf.write_bytes(k_s.data(), k_s.size());
        h_buf.write_uint32(min_bits);
        h_buf.write_uint32(n_bits);
        h_buf.write_uint32(max_bits);
        h_buf.write_mpint(p_bytes.data(), p_bytes.size());
        h_buf.write_mpint(g_bytes.data(), g_bytes.size());
        h_buf.write_mpint(e_bytes.data(), e_bytes.size());
        h_buf.write_mpint(f_bytes.data(), f_bytes.size());
        h_buf.write_mpint(K.data(), K.size());

        H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
    }
    else if (chosen_kex == "diffie-hellman-group14-sha256") {
        // [RFC 4253 고정 MODP Group 14]
        uint8_t x_bytes[32];
        std::random_device rd;
        std::mt19937_64 rng(rd());
        for (int i = 0; i < 32; ++i) x_bytes[i] = static_cast<uint8_t>(rng());
        x_bytes[0] |= 0x80;

        bn p = bn::from_bytes(DH_GROUP14_P, 256);
        bn g(2);
        bn x = bn::from_bytes(x_bytes, 32);

        bn e_bn = bn::mod_exp(g, x, p);
        std::vector<uint8_t> e_bytes = bn::to_bytes(e_bn);

        // 30번 송신: SSH_MSG_KEXDH_INIT
        ssh_buffer dh_init;
        dh_init.write_byte(30);
        dh_init.write_mpint(e_bytes.data(), e_bytes.size());
        send_packet(dh_init);

        // 31번 수신: SSH_MSG_KEXDH_REPLY
        auto reply = recv_packet();
        if (reply.read_byte() != 31)
            throw std::runtime_error("Abnormal DH Group14 response (Expected 31)");

        k_s = reply.read_bytes();
        auto f_bytes = reply.read_bytes();
        auto sig = reply.read_bytes();

        auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
        std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
        std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
        if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

        if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
            throw std::runtime_error("Host key fingerprint verification was rejected.");
        }

        bn f_bn = bn::from_bytes(f_bytes.data(), f_bytes.size());
        bn one(1);
        bn p_minus_one = bn::sub(p, one);
        if (f_bn <= one || f_bn >= p_minus_one) {
            throw std::runtime_error("Invalid server public key (f)");
        }

        bn K_bn = bn::mod_exp(f_bn, x, p);
        K = bn::to_bytes(K_bn);

        ssh_buffer h_buf;
        std::string client_id = client_banner_;
        while (!client_id.empty() && (client_id.back() == '\r' || client_id.back() == '\n')) client_id.pop_back();
        h_buf.write_string(client_id);
        h_buf.write_string(server_banner_);
        h_buf.write_bytes(client_kexinit_payload_.data(), client_kexinit_payload_.size());
        h_buf.write_bytes(server_kexinit_payload_.data(), server_kexinit_payload_.size());
        h_buf.write_bytes(k_s.data(), k_s.size());
        h_buf.write_mpint(e_bytes.data(), e_bytes.size());
        h_buf.write_mpint(f_bytes.data(), f_bytes.size());
        h_buf.write_mpint(K.data(), K.size());

        H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
    }
    else {
        // [RFC 8731 Curve25519]
        uint8_t priv_c[32], pub_c[32], base[32] = { 9 };
        std::random_device rd;
        std::mt19937_64 rng(rd());
        for (int i = 0; i < 32; ++i) priv_c[i] = static_cast<uint8_t>(rng());
        mino::core::crypt::x25519::curve25519(pub_c, priv_c, base);

        ssh_buffer ecdh_init;
        ecdh_init.write_byte(30);
        ecdh_init.write_bytes(pub_c, 32);
        send_packet(ecdh_init);

        auto reply = recv_packet();
        if (reply.read_byte() != 31)
            throw std::runtime_error("Abnormal ECDH response (Expected 31)");

        k_s = reply.read_bytes();
        auto pub_s = reply.read_bytes();
        auto sig = reply.read_bytes();

        auto hostkey_hash = mino::core::crypt::sha256::hash(k_s.data(), k_s.size());
        std::vector<uint8_t> hkh_vec(hostkey_hash.begin(), hostkey_hash.end());
        std::string fingerprint = "SHA256:" + mino::core::encoding::base64_encode(hkh_vec);
        if (logger_) logger_->info("[ssh_client] Host fingerprint: {}", fingerprint);

        if (host_key_verifier_cb_ && !host_key_verifier_cb_(host_, fingerprint)) {
            throw std::runtime_error("Host key fingerprint verification was rejected.");
        }

        K.resize(32);
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

        H = mino::core::crypt::sha256::hash(h_buf.data().data(), h_buf.size());
    }

    if (session_id_.empty()) session_id_.assign(H.begin(), H.end());

    // --- 3. NEWKEYS 교환 ---
    ssh_buffer newkeys;
    newkeys.write_byte(21); // SSH_MSG_NEWKEYS
    send_packet(newkeys);

    auto resp_nk = recv_packet();
    if (resp_nk.read_byte() != 21)
        throw std::runtime_error("Failed to receive the NEWKEYS packet");

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

    // --- 4. 사용자 인증 (Userauth) ---
    ssh_buffer srv;
    srv.write_byte(5); // SSH_MSG_SERVICE_REQUEST
    srv.write_string("ssh-userauth");
    send_packet(srv);

    auto srv_resp = recv_packet();
    if (srv_resp.read_byte() != 6) // SSH_MSG_SERVICE_ACCEPT
        throw std::runtime_error("Authentication service rejection");

    ssh_buffer auth;
    auth.write_byte(50); // SSH_MSG_USERAUTH_REQUEST
    auth.write_string(username_);
    auth.write_string("ssh-connection");
    auth.write_string("password");
    auth.write_byte(0);
    auth.write_string(password_);
    send_packet(auth);

    auto auth_resp = recv_packet();
    if (auth_resp.read_byte() != 52) // SSH_MSG_USERAUTH_SUCCESS
        throw std::runtime_error("User password authentication failed");

    // --- 5. 세션 채널 오픈 및 중간 패킷 처리 ---
    ssh_buffer ch_open;
    ch_open.write_byte(90); // SSH_MSG_CHANNEL_OPEN
    ch_open.write_string("session");
    ch_open.write_uint32(0);              // Sender channel ID
    ch_open.write_uint32(2 * 1024 * 1024); // Window size (2MB)
    ch_open.write_uint32(32768);           // Max packet size (32KB)
    send_packet(ch_open);

    while (true) {
        auto open_resp = recv_packet();
        uint8_t msg_type = open_resp.read_byte();

        if (msg_type == 91) { // SSH_MSG_CHANNEL_OPEN_CONFIRMATION
            open_resp.read_uint32(); // Sender channel ID (0)
            remote_channel_id_ = open_resp.read_uint32();
            break;
        }
        else if (msg_type == 92) { // SSH_MSG_CHANNEL_OPEN_FAILURE
            open_resp.read_uint32();
            uint32_t reason = open_resp.read_uint32();
            std::string desc = open_resp.read_string();
            throw std::runtime_error("Server rejected channel open. Code: " +
                std::to_string(reason) + ", Desc: " + desc);
        }
        else if (msg_type == 80) { // SSH_MSG_GLOBAL_REQUEST (OpenSSH hostkeys 등)
            std::string req_name = open_resp.read_string();
            uint8_t want_reply = open_resp.read_byte();
            if (want_reply) {
                ssh_buffer reply;
                reply.write_byte(82); // SSH_MSG_REQUEST_FAILURE
                send_packet(reply);
            }
        }
        else if (msg_type == 4 || msg_type == 2) { // DEBUG(4) / IGNORE(2)
            continue;
        }
        else if (msg_type == 1) { // SSH_MSG_DISCONNECT
            uint32_t reason = open_resp.read_uint32();
            std::string desc = open_resp.read_string();
            throw std::runtime_error("Disconnected by server during channel open. Reason: " +
                std::to_string(reason) + ", Desc: " + desc);
        }
        else {
            throw std::runtime_error("Unexpected packet type during channel open: " +
                std::to_string(msg_type));
        }
    }

    set_state(session_state::authenticated);
}


    void ssh_client::send_packet(const ssh_buffer& payload_buf) {
        std::lock_guard<std::mutex> lock(send_mutex_);
        const auto& payload = payload_buf.data();
        size_t block_size = keys_activated_ ? 16 : 8;

        size_t unpadded = 4 + 1 + payload.size();
        size_t pad_len = block_size - (unpadded % block_size);
        if (pad_len < 4) pad_len += block_size;

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
        if (packet_len > 35000)
            throw std::runtime_error("Permission packet size exceeded");

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
                throw std::runtime_error("HMAC Integrity Verification Failure");
            }
        }
        seq_in_++;

        uint8_t pad_len = body[0];
        if (pad_len + 1 > packet_len) {
            throw std::runtime_error("The padding length of the packet is not correct.");
        }
        size_t payload_len = packet_len - 1 - pad_len;
        return ssh_buffer(std::vector<uint8_t>(body.begin() + 1, body.begin() + 1 + payload_len));
    }

    void ssh_client::send_window_adjust(uint32_t bytes_to_add) {
        ssh_buffer adj;
        adj.write_byte(93);
        adj.write_uint32(remote_channel_id_);
        adj.write_uint32(bytes_to_add);
        send_packet(adj);
    }

    void ssh_client::receive_loop() {
        while (!stop_flag_ && is_connected()) {
            try {
                auto p = recv_packet();
                uint8_t type = p.read_byte();

                switch (type) {

                case 93: { // SSH_MSG_CHANNEL_WINDOW_ADJUST
                    uint32_t ch_id = p.read_uint32();
                    uint32_t bytes_to_add = p.read_uint32();
                    // 서버가 보낸 윈도우 조절 패킷 정상 소비
                    break;
                }

                case 94: {
                    uint32_t ch_id = p.read_uint32();
                    std::string data = p.read_string();

                    send_window_adjust(static_cast<uint32_t>(data.size()));

                    {
                        std::lock_guard<std::mutex> lock(channel_rx_mutex_);
                        channel_rx_buf_.insert(channel_rx_buf_.end(), data.begin(), data.end());
                    }
                    channel_rx_cv_.notify_all();

                    {
                        std::lock_guard<std::mutex> lock(rx_queue_mutex_);
                        rx_queue_.push({ rx_event_type::stdout_data, ch_id, std::move(data) });
                    }
                    rx_cv_.notify_one();
                    break;
                }
                case 95: {
                    uint32_t ch_id = p.read_uint32();
                    uint32_t code = p.read_uint32();
                    std::string data = p.read_string();

                    send_window_adjust(static_cast<uint32_t>(data.size()));

                    {
                        std::lock_guard<std::mutex> lock(rx_queue_mutex_);
                        rx_queue_.push({ rx_event_type::stderr_data, ch_id, std::move(data) });
                    }
                    rx_cv_.notify_one();
                    break;
                }
                case 99: {
                    {
                        std::lock_guard<std::mutex> lock(channel_req_mutex_);
                        channel_req_success_ = true;
                        channel_req_done_ = true;
                    }
                    channel_req_cv_.notify_all();
                    break;
                }
                case 100: {
                    {
                        std::lock_guard<std::mutex> lock(channel_req_mutex_);
                        channel_req_success_ = false;
                        channel_req_done_ = true;
                    }
                    channel_req_cv_.notify_all();
                    break;
                }
                case 80: {
                    std::string req_name = p.read_string();
                    uint8_t want_reply = p.read_byte();
                    if (want_reply) {
                        ssh_buffer reply;
                        reply.write_byte(82);
                        send_packet(reply);
                    }
                    break;
                }
                case 1: {
                    uint32_t reason = p.read_uint32();
                    std::string desc = p.read_string();
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
                rx_cv_.wait(lock, [this] { return stop_flag_ || !rx_queue_.empty(); });

                if (stop_flag_ && rx_queue_.empty()) break;
                if (rx_queue_.empty()) continue;

                ev = std::move(rx_queue_.front());
                rx_queue_.pop();
            }

            try {
                if (ev.type == rx_event_type::stdout_data && on_stdout_cb_) on_stdout_cb_(ev.channel_id, ev.data);
                else if (ev.type == rx_event_type::stderr_data && on_stderr_cb_) on_stderr_cb_(ev.channel_id, ev.data);
            }
            catch (...) {}
        }
    }

    void ssh_client::connection_supervisor() {
        int attempts = 0;
        while (!stop_flag_) {
            reset_session_state();

            if (establish_tcp()) {
                try {
                    do_handshake();
                    attempts = 0;
                    if (on_authenticated_cb_) on_authenticated_cb_();
                    receive_loop();
                }
                catch (...) {
                    reset_session_state();
                }
            }

            if (stop_flag_ || !policy_.enabled) break;

            attempts++;
            if (policy_.max_retries >= 0 && attempts > policy_.max_retries) break;

            double factor = std::pow(policy_.backoff_multiplier, attempts - 1);
            auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(policy_.initial_delay * factor);
            if (delay_ms > policy_.max_delay) delay_ms = policy_.max_delay;

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

    bool ssh_client::connect_sync(const std::string& host, uint16_t port,
        const std::string& username, const std::string& password,
        std::chrono::milliseconds timeout) {
        stop();
        stop_flag_ = false;
        set_server(host, port);
        set_user(username);
        set_password(password);
        policy_.enabled = false;

        if (!establish_tcp()) return false;
        try {
            do_handshake();
        }
        catch (...) {
            reset_session_state();
            return false;
        }

        worker_thread_ = std::thread([this]() { receive_loop(); });
        return true;
    }

    bool ssh_client::request_subsystem(const std::string& subsystem, std::chrono::milliseconds timeout) {
        if (!is_authenticated()) return false;
        {
            std::lock_guard<std::mutex> lock(channel_req_mutex_);
            channel_req_done_ = false;
            channel_req_success_ = false;
        }

        ssh_buffer req;
        req.write_byte(98);
        req.write_uint32(remote_channel_id_);
        req.write_string("subsystem");
        req.write_byte(1);
        req.write_string(subsystem);
        send_packet(req);

        std::unique_lock<std::mutex> lock(channel_req_mutex_);
        if (!channel_req_cv_.wait_for(lock, timeout, [this]() { return channel_req_done_.load(); })) {
            return false;
        }
        return channel_req_success_.load();
    }

    void ssh_client::send_channel_data(const uint8_t* data, size_t len) {
        if (!is_authenticated()) return;
        ssh_buffer req;
        req.write_byte(94);
        req.write_uint32(remote_channel_id_);
        req.write_bytes(data, len);
        send_packet(req);
    }

    void ssh_client::send_channel_data(const std::string& data) {
        send_channel_data(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    bool ssh_client::recv_channel_exact(uint8_t* out, size_t len, std::chrono::milliseconds timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock<std::mutex> lock(channel_rx_mutex_);
        while (channel_rx_buf_.size() < len) {
            if (stop_flag_ || !is_connected()) return false;
            if (channel_rx_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                return false;
            }
        }
        std::memcpy(out, channel_rx_buf_.data(), len);
        channel_rx_buf_.erase(channel_rx_buf_.begin(), channel_rx_buf_.begin() + len);
        return true;
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

} // namespace mino::network::ssh
