#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>

namespace mino::external::log::factory {

    class  tiny_logger {
    public:
        explicit tiny_logger(std::string name);

        void set_log_level(::spdlog::level::level_enum level);

        void setup(bool use_console, bool use_file, const std::string& file_name = "",
            const std::string& pattern = "",
            size_t max_size = 0, uint32_t max_files = 7,
            uint16_t rot_h = 0, uint16_t rot_m = 0);

        // Use a plain format parameter to remain compatible with older spdlog versions
        template <typename... Args>
        void operator()(const char* fmt, Args &&...args) {
            if (auto l = ::spdlog::get(name_)) l->log(level_, fmt, std::forward<Args>(args)...);
        }

        struct stream_proxy {
            std::shared_ptr<::spdlog::logger> logger;
            ::spdlog::level::level_enum level;
            std::ostringstream oss;

            stream_proxy(std::shared_ptr<::spdlog::logger> l, ::spdlog::level::level_enum lvl);

            // 복사 방지
            stream_proxy(const stream_proxy&) = delete;
            stream_proxy& operator=(const stream_proxy&) = delete;

            // 이동 생성자/대입 연산자
            stream_proxy(stream_proxy&& other) noexcept;
            stream_proxy& operator=(stream_proxy&& other) noexcept;

            ~stream_proxy();

            template <typename T> stream_proxy& operator<<(const T& msg) { oss << msg; return *this; }
        };

        template <typename T>
        stream_proxy operator<<(const T& msg) {
            stream_proxy p(::spdlog::get(name_), level_);
            p << msg;
            return p;
        }

    private:
        std::string name_;
        ::spdlog::level::level_enum level_;
    };

}  
