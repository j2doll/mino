#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/subsystem/subsystem_protocol.hpp"

namespace mino::core::subsystem {

    class stderr_sink : public mino::core::log::tinylog::sink {
    public:
        stderr_sink();
        ~stderr_sink() override = default;
        void log(mino::core::log::tinylog::log_level level, std::string_view msg) override;
    };

    class worker_service {
    public:
        using command_handler = std::function<std::pair<std::string, std::string>(const std::string& request_payload)>;

        worker_service();
        ~worker_service() = default;

        std::shared_ptr<mino::core::log::tinylog::logger> get_logger() const;
        void on_command(const std::string& command, command_handler handler);

        // 워커가 마스터에게 자율적으로 비동기 이벤트 전송
        void notify(const std::string& event_name, const std::string& payload);

        void run();

    private:
        void send_raw(const std::string& status, const std::string& b64_payload);

        std::mutex stdout_mutex_;
        std::unordered_map<std::string, command_handler> handlers_;
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;
    };

} // namespace mino::core::subsystem
