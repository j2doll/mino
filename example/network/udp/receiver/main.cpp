#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <memory>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/daemon.hpp"
#include "mino/core/log/log.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/network.hpp"

// my_udp_receiver_handler class definition
class my_udp_receiver_handler {
public:
    my_udp_receiver_handler(const std::string& name) : name_(name) {
        std::string logger_name = name + "_logger";
        auto console_sink = std::make_shared<mino::core::log::tinylog::console_sink>(name + "_console");
        logger_ = std::make_shared<mino::core::log::tinylog::logger>(logger_name);
        logger_->add_sink(console_sink);
        mino::core::log::tinylog::logger::register_logger(logger_);
    }

    void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        logger_ = logger;
    }

    void onReceive(const std::string& message, const std::string& ip, uint16_t port) {
        if (logger_) {
            logger_->info("[{}] Received from {}:{} - {}", name_, ip, port, message);
        }
    }

private:
    std::string name_;
    std::shared_ptr<mino::core::log::tinylog::logger> logger_;
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

    namespace mclt = mino::core::log::tinylog;

    // 메인 로거 생성
    auto main_console_sink = std::make_shared<mclt::console_sink>("main_console");
    auto main_logger = std::make_shared<mclt::logger>("main_logger");
    main_logger->add_sink(main_console_sink);
    mclt::logger::register_logger(main_logger);

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

    // 각 리시버에 로거 등록
    unicast4_receiver.set_logger(main_logger);
    unicast6_receiver.set_logger(main_logger);
    multicast4_receiver.set_logger(main_logger);
    multicast6_receiver.set_logger(main_logger);
    broadcast4_receiver.set_logger(main_logger);

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
        main_logger->error("Failed to start IPv4 UDP unicast receiver");
    }
    else {
        main_logger->info("<bright_green>[Unicast IPv4]</bright_green> UDP Receiver running on 0.0.0.0:12345");
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
        main_logger->error("Failed to start IPv6 UDP unicast receiver");
    }
    else {
        main_logger->info("<bright_green>[Unicast IPv6]</bright_green> UDP Receiver running on [::]:12346");
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
        main_logger->error("Failed to start IPv4 UDP multicast receiver");
    }
    else {
        main_logger->info("<bright_green>[Multicast IPv4]</bright_green> UDP Receiver running on 239.255.255.250:12347");
    }

    // IPv6 Multicast
    multicast6_receiver.set_ip_version(mino::network::udp::udp_receiver::ip_version_t::ipv6);
    my_udp_receiver_handler multicast6_handler("multicast6_handler");
    multicast6_receiver.set_on_receive_callback(
        [&multicast6_handler](const std::string& message, const std::string& ip, uint16_t port) {
            multicast6_handler.onReceive(message, ip, port);
        }
    );
    if (!multicast6_receiver.start_multicast("ff3e::1234", 12348)) {
        main_logger->error("Failed to start IPv6 UDP multicast receiver");
    }
    else {
        main_logger->info("<bright_green>[Multicast IPv6]</bright_green> UDP Receiver running on ff3e::1234:12348");
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
        main_logger->error("Failed to start IPv4 UDP broadcast receiver");
    }
    else {
        main_logger->info("<bright_green>[Broadcast IPv4]</bright_green> UDP Receiver running on port 12349");
    }

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

    main_logger->info("All UDP Receivers running. Type 'quit' to stop.");

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
