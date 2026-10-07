#include "mino/core/subsystem/subsystem_worker_service.hpp"
#include <iostream>

namespace mino::core::subsystem {

    stderr_sink::stderr_sink() : sink("stderr_sink") {}

    void stderr_sink::log(mino::core::log::tinylog::log_level, std::string_view msg) {
        std::cerr << msg << std::endl;
    }

    worker_service::worker_service() {
        logger_ = std::make_shared<mino::core::log::tinylog::logger>("subsystem_worker");
        logger_->add_sink(std::make_shared<stderr_sink>());
    }

    std::shared_ptr<mino::core::log::tinylog::logger> worker_service::get_logger() const {
        return logger_;
    }

    void worker_service::on_command(const std::string& command, command_handler handler) {
        handlers_[command] = std::move(handler);
    }

    void worker_service::send_raw(const std::string& status, const std::string& b64_payload) {
        std::lock_guard<std::mutex> lock(stdout_mutex_);
        std::cout << status << " " << b64_payload << std::endl;
    }

    void worker_service::notify(const std::string& event_name, const std::string& payload) {
        std::lock_guard<std::mutex> lock(stdout_mutex_);
        // 마스터 식별용 "EVENT:이름" 접두사 부여
        std::cout << protocol::serialize("EVENT:" + event_name, payload) << std::endl;
    }

    void worker_service::run() {
        std::ios_base::sync_with_stdio(true);
        logger_->info("Subsystem worker service initialized.");

        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            packet req_pkt;
            if (!protocol::deserialize(line, req_pkt)) {
                logger_->error("Failed to deserialize request packet.");
                send_raw("ERR", protocol::encode_base64("Failed to parse packet"));
                continue;
            }

            if (req_pkt.command == "QUIT") {
                logger_->info("Received QUIT command. Shutting down.");
                send_raw("OK", protocol::encode_base64("Terminated normally"));
                break;
            }

            auto it = handlers_.find(req_pkt.command);
            if (it != handlers_.end()) {
                auto [status, reply_payload] = it->second(req_pkt.payload);
                send_raw(status, protocol::encode_base64(reply_payload));
            }
            else {
                logger_->warn("Unsupported command: {}", req_pkt.command);
                send_raw("ERR", protocol::encode_base64("Unknown command: " + req_pkt.command));
            }
        }
        logger_->info("Subsystem worker service terminated successfully.");
    }

} // namespace mino::core::subsystem
