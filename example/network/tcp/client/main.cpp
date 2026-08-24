#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/termination_handler.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/tcp/tcp_client.hpp"

// Custom handler class for TCP client events
class my_tcp_client_handler {
public:
    my_tcp_client_handler(std::string name) {
        std::string logger_name = name + "_logger";
        auto console_sink = std::make_shared<mino::core::log::tinylog::console_sink>(name + "_console");
        logger_ = std::make_shared<mino::core::log::tinylog::logger>(logger_name);
        logger_->add_sink(console_sink);
        mino::core::log::tinylog::logger::register_logger(logger_);
    }

    void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
        logger_ = logger;
    }

    void on_connect() {
        if (logger_)
            logger_->info(" Connected to server!");
    }

    void on_close() {
        if (logger_)
            logger_->info("Disconnected!");
    }

    void on_receive(const std::string& data) {
        if (logger_)
            logger_->info(" Received: {}", data);
        std::vector<uint8_t> byte_array(data.begin(), data.end()); // Convert string to byte array
    }

protected:
    std::shared_ptr<mino::core::log::tinylog::logger> logger_;
};

void clean_up_resources(
    mino::network::tcp::tcp_client* tcp4_client,
    mino::network::tcp::tcp_client* tcp6_client)
{
    tcp4_client->stop();
    tcp6_client->stop();
}

// Example for both IPv4 and IPv6
int main() {
    mino::network::sock mnsock;

    auto& handler = mino::core::daemon::termination_handler::get_instance();
    handler.initialize();

    using tcp_client = mino::network::tcp::tcp_client;
    tcp_client client4;
    tcp_client client6;

    handler.set_callback([&client4, &client6]() {
        clean_up_resources(&client4, &client6);
        std::exit(0);
    });

    namespace mclt = mino::core::log::tinylog;

    // ----------- IPv4 Example -----------
    {
        auto console_sink4 = std::make_shared<mclt::console_sink>("tcp4_console");
        auto tcp4_logger = std::make_shared<mclt::logger>("tcp4_logger");
        tcp4_logger->add_sink(console_sink4);
        mclt::logger::register_logger(tcp4_logger);

        client4.set_logger(tcp4_logger);

        tcp4_logger->info("[IPv4 Example]");

        client4.set_server("127.0.0.1", 12345, AF_INET); // Set server IP and port, IPv4

        auto handler = std::make_shared<my_tcp_client_handler>("tcp4_handler");
        client4.set_on_connect([handler]() { handler->on_connect(); });
        client4.set_on_close([handler]() { handler->on_close(); });
        client4.set_on_receive([handler](const std::string& data) { handler->on_receive(data); });

        // Sleep duration between connection attempts
        auto sleep_duration = std::chrono::seconds(60);

        if (!client4.start(sleep_duration)) {
            tcp4_logger->error("Failed to start IPv4 client");
            return 1;
        }

        // [Test 1] Simulate the main thread handling its own loop
        for (int i = 0; i < 3; ++i) { // Run 3 times for demonstration
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (client4.is_connected()) {
                // Get current time as string
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
#ifdef _WIN32
                localtime_s(&tm, &now_c);
#else
                localtime_r(&now_c, &tm);
#endif
                std::ostringstream oss;
                oss << "Hello World (IPv4) " << std::put_time(&tm, "%H:%M:%S");
                if (client4.send_data(oss.str()) < 0) { // Send data to server
                    tcp4_logger->error("Failed to send data.");
                }
                else {
                    tcp4_logger->info("  Sent: {}", oss.str());
                }
            }
        }

        tcp4_logger->info("Before stopping IPv4 client, waiting for a moment...");
        client4.stop();
        tcp4_logger->info("IPv4 client stopped.");
    }

    // ----------- IPv6 Example -----------
    {
        auto console_sink6 = std::make_shared<mclt::console_sink>("tcp6_console");
        auto tcp6_logger = std::make_shared<mclt::logger>("tcp6_logger");
        tcp6_logger->add_sink(console_sink6);
        mclt::logger::register_logger(tcp6_logger);

        client6.set_logger(tcp6_logger);

        tcp6_logger->info("[IPv6 Example]");

        client6.set_server("::1", 12346, AF_INET6); // Set server IP and port, IPv6

        auto handler = std::make_shared<my_tcp_client_handler>("tcp6_handler");
        client6.set_on_connect([handler]() { handler->on_connect(); });
        client6.set_on_close([handler]() { handler->on_close(); });
        client6.set_on_receive([handler](const std::string& data) { handler->on_receive(data); });

        // Sleep duration between connection attempts
        auto sleep_duration = std::chrono::seconds(60);

        if (!client6.start(sleep_duration)) {
            tcp6_logger->error("Failed to start IPv6 client");
            return 1;
        }

        // [Test 1] Simulate the main thread handling its own loop
        for (int i = 0; i < 3; ++i) { // Run 3 times for demonstration
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (client6.is_connected()) {
                // Get current time as string
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
#ifdef _WIN32
                localtime_s(&tm, &now_c);
#else
                localtime_r(&now_c, &tm);
#endif
                std::ostringstream oss;
                oss << "Hello World (IPv6) " << std::put_time(&tm, "%H:%M:%S");
                if (client6.send_data(oss.str()) < 0) { // Send data to server
                    tcp6_logger->error("Failed to send data.");
                }
                else {
                    tcp6_logger->info("  Sent: {}", oss.str());
                }
            }
        }

        tcp6_logger->info("Before stopping IPv6 client, waiting for a moment...");
        client6.stop();
        tcp6_logger->info("IPv6 client stopped.");
    }

    return 0;
}
