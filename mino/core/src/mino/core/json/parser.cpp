#include <cctype>
#include <cstdlib>

#include "mino/core/json/parser.hpp"
#include "mino/core/json/value.hpp"

namespace mino::core::json {

    value parser::parse(std::string_view src) noexcept {
        size_t idx = 0;
        skip_whitespace(src, idx);
        return parse_value(src, idx);
    }

    void parser::skip_whitespace(std::string_view src, size_t& idx) noexcept {
        while (idx < src.size() && (src[idx] == ' ' || src[idx] == '\t' || src[idx] == '\n' || src[idx] == '\r')) {
            ++idx;
        }
    }

    value parser::parse_value(std::string_view src, size_t& idx) noexcept {
        skip_whitespace(src, idx);
        if (idx >= src.size()) return {};

        char c = src[idx];
        if (c == 'n') return parse_null(src, idx);
        if (c == 't' || c == 'f') return parse_bool(src, idx);
        if (c == '"') return parse_string(src, idx);
        if (c == '[') return parse_array(src, idx);
        if (c == '{') return parse_object(src, idx);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number(src, idx);

        return {};
    }

    value parser::parse_null(std::string_view src, size_t& idx) noexcept {
        if (src.substr(idx, 4) == "null") {
            idx += 4;
            return {};
        }
        return {};
    }

    value parser::parse_bool(std::string_view src, size_t& idx) noexcept {
        if (src.substr(idx, 4) == "true") {
            idx += 4;
            return value(true);
        }
        if (src.substr(idx, 5) == "false") {
            idx += 5;
            return value(false);
        }
        return {};
    }

    value parser::parse_number(std::string_view src, size_t& idx) noexcept {
        size_t end_idx = idx;
        if (end_idx < src.size() && (src[end_idx] == '-' || src[end_idx] == '+')) {
            ++end_idx;
        }
        while (end_idx < src.size()) {
            char c = src[end_idx];
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                ++end_idx;
            } else {
                break;
            }
        }

        std::string num_str(src.substr(idx, end_idx - idx));
        char* end_ptr = nullptr;
        double val = std::strtod(num_str.c_str(), &end_ptr);

        if (end_ptr != num_str.c_str()) {
            idx += (end_ptr - num_str.c_str());
            return value(val);
        }

        ++idx;
        return {};
    }

    std::string parser::parse_raw_string(std::string_view src, size_t& idx) noexcept {
        ++idx;
        size_t start = idx;
        while (idx < src.size() && src[idx] != '"') {
            if (src[idx] == '\\' && idx + 1 < src.size()) ++idx;
            ++idx;
        }
        std::string res(src.substr(start, idx - start));
        if (idx < src.size()) ++idx;
        return res;
    }

    value parser::parse_string(std::string_view src, size_t& idx) noexcept {
        return value(parse_raw_string(src, idx));
    }

    value parser::parse_array(std::string_view src, size_t& idx) noexcept {
        ++idx;
        array_t arr;
        skip_whitespace(src, idx);

        if (idx < src.size() && src[idx] == ']') {
            ++idx;
            return value(std::move(arr));
        }

        while (idx < src.size()) {
            arr.push_back(parse_value(src, idx));
            skip_whitespace(src, idx);
            if (idx < src.size() && src[idx] == ',') {
                ++idx;
            }
            else if (idx < src.size() && src[idx] == ']') {
                ++idx;
                break;
            }
        }
        return value(std::move(arr));
    }

    value parser::parse_object(std::string_view src, size_t& idx) noexcept {
        ++idx;
        object_t obj;
        skip_whitespace(src, idx);

        if (idx < src.size() && src[idx] == '}') {
            ++idx;
            return value(std::move(obj));
        }

        while (idx < src.size()) {
            skip_whitespace(src, idx);
            if (src[idx] != '"') break;
            std::string key = parse_raw_string(src, idx);

            skip_whitespace(src, idx);
            if (idx < src.size() && src[idx] == ':') ++idx;

            obj[key] = parse_value(src, idx);

            skip_whitespace(src, idx);
            if (idx < src.size() && src[idx] == ',') {
                ++idx;
            }
            else if (idx < src.size() && src[idx] == '}') {
                ++idx;
                break;
            }
        }
        return value(std::move(obj));
    }

} // namespace mino::core::json