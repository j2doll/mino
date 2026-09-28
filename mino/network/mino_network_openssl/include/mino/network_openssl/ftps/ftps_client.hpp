#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <atomic>
#include <thread>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mino/network/ethernet.hpp"
#include "mino/network/ftp/tcp/ftp_client.hpp"

namespace mino::network_openssl::ftps {

    using file_info = mino::network::ftp::tcp::file_info;
    using ftp_client_base = mino::network::ftp::tcp::ftp_client_base;
    using i_progress_listener = mino::network::ftp::tcp::i_progress_listener;

    enum class ftps_data_protection {
        clear,       // PROT C: 데이터 채널 평문
        private_ssl  // PROT P: 데이터 채널 TLS 암호화 (표준)
    };

    // TLS 세션 재개가 적용된 데이터 채널 소켓 RAII 구조체
    struct tls_data_connection {
        socket_t sock{
#ifdef _WIN32
            INVALID_SOCKET
#else
            - 1
#endif
        };
        SSL* ssl{ nullptr };

        ~tls_data_connection() {
            close_connection();
        }

        void close_connection() {
            if (ssl) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = nullptr;
            }
#ifdef _WIN32
            if (sock != INVALID_SOCKET) {
                ::shutdown(sock, SD_BOTH);
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
#else
            if (sock >= 0) {
                ::shutdown(sock, SHUT_RDWR);
                close(sock);
                sock = -1;
            }
#endif
        }

        int write_data(const char* buf, int len) {
            if (ssl) return SSL_write(ssl, buf, len);
#ifdef _WIN32
            return ::send(sock, buf, len, 0);
#else
            return static_cast<int>(::send(sock, buf, len, 0));
#endif
        }

        int read_data(char* buf, int len) {
            if (ssl) return SSL_read(ssl, buf, len);
#ifdef _WIN32
            return ::recv(sock, buf, len, 0);
#else
            return static_cast<int>(::recv(sock, buf, len, 0));
#endif
        }
    };

    class ftps_client : public ftp_client_base {
    private:
        socket_t control_socket_{
#ifdef _WIN32
            INVALID_SOCKET
#else
            - 1
#endif
        };
        SSL_CTX* ssl_ctx_{ nullptr };
        SSL* ssl_control_{ nullptr };
        std::mutex ssl_mutex_;

        std::thread receive_thread_;
        std::atomic<bool> is_connected_{ false };
        std::atomic<bool> stop_flag_{ false };

        std::mutex control_mutex_;
        std::condition_variable control_cv_;
        std::string control_buffer_;
        bool has_control_response_{ false };

        ftps_data_protection data_prot_{ ftps_data_protection::private_ssl };
        bool verify_peer_{ false };
        std::string ca_file_;
        std::string ca_path_;

        bool init_ssl_ctx();
        void cleanup_ssl();
        void close_control_socket();
        void receive_loop();

        std::string read_plain_response(std::chrono::seconds timeout = std::chrono::seconds(5));
        bool send_plain_command(const std::string& cmd, const std::string& arg = "");

        std::string read_control_response(std::chrono::seconds timeout = std::chrono::seconds(5));
        bool send_command(const std::string& cmd, const std::string& arg = "");

        std::unique_ptr<tls_data_connection> establish_tls_data_connection();
        std::int64_t get_remote_file_size(const std::string& remote_file);

    public:
        ftps_client();
        ~ftps_client() override;

        void set_tls_verification(bool verify, const std::string& ca_file = "", const std::string& ca_path = "");
        void set_data_protection_mode(ftps_data_protection mode);

        bool connect(const std::string& host, int p = 21,
            const std::string& user = "anonymous",
            const std::string& pass = "") override;

        bool upload(const std::string& local_file, const std::string& remote_file) override;
        bool download(const std::string& remote_file, const std::string& local_file) override;
        bool delete_file(const std::string& remote_file) override;
        std::vector<file_info> list_directory(const std::string& path) override;
        bool create_directory(const std::string& path) override;
        bool remove_directory(const std::string& path) override;
    };

} // namespace mino::network_openssl::ftps
