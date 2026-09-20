#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>

namespace mino::network_openssl::redis {

    enum class redis_type {
        simple_string,
        error,
        integer,
        bulk_string,
        array,
        null_value
    };

    struct redis_value {
        redis_type type{ redis_type::null_value };
        std::variant<std::monostate, std::string, int64_t, std::vector<redis_value>> data;

        bool is_null() const noexcept;
        bool is_error() const noexcept;

        std::optional<std::string> as_string() const;
        std::optional<int64_t> as_integer() const;
        const std::vector<redis_value>* as_array() const;
    };

}  
