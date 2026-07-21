#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "mino/external/log/factory/tiny_logger.hpp"

namespace mino::external::log::factory {

    tiny_logger::tiny_logger(std::string name)
        : name_(std::move(name)), level_(::spdlog::level::info) {
    }

    void tiny_logger::set_log_level(::spdlog::level::level_enum level) {
        level_ = level;
        if (auto l = ::spdlog::get(name_)) l->set_level(level_);
    }

    void tiny_logger::setup(bool use_console, bool use_file, const std::string& file_name,
        const std::string& pattern, size_t max_size, uint32_t max_files,
        uint16_t rot_h, uint16_t rot_m) {

        std::vector<::spdlog::sink_ptr> sinks;
        if (use_console) sinks.push_back(std::make_shared<::spdlog::sinks::stdout_color_sink_mt>());

        if (use_file && !file_name.empty()) {
            if (max_size > 0) {
                sinks.push_back(std::make_shared<::spdlog::sinks::rotating_file_sink_mt>(file_name, max_size, max_files));
            }
            else {
                sinks.push_back(std::make_shared<::spdlog::sinks::daily_file_sink_mt>(file_name, rot_h, rot_m, false, max_files));
            }
        }

        auto logger = std::make_shared<::spdlog::logger>(name_, sinks.begin(), sinks.end());
        logger->set_level(level_);
        if (!pattern.empty()) logger->set_pattern(pattern);
        ::spdlog::register_logger(logger);
    }

    // stream_proxy 구현
    tiny_logger::stream_proxy::stream_proxy(std::shared_ptr<::spdlog::logger> l, ::spdlog::level::level_enum lvl)
        : logger(l), level(lvl) {
    }

    tiny_logger::stream_proxy::stream_proxy(stream_proxy&& other) noexcept
        : logger(std::move(other.logger)), level(other.level), oss(std::move(other.oss)) {
        other.logger = nullptr;
    }

    tiny_logger::stream_proxy& tiny_logger::stream_proxy::operator=(stream_proxy&& other) noexcept {
        if (this != &other) {
            logger = std::move(other.logger);
            level = other.level;
            oss = std::move(other.oss);
            other.logger = nullptr;
        }
        return *this;
    }

    tiny_logger::stream_proxy::~stream_proxy() {
        if (logger) logger->log(level, oss.str());
    }

}
