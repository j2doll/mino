#include <sstream>

#include "mino/core/json/serializer.hpp"
#include "mino/core/json/value.hpp"

namespace mino::core::json {

    std::string serializer::serialize(const value& val) noexcept {
        std::stringstream ss;
        if (val.is_null()) {
            ss << "null";
        }
        else if (val.is_bool()) {
            ss << (std::get<bool>(val.data) ? "true" : "false");
        }
        else if (val.is_number()) {
            ss << std::get<double>(val.data);
        }
        else if (val.is_string()) {
            ss << "\"" << std::get<std::string>(val.data) << "\"";
        }
        else if (val.is_array()) {
            ss << "[";
            const auto& arr = std::get<array_t>(val.data);
            for (size_t i = 0; i < arr.size(); ++i) {
                ss << serialize(arr[i]);
                if (i + 1 < arr.size()) ss << ",";
            }
            ss << "]";
        }
        else if (val.is_object()) {
            ss << "{";
            const auto& obj = std::get<object_t>(val.data);
            size_t count = 0;
            for (const auto& [key, v] : obj) {
                ss << "\"" << key << "\":" << serialize(v);
                if (++count < obj.size()) ss << ",";
            }
            ss << "}";
        }
        return ss.str();
    }

} // namespace mino::core::json
