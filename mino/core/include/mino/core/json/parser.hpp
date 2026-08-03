#pragma once

#include <string_view>
#include <string>

#include "mino/core/json/json_fwd.hpp"

namespace mino::core::json {

    class parser {
    public:
        static value parse(std::string_view src) noexcept;

    private:
        static void skip_whitespace(std::string_view src, size_t& idx) noexcept;
        static value parse_value(std::string_view src, size_t& idx) noexcept;
        static value parse_null(std::string_view src, size_t& idx) noexcept;
        static value parse_bool(std::string_view src, size_t& idx) noexcept;
        static value parse_number(std::string_view src, size_t& idx) noexcept;
        static std::string parse_raw_string(std::string_view src, size_t& idx) noexcept;
        static value parse_string(std::string_view src, size_t& idx) noexcept;
        static value parse_array(std::string_view src, size_t& idx) noexcept;
        static value parse_object(std::string_view src, size_t& idx) noexcept;
    };

} // namespace mino::core::json


