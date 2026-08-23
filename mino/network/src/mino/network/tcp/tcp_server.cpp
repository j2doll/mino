#include <iostream>
#include <algorithm>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/network/tcp/tcp_server.hpp"

namespace mino::network::tcp {

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
        so_linger.l_onoff = 1;
        so_linger.l_linger = 0;

#ifdef _WIN32
        setsockopt(fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&so_linger), static_cast<int>(sizeof(so_linger)));
        closesocket(fd);
#else
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, static_cast<socklen_t>(sizeof(so_linger)));
        close(fd);
#endif
    }

    tcp_server::tcp_server()
        : is_running(false),
        logger(nullptr),
#ifdef _WIN32
        server_socket(INVALID_SOCKET)
#else
        server_socket(-1)
#endif
    {
    }

    tcp_server::~tcp_server() {
        quit();
    }

    tcp_server::start_result tcp_server::start(const std::string& ip, int port) {
        struct addrinfo hints {}, * res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        std::string port_str = std::to_string(port);
        if (getaddrinfo(ip.empty() ? nullptr : ip.c_str(), port_str.c_str(), &hints, &res) != 0 || !res)
        {
            if (logger) logger->error(
                "[tcp_server] getaddrinfo failed for {}:{}",
                ip.empty() ? "<all interfaces>" : ip, port);

            return start_result::socket_creation_failed;
        }

        struct addrinfo* p;
        for (p = res; p != nullptr; p = p->ai_next) {
            server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#ifdef _WIN32
            if (server_socket == INVALID_SOCKET) continue;
#else
            if (server_socket < 0) continue;
#endif

            int opt = 1;
            setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

            if (bind(server_socket, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
                address_family = p->ai_family;
                break;
            }
#ifdef _WIN32
            closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
            close(server_socket); server_socket = -1;
#endif
        }
        freeaddrinfo(res);

#ifdef _WIN32
        if (server_socket == INVALID_SOCKET) return start_result::bind_failed;
#else
        if (server_socket < 0) return start_result::bind_failed;
#endif

        if (listen(server_socket, SOMAXCONN) == -1) {
#ifdef _WIN32
            closesocket(server_socket); server_socket = INVALID_SOCKET;
#else
            close(server_socket); server_socket = -1;
#endif
            return start_result::listen_failed;
        }

        is_running = true;
        server_thread = std::thread(&tcp_server::accept_loop, this);
        return start_result::success;
    }

    void tcp_server::set_on_connect_callback(callback cb) { on_connect = std::move(cb); }
    void tcp_server::set_on_receive_callback(callback cb) { on_receive = std::move(cb); }
    void tcp_server::set_on_close_callback(callback cb) { on_close = std::move(cb); }

    void tcp_server::set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger_ptr) {
        logger = std::move(logger_ptr);
    }

    int tcp_server::send_to_client(socket_t client_socket, const std::string& message) {
        std::lock_guard<std::mutex> lock(send_mutex);
#ifdef _WIN32
        return send(client_socket, message.c_str(), static_cast<int>(message.size()), 0);
#else
        return send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
#endif
    }

    std::vector<socket_t> tcp_server::broadcast_to_clients(const std::string& message) {
        std::lock_guard<std::mutex> lock(send_mutex);
        std::vector<socket_t> failed_clients;

        for (socket_t client_socket : client_sockets) {
#ifdef _WIN32
            if (send(client_socket, message.c_str(), static_cast<int>(message.size()), 0) < 0) {
#else
            if (send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL) < 0) {
#endif
                failed_clients.push_back(client_socket);
            }
            }
        return failed_clients;
        }

    void tcp_server::close_client(socket_t client_socket) {
        std::lock_guard<std::mutex> lock(send_mutex);

        auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
        if (it != client_sockets.end()) {
            client_sockets.erase(it);
#ifdef _WIN32
            ::shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
#else
            ::shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
#endif
            if (on_close) {
                try {
                    on_close(client_socket, "Client disconnected");
                }
                catch (const std::exception& e) {
                    if (logger) logger->error("[tcp_server] on_close callback threw exception: {}", e.what());
                    else std::cerr << "[tcp_server] on_close callback threw exception: " << e.what() << std::endl;
                }
                catch (...) {
                    if (logger) logger->error("[tcp_server] on_close callback threw unknown exception");
                    else std::cerr << "[tcp_server] on_close callback threw unknown exception" << std::endl;
                }
            }
        }
    }

    void tcp_server::quit() {
        if (is_running) {
            is_running = false;

#ifdef _WIN32
            if (server_socket != INVALID_SOCKET) {
                ::shutdown(server_socket, SD_BOTH);
                closesocket(server_socket); server_socket = INVALID_SOCKET;
            }
#else
            if (server_socket >= 0) {
                ::shutdown(server_socket, SHUT_RDWR);
                close(server_socket); server_socket = -1;
            }
#endif

            if (server_thread.joinable()) {
                server_thread.join();
            }

            std::lock_guard<std::mutex> lock(send_mutex);
            for (socket_t client : client_sockets) {
#ifdef _WIN32
                ::shutdown(client, SD_BOTH);
                closesocket(client);
#else
                ::shutdown(client, SHUT_RDWR);
                close(client);
#endif
            }
            client_sockets.clear();
        }
    }

    void tcp_server::shutdown_by_force() {
        try {
            if (is_running) {
                is_running = false;
            }

#ifdef _WIN32
            if (server_socket != INVALID_SOCKET) {
                close_socket_without_timewait(server_socket);
                server_socket = INVALID_SOCKET;
            }
#else
            if (server_socket >= 0) {
                close_socket_without_timewait(server_socket);
                server_socket = -1;
            }
#endif

            if (server_thread.joinable()) {
                server_thread.join();
            }

            {
                std::lock_guard<std::mutex> lock(send_mutex);
                for (socket_t client : client_sockets) {
                    if (on_close) {
                        try {
                            on_close(client, "Forcefully closed");
                        }
                        catch (const std::exception& e) {
                            if (logger) logger->error("[tcp_server] on_close callback threw exception during force shutdown: {}", e.what());
                            else std::cerr << "[tcp_server] on_close callback threw exception during force shutdown: " << e.what() << std::endl;
                        }
                        catch (...) {
                            if (logger) logger->error("[tcp_server] on_close callback threw unknown exception during force shutdown");
                            else std::cerr << "[tcp_server] on_close callback threw unknown exception during force shutdown" << std::endl;
                        }
                    }
                    close_socket_without_timewait(client);
                }
                client_sockets.clear();
            }

        }
        catch (const std::exception& ex) {
            if (logger) logger->error("Exception in shutdown_by_force: {}", ex.what());
            else std::cerr << "Exception in shutdown_by_force: " << ex.what() << std::endl;
        }
        catch (...) {
            if (logger) logger->error("Unknown exception in shutdown_by_force");
            else std::cerr << "Unknown exception in shutdown_by_force" << std::endl;
        }
    }

    std::vector<socket_t> tcp_server::get_client_sockets() {
        std::lock_guard<std::mutex> lock(send_mutex);
        return client_sockets;
    }

    void tcp_server::accept_loop() {
        while (is_running) {
            sockaddr_storage client_addr{};
            socklen_t client_len = sizeof(client_addr);
            socket_t client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

#ifdef _WIN32
            if (client_socket == INVALID_SOCKET) {
#else
            if (client_socket < 0) {
#endif
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(send_mutex);
                client_sockets.push_back(client_socket);
            }

            if (on_connect) {
                try {
                    on_connect(client_socket, "New client connected");
                }
                catch (const std::exception& e) {
                    if (logger) logger->error("[tcp_server] on_connect callback threw exception: {}", e.what());
                    else std::cerr << "[tcp_server] on_connect callback threw exception: " << e.what() << std::endl;
                }
                catch (...) {
                    if (logger) logger->error("[tcp_server] on_connect callback threw unknown exception");
                    else std::cerr << "[tcp_server] on_connect callback threw unknown exception" << std::endl;
                }
            }

            try {
                std::thread(&tcp_server::client_handler, this, client_socket).detach();
            }
            catch (const std::system_error& e) {
                if (logger) logger->error("[tcp_server] Failed to create client handler thread: {}", e.what());
                else std::cerr << "[tcp_server] Failed to create client handler thread: " << e.what() << std::endl;
                close_client(client_socket);
            }
            }
        }

    void tcp_server::client_handler(socket_t client_socket) {
        try {
            char buffer[BUFFER_SIZE] = {};
            while (is_running) {
                int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    close_client(client_socket);
                    break;
                }

                if (on_receive) {
                    try {
                        on_receive(client_socket, std::string(buffer, bytes_received));
                    }
                    catch (const std::exception& e) {
                        if (logger) logger->error("[tcp_server] on_receive callback threw exception: {}", e.what());
                        else std::cerr << "[tcp_server] on_receive callback threw exception: " << e.what() << std::endl;
                        close_client(client_socket);
                        break;
                    }
                    catch (...) {
                        if (logger) logger->error("[tcp_server] on_receive callback threw unknown exception");
                        else std::cerr << "[tcp_server] on_receive callback threw unknown exception" << std::endl;
                        close_client(client_socket);
                        break;
                    }
                }
            }
        }
        catch (const std::exception& e) {
            if (logger) logger->error("[tcp_server] client_handler caught exception: {}", e.what());
            else std::cerr << "[tcp_server] client_handler caught exception: " << e.what() << std::endl;
            close_client(client_socket);
        }
        catch (...) {
            if (logger) logger->error("[tcp_server] client_handler caught unknown exception");
            else std::cerr << "[tcp_server] client_handler caught unknown exception" << std::endl;
            close_client(client_socket);
        }
    }

    }
