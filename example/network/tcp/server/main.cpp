#include <iostream>
#include <thread>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/termination_handler.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/tcp/tcp_server.hpp"

class my_tcp_server_handler {
public:
    void on_receive(socket_t client_socket, const std::string& message) {
        std::cout << "my_handler::on_receive: " << client_socket << " - " << message << std::endl;
    }
    void on_connect(socket_t client_socket, const std::string& message) {
        std::cout << "my_handler::on_connect: " << client_socket << " - " << message << std::endl;
    }
    void on_close(socket_t client_socket, const std::string& message) {
        std::cout << "my_handler::on_close: " << client_socket << " - " << message << std::endl;
    }
};

void clean_up_resources(
    mino::network::tcp::tcp_server* tcp4_server,
    mino::network::tcp::tcp_server* tcp6_server)
{
    tcp4_server->quit();
    tcp6_server->quit();
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }
#endif

    auto& handler = mino::core::daemon::termination_handler::get_instance();
    handler.initialize();

    auto main_logger = ::spdlog::stdout_color_mt("main_logger");

    using tcp_server = mino::network::tcp::tcp_server;
    tcp_server server_ipv4; // ip v4 server
    tcp_server server_ipv6; // ip v6 server

    handler.set_callback([&server_ipv4, &server_ipv6]() {
        clean_up_resources(&server_ipv4, &server_ipv6);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
    });

    auto tcp4_logger = ::spdlog::stdout_color_mt("tcp4_logger");
    server_ipv4.set_logger(tcp4_logger);

    my_tcp_server_handler my_handler; // Custom tcp handler for events

    // set callbacks 
    server_ipv4.set_on_connect_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_connect(client_socket, "[IPv4] " + message);
        }
    );

    server_ipv4.set_on_receive_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_receive(client_socket, "[IPv4] " + message);
        }
    );

    server_ipv4.set_on_close_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_close(client_socket, "[IPv4] " + message);
        }
    );

    std::string ipv4_addr = "127.0.0.1";
    unsigned short port_ipv4 = 12345;

    if (server_ipv4.start(ipv4_addr, port_ipv4) != tcp_server::start_result::success) {
        tcp4_logger->error("Failed to start IPv4 server");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    // set callbacks
    server_ipv6.set_on_connect_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_connect(client_socket, "[IPv6] " + message);
        }
    );

    auto tcp6_logger = ::spdlog::stdout_color_mt("tcp6_logger");
    server_ipv6.set_logger(tcp6_logger);

    server_ipv6.set_on_receive_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_receive(client_socket, "[IPv6] " + message);
        }
    );

    server_ipv6.set_on_close_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_close(client_socket, "[IPv6] " + message);
        }
    );

    std::string ipv6_addr = "::1"; // Loopback address for IPv6
    unsigned short port_ipv6 = 12346;

    if (server_ipv6.start(ipv6_addr, port_ipv6) != tcp_server::start_result::success) {
        tcp6_logger->error("Failed to start IPv6 server");
        server_ipv4.quit();
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    tcp4_logger->info("IPv4 server running on {}:{}", ipv4_addr, port_ipv4);
    tcp6_logger->info("IPv6 server running on [{}]:{}", ipv6_addr, port_ipv6);

    main_logger->info("Type 'broadcast4' or 'broadcast6' to send a message, 'quit' to stop.");

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit") {
            break;
        }
        if (input.rfind("broadcast4 ", 0) == 0) {
            std::string msg = input.substr(11);
            std::vector<socket_t> failed_list = server_ipv4.broadcast_to_clients(msg);
            for (auto failed_id : failed_list) {
                tcp4_logger->error("Failed to send to IPv4 client: {}", failed_id);
            }
        }
        else if (input.rfind("broadcast6 ", 0) == 0) {
            std::string msg = input.substr(11);
            std::vector<socket_t> failed_list = server_ipv6.broadcast_to_clients(msg);
            for (auto failed_id : failed_list) {
                tcp6_logger->error("Failed to send to IPv6 client: {}", failed_id);
            }
        }
    }

    main_logger->info("Shutting down servers...");

    server_ipv4.quit();
    server_ipv6.quit();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

