#include <sstream>
#include <string>

#include "mino/core/json/serializer.hpp"
#include "mino/core/json/value.hpp"

namespace mino::core::json {

namespace {
    // 숫자를 기존 동작과 비슷하게 스트림으로 포맷
    static std::string number_to_string(double d) {
        std::ostringstream ss;
        ss << d;
        return ss.str();
    }
} // anonymous

static void serialize_compact(const value& val, std::string& out) {
    if (val.is_null()) {
        out += "null";
    }
    else if (val.is_bool()) {
        out += (std::get<bool>(val.data) ? "true" : "false");
    }
    else if (val.is_number()) {
        out += number_to_string(std::get<double>(val.data));
    }
    else if (val.is_string()) {
        out += "\"";
        out += std::get<std::string>(val.data);
        out += "\"";
    }
    else if (val.is_array()) {
        out += "[";
        const auto& arr = std::get<array_t>(val.data);
        for (size_t i = 0; i < arr.size(); ++i) {
            serialize_compact(arr[i], out);
            if (i + 1 < arr.size()) out += ",";
        }
        out += "]";
    }
    else if (val.is_object()) {
        out += "{";
        const auto& obj = std::get<object_t>(val.data);
        size_t count = 0;
        for (const auto& [key, v] : obj) {
            out += "\"";
            out += key;
            out += "\":";
            serialize_compact(v, out);
            if (++count < obj.size()) out += ",";
        }
        out += "}";
    }
}

static void serialize_pretty(const value& val, int indent, int level, std::string& out) {
    auto append_indent = [&](int lv) {
        out.append(lv * indent, ' ');
    };

    if (val.is_null()) {
        out += "null";
    }
    else if (val.is_bool()) {
        out += (std::get<bool>(val.data) ? "true" : "false");
    }
    else if (val.is_number()) {
        out += number_to_string(std::get<double>(val.data));
    }
    else if (val.is_string()) {
        out += "\"";
        out += std::get<std::string>(val.data);
        out += "\"";
    }
    else if (val.is_array()) {
        const auto& arr = std::get<array_t>(val.data);
        if (arr.empty()) {
            out += "[]";
            return;
        }
        out += "[\n";
        for (size_t i = 0; i < arr.size(); ++i) {
            append_indent(level + 1);
            serialize_pretty(arr[i], indent, level + 1, out);
            if (i + 1 < arr.size()) out += ",";
            out += "\n";
        }
        append_indent(level);
        out += "]";
    }
    else if (val.is_object()) {
        const auto& obj = std::get<object_t>(val.data);
        if (obj.empty()) {
            out += "{}";
            return;
        }
        out += "{\n";
        size_t count = 0;
        for (const auto& [key, v] : obj) {
            append_indent(level + 1);
            out += "\"";
            out += key;
            out += "\": ";
            serialize_pretty(v, indent, level + 1, out);
            if (++count < obj.size()) out += ",";
            out += "\n";
        }
        append_indent(level);
        out += "}";
    }
}

std::string serializer::serialize(const value& val, int indent) noexcept {
    try {
        std::string out;
        if (indent <= 0) {
            serialize_compact(val, out);
        }
        else {
            serialize_pretty(val, indent, 0, out);
        }
        return out;
    }
    catch (...) {
        return std::string(); // noexcept 보장: 실패 시 빈 문자열 반환
    }
}

} // namespace mino::core::json
