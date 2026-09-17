#include "mino/network/redis/resp_parser.hpp"

#include <charconv>

namespace mino::network::redis {

    bool resp_parser::parse(std::string_view buffer, redis_value& out_value, size_t& consumed_bytes) {
        consumed_bytes = 0;
        return parse_internal(buffer, out_value, consumed_bytes);
    }

    bool resp_parser::read_line(std::string_view src, size_t offset, std::string_view& line, size_t& line_total_len) {
        size_t crlf_pos = src.find("\r\n", offset);
        if (crlf_pos == std::string_view::npos) return false;

        line = src.substr(offset, crlf_pos - offset);
        line_total_len = (crlf_pos - offset) + 2;
        return true;
    }

    bool resp_parser::parse_internal(std::string_view src, redis_value& out_value, size_t& consumed) {
        if (src.size() <= consumed) return false;

        char prefix = src[consumed];
        size_t current_consumed = consumed + 1;

        switch (prefix) {
        case '+':
        case '-': {
            std::string_view line;
            size_t line_len = 0;
            if (!read_line(src, current_consumed, line, line_len)) return false;

            out_value.type = (prefix == '+') ? redis_type::simple_string : redis_type::error;
            out_value.data = std::string(line);
            consumed = current_consumed + line_len;
            return true;
        }
        case ':': {
            std::string_view line;
            size_t line_len = 0;
            if (!read_line(src, current_consumed, line, line_len)) return false;

            int64_t val = 0;
            auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), val);
            if (ec != std::errc()) return false;

            out_value.type = redis_type::integer;
            out_value.data = val;
            consumed = current_consumed + line_len;
            return true;
        }
        case '$': {
            std::string_view line;
            size_t line_len = 0;
            if (!read_line(src, current_consumed, line, line_len)) return false;

            int64_t str_len = 0;
            auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), str_len);
            if (ec != std::errc()) return false;

            current_consumed += line_len;

            if (str_len == -1) {
                out_value.type = redis_type::null_value;
                out_value.data = std::monostate{};
                consumed = current_consumed;
                return true;
            }

            if (src.size() < current_consumed + static_cast<size_t>(str_len) + 2) {
                return false;
            }

            out_value.type = redis_type::bulk_string;
            out_value.data = std::string(src.substr(current_consumed, str_len));
            consumed = current_consumed + static_cast<size_t>(str_len) + 2;
            return true;
        }
        case '*': {
            std::string_view line;
            size_t line_len = 0;
            if (!read_line(src, current_consumed, line, line_len)) return false;

            int64_t array_count = 0;
            auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), array_count);
            if (ec != std::errc()) return false;

            current_consumed += line_len;

            if (array_count == -1) {
                out_value.type = redis_type::null_value;
                out_value.data = std::monostate{};
                consumed = current_consumed;
                return true;
            }

            std::vector<redis_value> elements;
            elements.reserve(array_count);

            for (int64_t i = 0; i < array_count; ++i) {
                redis_value elem;
                if (!parse_internal(src, elem, current_consumed)) {
                    return false;
                }
                elements.push_back(std::move(elem));
            }

            out_value.type = redis_type::array;
            out_value.data = std::move(elements);
            consumed = current_consumed;
            return true;
        }
        default:
            return false;
        }
    }

} // namespace mino::network::redis
