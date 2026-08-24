#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/tcp/tcp_server.hpp"

class my_tcp_server_handler {
public:
    void on_receive(socket_t client_socket, const std::string& message) {
        std::string ret = "my_handler::on_receive: " + std::to_string(client_socket) + " - " + message;
        std::cout << ret << std::endl;
    }

    void on_connect(socket_t client_socket, const std::string& message) {
        std::string ret = "my_handler::on_connect: " + std::to_string(client_socket) + " - " + message;
        std::cout << ret << std::endl;
    }

    void on_close(socket_t client_socket, const std::string& message) {
        std::string ret = "my_handler::on_close: " + std::to_string(client_socket) + " - " + message;
        std::cout << ret << std::endl;
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
    mino::network::sock mnsock;

    auto& handler = mino::core::daemon::termination_handler::get_instance();
    handler.initialize();

    namespace mclt = mino::core::log::tinylog;

    // 메인 로거 생성
    auto main_console_sink = std::make_shared<mclt::console_sink>("main_console");
    assert(main_console_sink);
    auto main_logger = std::make_shared<mclt::logger>("main_logger");
    assert(main_logger); 
    main_logger->add_sink(main_console_sink);
    mclt::logger::register_logger(main_logger);

    using tcp_server = mino::network::tcp::tcp_server;
    tcp_server server_ipv4; // IPv4 server
    tcp_server server_ipv6; // IPv6 server

    handler.set_callback([&server_ipv4, &server_ipv6]() {
        clean_up_resources(&server_ipv4, &server_ipv6);
        std::exit(0);
    });

    // IPv4 로거 생성
    auto tcp4_console_sink = std::make_shared<mclt::console_sink>("tcp4_console");
    assert(tcp4_console_sink);
    auto tcp4_logger = std::make_shared<mclt::logger>("tcp4_logger");
    assert(tcp4_logger);
    tcp4_logger->add_sink(tcp4_console_sink);
    mclt::logger::register_logger(tcp4_logger);
    server_ipv4.set_logger(tcp4_logger);

    my_tcp_server_handler my_handler;

    // IPv4 콜백 설정
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
        return -1;
    }

    // IPv6 로거 생성
    auto tcp6_console_sink = std::make_shared<mclt::console_sink>("tcp6_console");
    assert(tcp6_console_sink);
    auto tcp6_logger = std::make_shared<mclt::logger>("tcp6_logger");
    assert(tcp6_logger);
    tcp6_logger->add_sink(tcp6_console_sink);
    mclt::logger::register_logger(tcp6_logger);
    server_ipv6.set_logger(tcp6_logger);

    // IPv6 콜백 설정
    server_ipv6.set_on_connect_callback(
        [&my_handler](socket_t client_socket, const std::string& message) {
            my_handler.on_connect(client_socket, "[IPv6] " + message);
        }
    );

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

    std::string ipv6_addr = "::1";
    unsigned short port_ipv6 = 12346;

    if (server_ipv6.start(ipv6_addr, port_ipv6) != tcp_server::start_result::success) {
        tcp6_logger->error("Failed to start IPv6 server");
        server_ipv4.quit();
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

    return 0;
}
