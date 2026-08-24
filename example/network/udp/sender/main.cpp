#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <memory>
#include <atomic>
#include <thread>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/daemon.hpp"
#include "mino/core/log/log.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/network.hpp"

void udp_sender_thread(
    mino::network::udp::udp_sender& sender,
    std::shared_ptr<mino::core::log::tinylog::logger> logger,
    std::atomic<bool>& running_flag)
{
    mino::network::udp::udp_sender sender6;
    sender6.set_logger(logger);

    bool reuse_address6 = true;
    bool use_multicast6 = true;
    if (!sender6.create(AF_INET6, reuse_address6, use_multicast6)) {
        if (logger) logger->error("Failed to create UDP sender for IPv6");
        return;
    }

    while (running_flag.load()) {
        // IPv4 Unicast
        sender.send_data_to("IPv4 Hello Unicast", "127.0.0.1", 12345);
        if (logger) logger->info("<bright_green>[Unicast IPv4]</bright_green> Sent at 127.0.0.1:12345");

        // IPv4 Multicast
        sender.send_data_to("IPv4 Hello Multicast", "239.255.255.250", 12347);
        if (logger) logger->info("<bright_green>[Multicast IPv4]</bright_green> Sent at 239.255.255.250:12347");

        // IPv4 Broadcast
        sender.send_data_to("IPv4 Hello Broadcast", "255.255.255.255", 12349);
        if (logger) logger->info("<bright_green>[Broadcast IPv4]</bright_green> Sent at 255.255.255.255:12349");

        // IPv6 Unicast
        auto loopback6 = "::1";
        auto ret_l6 = sender6.send_data_to("Hello Unicast IPv6", loopback6, 12346);
        if (logger) logger->info("<bright_green>[Unicast IPv6]</bright_green> Sent at [::1]:12346 -> {}", ret_l6);

        // IPv6 Multicast
        auto ret_m6 = sender6.send_data_to("Hello Multicast IPv6", "ff3e::1234", 12348);
        if (logger) logger->info("<bright_green>[Multicast IPv6]</bright_green> Sent at [ff3e::1234]:12348 -> {}", ret_m6);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void clean_up_resources(mino::network::udp::udp_sender* udp_sender)
{
    udp_sender->stop();
}

int main() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }
#endif

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto console_sink = std::make_shared<mclt::console_sink>("udp_sender_console");
    auto logger = std::make_shared<mclt::logger>("udp_sender_logger");
    logger->add_sink(console_sink);
    mclt::logger::register_logger(logger);

    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize();

    namespace mnu = mino::network;
    using udp_sender = mnu::udp::udp_sender;

    udp_sender sender;
    sender.set_logger(logger);

    // 프로세스 종료 시그널(SIGINT, SIGTERM, Ctrl+C) 발생 시 자원 정리 콜백 등록
    handler.set_callback([&sender]() {
        clean_up_resources(&sender);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
        });

    bool reuse_address = true;
    bool use_multicast = true;
    if (!sender.create(AF_INET, reuse_address, use_multicast)) {
        logger->error("Failed to create UDP sender");
        return 1;
    }
    if (!sender.set_multicast_ttl(2)) {
        logger->error("Failed to set multicast TTL");
    }

    std::atomic<bool> running_flag(true);
    std::thread sender_thread(udp_sender_thread, std::ref(sender), logger, std::ref(running_flag));

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    running_flag.store(false);
    if (sender_thread.joinable()) {
        sender_thread.join();
    }

    sender.stop();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
