#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <cstddef>
#include <cstdint>

#include <spdlog/spdlog.h>

namespace mino::external::log::factory {

    class tiny_logger; // forward declaration

    class  logger_registry {
    public:
        static logger_registry& instance(); 

        // 멤버 함수: 레벨 인자를 명시적으로 받음 (헤더에 기본값 없음)
        void create(const std::string& name, ::spdlog::level::level_enum lvl,
            bool console, bool file, const std::string& path,
            std::size_t max_size, std::uint32_t max_files);

        void create_by_max_size(const std::string& name, ::spdlog::level::level_enum lvl,
            bool console, bool file, const std::string& path,
            std::size_t max_size, std::uint32_t max_files);

        void create_by_retention_days(const std::string& name, ::spdlog::level::level_enum lvl,
            bool console, bool file, const std::string& path,
            std::uint32_t retention_days, std::uint16_t rot_h, std::uint16_t rot_m);

        tiny_logger& get(const std::string& name);

    private:
        logger_registry() = default;
        std::unordered_map<std::string, std::shared_ptr<tiny_logger>> registry_;
    };

    // 전역 헬퍼: 레벨을 명시하는 오버로드 (헤더에서는 기본 레벨 값을 제공하지 않음)
    void create(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console = true, bool file = false, const std::string& path = "",
        std::size_t max_size = 0, std::uint32_t max_files = 7);

    void create_by_max_size(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console = true, bool file = false, const std::string& path = "",
        std::size_t max_size = 0, std::uint32_t max_files = 7);

    void create_by_retention_days(const std::string& name, ::spdlog::level::level_enum lvl,
        bool console = true, bool file = false, const std::string& path = "",
        std::uint32_t retention_days = 7, std::uint16_t rot_h = 0, std::uint16_t rot_m = 0);

    // 전역 헬퍼: 레벨을 명시하지 않는 편의 오버로드 (구현에서 기본 레벨 사용)
    void create(const std::string& name,
        bool console = true, bool file = false, const std::string& path = "",
        std::size_t max_size = 0, std::uint32_t max_files = 7);

    void create_by_max_size(const std::string& name,
        bool console = true, bool file = false, const std::string& path = "",
        std::size_t max_size = 0, std::uint32_t max_files = 7);

    void create_by_retention_days(const std::string& name,
        bool console = true, bool file = false, const std::string& path = "",
        std::uint32_t retention_days = 7, std::uint16_t rot_h = 0, std::uint16_t rot_m = 0);

    tiny_logger& get(const std::string& name);

}  
