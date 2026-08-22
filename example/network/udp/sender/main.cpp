#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/daemon.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/network.hpp"

void udp_sender_thread(mino::network::udp::udp_sender& sender, std::atomic<bool>& running_flag) {

    mino::network::udp::udp_sender sender6;
    bool reuse_address6 = true;
    bool use_multicast6 = true;
    if (!sender6.create(AF_INET6, reuse_address6, use_multicast6)) {
        std::cerr << "Failed to create UDP sender for IPv6" << std::endl;
        return;
    }

    while (running_flag.load()) { // Loop until the running_flag is set to false

        // NOTE: send_data_to does not use setting of server ip and port. only use parameters.
        sender.send_data_to("IPv4 Hello Unicast", "127.0.0.1", 12345); // Send unicast
        std::cout << "IPv4 unicast sent at 127.0.0.1:12345" << std::endl;

        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); // for testing

        sender.send_data_to("IPv4 Hello Multicast", "239.255.255.250", 12347); // Send multicast 
        std::cout << "IPv4 multicast sent at 239.255.255.250:12347" << std::endl;

        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); // for testing

        sender.send_data_to("IPv4 Hello Broadcast", "255.255.255.255", 12349); // Broadcast transmission
        std::cout << "IPv4 broadcast sent at 255.255.255.255:12349" << std::endl;

        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); // for testing

        //----------------------------
        // IPv6

        auto loopback6 = "::1"; // This is equivalent to the above, but some systems may require the full form for certain operations.
        auto ret_l6 = sender6.send_data_to("Hello Unicast IPv6", loopback6, 12346, AF_INET6); // Send unicast IPv6
        std::cout << "IPv6 unicast sent at ::1 :12346 -> " << ret_l6 << std::endl;

        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); // for testing

        auto ret_m6 = sender6.send_data_to("Hello Multicast IPv6", "ff3e::1234", 12348, AF_INET6); // Send multicast IPv6
            std::cout << "IPv6 multicast sent at ff3e::1234 :12348 -> " << ret_m6 << std::endl;
 
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
        return (-1);
    }
#endif

    namespace mcd = mino::core::daemon;
    auto& handler = mcd::termination_handler::get_instance();
    handler.initialize();

    mino::network::udp::udp_sender sender;

    // this handler will be called,
    // when the process receives termination signals
    // (e.g., SIGINT, SIGTERM, Ctrl+C)
    handler.set_callback([&sender]() {
        clean_up_resources(&sender);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
    });

    // sender.set_server("127.0.0.1", 12345);

    bool reuse_address = true;
    bool use_multicast = true;
    if (!sender.create(AF_INET, reuse_address, use_multicast)) {
        std::cerr << "Failed to create UDP sender" << std::endl;
        return 1;
    }
    if (!sender.set_multicast_ttl(2)) { // Multicast Egress TTL Settings (Optional)
        std::cerr << "Failed to set multicast TTL" << std::endl;
    }

    std::atomic<bool> running_flag(true);
    std::thread sender_thread(udp_sender_thread, std::ref(sender), std::ref(running_flag));

    // Main thread loop with 10-second sleep
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait a bit
        // std::cout << "Main thread is running..." << std::endl;

        // It can be set to stop sending when certain conditions are met.
        // example> if (condition) { break; }

        /*
        auto ret = sender.send_data("Hello Unicast (from main thread)"); // Send unicast using the set server IP and port
        if (ret < 0) {
            std::cerr << "Failed to send data from main thread" << std::endl;
        }
        else {
            std::cout << "Unicast sent at 127.0.0.1:12345" << std::endl;
        }
        //*/
    }

    // Processing at the end of the program
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


