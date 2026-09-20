#include "mino/network_openssl/redis/redis_value.hpp"

namespace mino::network_openssl::redis {

    bool redis_value::is_null() const noexcept {
        return type == redis_type::null_value;
    }

    bool redis_value::is_error() const noexcept {
        return type == redis_type::error;
    }

    std::optional<std::string> redis_value::as_string() const {
        if (std::holds_alternative<std::string>(data)) {
            return std::get<std::string>(data);
        }
        return std::nullopt;
    }

    std::optional<int64_t> redis_value::as_integer() const {
        if (std::holds_alternative<int64_t>(data)) {
            return std::get<int64_t>(data);
        }
        return std::nullopt;
    }

    const std::vector<redis_value>* redis_value::as_array() const {
        if (std::holds_alternative<std::vector<redis_value>>(data)) {
            return &std::get<std::vector<redis_value>>(data);
        }
        return nullptr;
    }

}  
