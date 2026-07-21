#include <cerrno>
#include <cstring>
#include <sstream>

#include <spdlog/spdlog.h>

#include "mino/network/tcp/tcp_client.hpp"

namespace mino::network::tcp {

    // Helper: 소켓을 TIME_WAIT 없이 즉시 종료(RST)하도록 SO_LINGER 설정 후 닫습니다.
    static void close_socket_without_timewait(socket_t fd) {
        if (fd
#ifdef _WIN32
            == INVALID_SOCKET
#else
            < 0
#endif
        ) {
            return;
        }

        struct linger so_linger;
        so_linger.l_onoff = 1;    // linger 옵션 활성화
        so_linger.l_linger = 0;   // 0초 -> 즉시 RST 전송

#ifdef _WIN32
        setsockopt(fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&so_linger), static_cast<int>(sizeof(so_linger)));
        closesocket(fd);
#else
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, static_cast<socklen_t>(sizeof(so_linger)));
        close(fd);
#endif
    }

    tcp_client::tcp_client()
        : address_family(AF_INET),
#ifdef _WIN32
        socket_fd(INVALID_SOCKET),
#else
        socket_fd(-1),
#endif
        is_connected_flag(false), stop_flag(false), thread_running(false), server_port(0) {
    }

    tcp_client::~tcp_client() {
        stop();
    }

    void tcp_client::set_server(const std::string& ip, unsigned short port, int family) {
        server_ip = ip;
        server_port = port;
        this->address_family = family;
    }

    void tcp_client::set_on_connect(callback cb) { on_connect = std::move(cb); }
    void tcp_client::set_on_close(callback cb) { on_close = std::move(cb); }
    void tcp_client::set_on_receive(receive_callback cb) { on_receive = std::move(cb); }

    void tcp_client::set_logger(std::shared_ptr<spdlog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    bool tcp_client::start(std::chrono::seconds sleep_time) {
        if (server_ip.empty() || server_port == 0) return false;
        stop_flag = false;
        // 기존 스레드가 남아있다면 먼저 정리하는게 안전합니다.
        if (client_thread.joinable()) {
            // move-out 패턴: 빠르게 소유권 이전 후 detach(안전성 우선)
            try {
                std::thread tmp = std::move(client_thread);
                if (tmp.joinable()) {
                    tmp.detach();
                }
            } catch (...) {
                // 무시: 안전하게 새 스레드를 시작
            }
        }
        client_thread = std::thread(&tcp_client::connect_to_server, this, sleep_time);
        return true;
    }

    void tcp_client::stop() {
        stop_flag = true;
        close_connection();

        // client_thread를 안전하게 이동시켜 외부에서 정리
        std::thread local_thread;
        if (client_thread.joinable()) {
            if (client_thread.get_id() != std::this_thread::get_id()) {
                local_thread = std::move(client_thread);
            } else {
                if (logger) {
                    std::ostringstream ss;
                    ss << std::this_thread::get_id();
                    logger->warn("[tcp_client] stop() called from client thread; skipping join to avoid deadlock. thread_id={}", ss.str());
                }
            }
        }

        if (local_thread.joinable()) {
            // 스레드가 정상 종료할 시간을 짧게 기다립니다(총 500ms).
            int retries = 50;
            while (thread_running && retries-- > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // 안전 최우선: join이 문제를 일으키는 상황이므로 detach로 처리하여 크래시를 회피합니다.
            try {
                local_thread.detach();
            } catch (...) {
                // 무시
            }
        }
    }

    int tcp_client::send_data(const std::string& data) {
        if (!is_connected() || data.empty()) return -1;

        std::lock_guard<std::mutex> lock(send_mutex);
#ifdef _WIN32
        return ::send(socket_fd, data.c_str(), static_cast<int>(data.size()), 0);
#else
        return ::send(socket_fd, data.c_str(), data.size(), MSG_NOSIGNAL);
#endif
    }

    void tcp_client::close_connection() {
#ifdef _WIN32
        if (socket_fd == INVALID_SOCKET) {
            if (logger) logger->debug("[tcp_client] close_connection() called but socket already INVALID_SOCKET");
            return;
        }
#else
        if (socket_fd < 0) {
            if (logger) logger->debug("[tcp_client] close_connection() called but socket_fd < 0");
            return;
        }
#endif

        {
            std::lock_guard<std::mutex> lock(send_mutex);
            is_connected_flag = false;
#ifdef _WIN32
            // Interrupt blocking recv/connect on Windows
            ::shutdown(socket_fd, SD_BOTH);
            closesocket(socket_fd);
            socket_fd = INVALID_SOCKET;
#else
            // Interrupt blocking recv/connect on POSIX (Rocky8 포함)
            ::shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            socket_fd = -1;
#endif
        }

        if (logger) {
#ifdef _WIN32
            int err = WSAGetLastError();
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            logger->info("[tcp_client] close_connection() executed. Last WSA error: {} thread_id={}", err, ss.str());
#else
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            logger->info("[tcp_client] close_connection() executed. errno: {} ({}) thread_id={}", errno, std::strerror(errno), ss.str());
#endif
        }

        if (on_close) on_close();
    }

    bool tcp_client::is_connected() const { return is_connected_flag; }

    void tcp_client::connect_to_server(std::chrono::seconds sleep_time) {
        thread_running = true;
        while (!stop_flag) {
            // [수정] 데드락 해결을 위해 socket 생성 시 불필요한 전체 락을 제거합니다.
            socket_t tmp_fd = ::socket(address_family, SOCK_STREAM, 0);
#ifdef _WIN32
            if (tmp_fd == INVALID_SOCKET) {
#else
            if (tmp_fd < 0) {
#endif
                // 연결 실패 시, sleep_time 초만큼 대기 후 재시도
                std::this_thread::sleep_for(sleep_time);
                continue;
            }

            sockaddr_storage server_addr_storage{};
            socklen_t addr_len = 0;

            if (address_family == AF_INET) { // ip v4
                sockaddr_in* addr4 = reinterpret_cast<sockaddr_in*>(&server_addr_storage);
                addr4->sin_family = AF_INET;
                addr4->sin_port = htons(server_port);
                inet_pton(AF_INET, server_ip.c_str(), &addr4->sin_addr);
                addr_len = sizeof(sockaddr_in);
            }
            else  if (address_family == AF_INET6) { // ip v6
                sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(&server_addr_storage);
                addr6->sin6_family = AF_INET6;
                addr6->sin6_port = htons(server_port);
                inet_pton(AF_INET6, server_ip.c_str(), &addr6->sin6_addr);
                addr_len = sizeof(sockaddr_in6);
            }
            else {
                // 정의되지 않은 주소 패밀리
                if (logger) logger->error("Undefined address family: {}", address_family);
                std::this_thread::sleep_for(sleep_time);
                continue;
            }

            // 연결 시도 
            if (::connect(tmp_fd, reinterpret_cast<sockaddr*>(&server_addr_storage), addr_len) == 0) {
                {
                    std::lock_guard<std::mutex> lock(send_mutex);
                    socket_fd = tmp_fd;
                    is_connected_flag = true;
                }
                if (logger) logger->info("[tcp_client] Connected to {}:{}", server_ip, server_port);
                if (on_connect) {
                    on_connect();
                }
                receive_loop();
            }
            else
            {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (logger) logger->warn("[tcp_client] connect() failed. WSAGetLastError: {}", err);
                closesocket(tmp_fd);
#else
                int err = errno;
                if (logger) logger->warn("[tcp_client] connect() failed. errno: {} ({})", err, std::strerror(err));
                close(tmp_fd);
#endif
            }

            std::this_thread::sleep_for(sleep_time);
        }
        thread_running = false;
    }

    void tcp_client::receive_loop() {
        char buffer[BUFFER_SIZE];
        while (!stop_flag) {
#ifdef _WIN32
            int bytes_received = ::recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
#else
            int bytes_received = ::recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
#endif
            if (bytes_received > 0) {
                if (on_receive) on_receive(std::string(buffer, bytes_received));
            }
            else
            {
                // bytes_received == 0 : orderly shutdown by peer
                // bytes_received < 0 : error
                if (logger) {
#ifdef _WIN32
                    int err = WSAGetLastError();
                    std::ostringstream ss;
                    ss << std::this_thread::get_id();
                    logger->warn("[tcp_client] recv returned {}. WSAGetLastError: {} thread_id={}", bytes_received, err, ss.str());
#else
                    int err = errno;
                    std::ostringstream ss;
                    ss << std::this_thread::get_id();
                    if (bytes_received == 0) {
                        logger->warn("[tcp_client] recv returned 0 (peer closed connection). thread_id={}", ss.str());
                    } else {
                        logger->warn("[tcp_client] recv returned {}. errno: {} ({}) thread_id={}", bytes_received, err, std::strerror(err), ss.str());
                    }
#endif
                }

                break;
            }
        }

        // receive_loop 종료 직후 로그
        if (logger) {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            logger->info("[tcp_client] receive_loop exiting. thread_id={}", ss.str());
        }

        close_connection();
    }

    void tcp_client::shutdown_by_force() {
        try {
            // 강제 종료: SO_LINGER{on,0} 적용 후 즉시 닫음
            stop_flag = true;

            // 먼저 현재 연결 소켓을 강제 종료하여 blocking 동작을 깨웁니다.
            {
                std::lock_guard<std::mutex> lock(send_mutex);
#ifdef _WIN32
                if (socket_fd != INVALID_SOCKET) {
                    close_socket_without_timewait(socket_fd);
                    socket_fd = INVALID_SOCKET;
                }
#else
                if (socket_fd >= 0) {
                    close_socket_without_timewait(socket_fd);
                    socket_fd = -1;
                }
#endif
                is_connected_flag = false;
            }

            // 스레드 정리: move-out + 짧은 대기 후 detach
            std::thread local_thread;
            if (client_thread.joinable()) {
                if (client_thread.get_id() != std::this_thread::get_id()) {
                    local_thread = std::move(client_thread);
                } else {
                    if (logger) {
                        std::ostringstream ss;
                        ss << std::this_thread::get_id();
                        logger->warn("[tcp_client] shutdown_by_force() called from client thread; skipping join to avoid deadlock. thread_id={}", ss.str());
                    }
                }
            }

            if (local_thread.joinable()) {
                int retries = 50;
                while (thread_running && retries-- > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                try {
                    local_thread.detach();
                } catch (...) {
                    // 무시
                }
            }

            // 콜백 알림
            if (on_close) {
                on_close();
            }

        } catch (const std::exception& ex) {
            if (logger) logger->error("Exception in shutdown_by_force: {}", ex.what());
            else std::cerr << "Exception in shutdown_by_force: " << ex.what() << std::endl;
        } catch (...) {
            if (logger) logger->error("Unknown exception in shutdown_by_force.");
            else std::cerr << "Unknown exception in shutdown_by_force." << std::endl;
        } 
    }

} 
