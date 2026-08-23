#include <iostream>
#include <string>
#include <thread>
#include <atomic>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/daemon.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/network.hpp"

// my_udp_receiver_handler class definition
class my_udp_receiver_handler {
public:
    my_udp_receiver_handler(const std::string& name) : name_(name) {}
    void onReceive(const std::string& message, const std::string& ip, uint16_t port) {
        std::string ret =
            "[" + name_ + "] Received from " +
            ip + ":" + std::to_string(port) + " - " + message;
        std::cout << ret << std::endl;
    }
private: 
    std::string name_;
};

void run_receiver(mino::network::udp::udp_receiver& receiver, std::atomic<bool>& stop_flag) {
    while (!stop_flag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    receiver.quit();
}

void clean_up_resources(
    mino::network::udp::udp_receiver* unicast4_receiver,
    mino::network::udp::udp_receiver* unicast6_receiver,
    mino::network::udp::udp_receiver* multicast4_receiver,
    mino::network::udp::udp_receiver* multicast6_receiver,
    mino::network::udp::udp_receiver* broadcast4_receiver
)
{
    unicast4_receiver->quit();
    unicast6_receiver->quit();
    multicast4_receiver->quit();
    multicast6_receiver->quit();
    broadcast4_receiver->quit();
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }
#endif

    namespace mcd = mino::core::daemon;
    using termination_handler = mcd::termination_handler;
    auto& handler = termination_handler::get_instance();
    handler.initialize();

    namespace mnu = mino::network::udp;
    using udp_receiver = mnu::udp_receiver;

    udp_receiver unicast4_receiver;
    udp_receiver unicast6_receiver;
    udp_receiver multicast4_receiver;
    udp_receiver multicast6_receiver;
    udp_receiver broadcast4_receiver;

    // Register termination callback to clean up resources on exit (Ctrl+C)
    handler.set_callback([&unicast4_receiver, &unicast6_receiver, &multicast4_receiver, &multicast6_receiver, &broadcast4_receiver]() {
        clean_up_resources(&unicast4_receiver, &unicast6_receiver, &multicast4_receiver, &multicast6_receiver, &broadcast4_receiver);
#ifdef _WIN32
        WSACleanup();
#endif
        std::exit(0);
    });

    // IPv4 Unicast
    unicast4_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv4);
    my_udp_receiver_handler unicast4_handler("unicast4_handler");
    unicast4_receiver.set_on_receive_callback(
        [&unicast4_handler](const std::string& message, const std::string& ip, uint16_t port) {
            unicast4_handler.onReceive(message, ip, port);
        }
    );
    if (!unicast4_receiver.start_unicast("0.0.0.0", 12345)) {
        std::cerr << "Failed to start IPv4 UDP unicast receiver" << std::endl;
    }
    else {
        std::cout << "[Unicast IPv4] UDP Receiver running on 0.0.0.0:12345" << std::endl;
    }

    // IPv6 Unicast
    unicast6_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv6);
    my_udp_receiver_handler unicast6_handler("unicast6_handler");
    unicast6_receiver.set_on_receive_callback(
        [&unicast6_handler](const std::string& message, const std::string& ip, uint16_t port) {
            unicast6_handler.onReceive(message, ip, port);
        }
    );
    if (!unicast6_receiver.start_unicast("::", 12346)) {
        std::cerr << "Failed to start IPv6 UDP unicast receiver" << std::endl;
    }
    else {
        std::cout << "[Unicast IPv6] UDP Receiver running on [::]:12346" << std::endl;
    }

    // IPv4 Multicast
    multicast4_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv4);
    my_udp_receiver_handler multicast4_handler("multicast4_handler");
    multicast4_receiver.set_on_receive_callback(
        [&multicast4_handler](const std::string& message, const std::string& ip, uint16_t port) {
            multicast4_handler.onReceive(message, ip, port);
        }
    );
    if (!multicast4_receiver.start_multicast("239.255.255.250", 12347)) {
        std::cerr << "Failed to start IPv4 UDP multicast receiver" << std::endl;
    }
    else {
        std::cout << "[Multicast IPv4] UDP Receiver running on 239.255.255.250:12347" << std::endl;
    }

    // IPv6 Multicast
    multicast6_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv6);
    my_udp_receiver_handler multicast6_handler("multicast6_handler");
    multicast6_receiver.set_on_receive_callback(
        [&multicast6_handler](const std::string& message, const std::string& ip, uint16_t port) {
            multicast6_handler.onReceive(message, ip, port);
        }
    );
    // Use a random, valid IPv6 multicast address for demonstration
    if (!multicast6_receiver.start_multicast("ff3e::1234", 12348)) {
        std::cerr << "Failed to start IPv6 UDP multicast receiver" << std::endl;
    }
    else {
        std::cout << "[Multicast IPv6] UDP Receiver running on ff3e::1234:12348" << std::endl;
    }

    // IPv4 Broadcast
    broadcast4_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv4);
    my_udp_receiver_handler broadcast4_handler("broadcast4_handler");
    broadcast4_receiver.set_on_receive_callback(
        [&broadcast4_handler](const std::string& message, const std::string& ip, uint16_t port) {
            broadcast4_handler.onReceive(message, ip, port);
        }
    );
    if (!broadcast4_receiver.start_broadcast(12349)) {
        std::cerr << "Failed to start IPv4 UDP broadcast receiver" << std::endl;
    }
    else {
        std::cout << "[Broadcast IPv4] UDP Receiver running on port 12349" << std::endl;
    }

    // NOTE: ipv6 is not supported broadcast. So we skip ipv6 broadcast receiver.

    // Run all receivers in separate threads for demonstration
    std::atomic<bool> stop_flag(false);
    std::thread t1(run_receiver, std::ref(unicast4_receiver), std::ref(stop_flag));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t2(run_receiver, std::ref(unicast6_receiver), std::ref(stop_flag));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t3(run_receiver, std::ref(multicast4_receiver), std::ref(stop_flag));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t4(run_receiver, std::ref(multicast6_receiver), std::ref(stop_flag));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t5(run_receiver, std::ref(broadcast4_receiver), std::ref(stop_flag));

    std::cout << "All UDP Receivers running. Type 'quit' to stop." << std::endl;

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit") break;
    }

    stop_flag = true;

    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();
    if (t3.joinable()) t3.join();
    if (t4.joinable()) t4.join();
    if (t5.joinable()) t5.join();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}




