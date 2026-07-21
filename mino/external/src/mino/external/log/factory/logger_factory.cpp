#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "mino/external/log/factory/logger_factory.hpp"
#include "mino/external/log/factory/tiny_logger.hpp"

namespace mino::external::log::factory {

    logger_registry& logger_registry::instance() {
        static logger_registry inst;
        return inst;
    }

    // 멤버 구현 (레벨 인자 필요)
    void logger_registry::create(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {

        auto logger = std::make_shared<tiny_logger>(name);
        logger->set_log_level(lvl);
        logger->setup(console, file, path, "", max_size, max_files);
        registry_[name] = logger;
    }

    void logger_registry::create_by_max_size(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {

        auto logger = std::make_shared<tiny_logger>(name);
        logger->set_log_level(lvl);
        logger->setup(console, file, path, "", max_size, max_files);
        registry_[name] = logger;
    }

    void logger_registry::create_by_retention_days(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::uint32_t retention_days, std::uint16_t rot_h, std::uint16_t rot_m) {

        auto logger = std::make_shared<tiny_logger>(name);
        logger->set_log_level(lvl);
        logger->setup(console, file, path, "", 0, retention_days, rot_h, rot_m);
        registry_[name] = logger;
    }

    tiny_logger& logger_registry::get(const std::string& name) {
        auto it = registry_.find(name);
        if (it == registry_.end()) {
            static tiny_logger fallback("fallback");
            fallback.set_log_level(::spdlog::level::off);
            return fallback;
        }
        return *(it->second);
    }

    // 전역 헬퍼: 레벨을 명시하는 오버로드 구현
    void create(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {
        logger_registry::instance().create(name, lvl, console, file, path, max_size, max_files);
    }

    void create_by_max_size(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {
        logger_registry::instance().create_by_max_size(name, lvl, console, file, path, max_size, max_files);
    }

    void create_by_retention_days(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console, bool file, const std::string& path,
        std::uint32_t retention_days, std::uint16_t rot_h, std::uint16_t rot_m) {
        logger_registry::instance().create_by_retention_days(name, lvl, console, file, path, retention_days, rot_h, rot_m);
    }

    // 전역 헬퍼: 레벨을 명시하지 않는 편의 오버로드는 기존대로 기본값을 여기에서 사용
    void create(const std::string& name,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {
        logger_registry::instance().create(name, ::spdlog::level::info, console, file, path, max_size, max_files);
    }

    void create_by_max_size(const std::string& name,
        bool console, bool file, const std::string& path,
        std::size_t max_size, std::uint32_t max_files) {
        logger_registry::instance().create_by_max_size(name, ::spdlog::level::info, console, file, path, max_size, max_files);
    }

    void create_by_retention_days(const std::string& name,
        bool console, bool file, const std::string& path,
        std::uint32_t retention_days, std::uint16_t rot_h, std::uint16_t rot_m) {
        logger_registry::instance().create_by_retention_days(name, ::spdlog::level::info, console, file, path, retention_days, rot_h, rot_m);
    }

    tiny_logger& get(const std::string& name) {
        return logger_registry::instance().get(name);
    }

}
