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

int main() {
    mino::network::sock mnsock;

    auto& handler = mino::core::daemon::termination_handler::get_instance();
    handler.initialize();

    using tcp_client = mino::network::tcp::tcp_client;
    tcp_client client4;
    tcp_client client6;

    // Ctrl+C 또는 종료 시그널 수신 시 안전하게 리소스 해제
    handler.set_callback([&client4, &client6]() {
        clean_up_resources(&client4, &client6);
        std::exit(0);
        });

    namespace mclt = mino::core::log::tinylog;

    // ----------- 1. IPv4 Client 설정 및 시작 -----------
    auto console_sink4 = std::make_shared<mclt::console_sink>("tcp4_console");
    auto tcp4_logger = std::make_shared<mclt::logger>("tcp4_logger");
    tcp4_logger->add_sink(console_sink4);
    mclt::logger::register_logger(tcp4_logger);

    client4.set_logger(tcp4_logger);
    tcp4_logger->info("[IPv4 Example] Initializing...");

    client4.set_server("127.0.0.1", 12345, AF_INET);

    auto handler4 = std::make_shared<my_tcp_client_handler>("tcp4_handler");
    client4.set_on_connect([handler4]() { handler4->on_connect(); });
    client4.set_on_close([handler4]() { handler4->on_close(); });
    client4.set_on_receive([handler4](const std::string& data) { handler4->on_receive(data); });

    // IPv4 재연결 설정: 초기 10초, 최대 60초 도달 후 60초 간격으로 무한 재시도 (max_retries = 0)
    mino::network::tcp::reconnect_config recon_cfg4;
    recon_cfg4.initial_interval = std::chrono::seconds(10);
    recon_cfg4.max_interval = std::chrono::seconds(60);
    recon_cfg4.max_retries = 0;
    recon_cfg4.backoff_multiplier = 2.0;

    if (!client4.start(recon_cfg4)) {
        tcp4_logger->error("Failed to start IPv4 client");
        return 1;
    }

    // ----------- 2. IPv6 Client 설정 및 시작 -----------
    auto console_sink6 = std::make_shared<mclt::console_sink>("tcp6_console");
    auto tcp6_logger = std::make_shared<mclt::logger>("tcp6_logger");
    tcp6_logger->add_sink(console_sink6);
    mclt::logger::register_logger(tcp6_logger);

    client6.set_logger(tcp6_logger);
    tcp6_logger->info("[IPv6 Example] Initializing...");

    client6.set_server("::1", 12346, AF_INET6);

    auto handler6 = std::make_shared<my_tcp_client_handler>("tcp6_handler");
    client6.set_on_connect([handler6]() { handler6->on_connect(); });
    client6.set_on_close([handler6]() { handler6->on_close(); });
    client6.set_on_receive([handler6](const std::string& data) { handler6->on_receive(data); });

    // IPv6 재연결 설정: 초기 10초, 최대 60초 도달 후 60초 간격으로 무한 재시도 (max_retries = 0)
    mino::network::tcp::reconnect_config recon_cfg6;
    recon_cfg6.initial_interval = std::chrono::seconds(10);
    recon_cfg6.max_interval = std::chrono::seconds(60);
    recon_cfg6.max_retries = 0;
    recon_cfg6.backoff_multiplier = 2.0;

    if (!client6.start(recon_cfg6)) {
        tcp6_logger->error("Failed to start IPv6 client");
        return 1;
    }

    // ----------- 3. 메인 모니터링 루프 (두 클라이언트 동시 관찰) -----------
    tcp4_logger->info("Both IPv4 & IPv6 clients are running.");
    tcp4_logger->info("Observing reconnection loop (Press Ctrl+C to terminate)...");

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &now_c);
#else
        localtime_r(&now_c, &tm);
#endif

        // IPv4 연결 시 데이터 전송
        if (client4.is_connected()) {
            std::ostringstream oss;
            oss << "Hello World (IPv4) " << std::put_time(&tm, "%H:%M:%S");
            if (client4.send_data(oss.str()) < 0) {
                tcp4_logger->error("Failed to send IPv4 data.");
            }
            else {
                tcp4_logger->info("  Sent: {}", oss.str());
            }
        }

        // IPv6 연결 시 데이터 전송
        if (client6.is_connected()) {
            std::ostringstream oss;
            oss << "Hello World (IPv6) " << std::put_time(&tm, "%H:%M:%S");
            if (client6.send_data(oss.str()) < 0) {
                tcp6_logger->error("Failed to send IPv6 data.");
            }
            else {
                tcp6_logger->info("  Sent: {}", oss.str());
            }
        }
    }

    return 0;
}
