#pragma once

#include <string_view>

#include "mino/network/redis/redis_value.hpp"

namespace mino::network::redis {

    class resp_parser {
    public:
        static bool parse(std::string_view buffer, redis_value& out_value, size_t& consumed_bytes);

    private:
        static bool read_line(std::string_view src, size_t offset, std::string_view& line, size_t& line_total_len);
        static bool parse_internal(std::string_view src, redis_value& out_value, size_t& consumed);
    };

} // namespace mino::network::redis
