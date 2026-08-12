#include "mino/core/json/value.hpp"
#include <string_view>
#include <cctype>

namespace mino::core::json {

    value::value() noexcept : data(std::monostate{}) {}
    value::value(bool val) noexcept : data(val) {}
    value::value(double val) noexcept : data(val) {}
    value::value(int val) noexcept : data(static_cast<double>(val)) {}
    value::value(const std::string& val) : data(val) {}
    value::value(std::string&& val) noexcept : data(std::move(val)) {}
    value::value(const char* val) : data(std::string(val)) {}
    value::value(const array_t& val) : data(val) {}
    value::value(array_t&& val) noexcept : data(std::move(val)) {}
    value::value(const object_t& val) : data(val) {}
    value::value(object_t&& val) noexcept : data(std::move(val)) {}

    value_type value::get_type() const noexcept {
        return static_cast<value_type>(data.index());
    }

    bool value::is_null() const noexcept { return std::holds_alternative<std::monostate>(data); }
    bool value::is_bool() const noexcept { return std::holds_alternative<bool>(data); }
    bool value::is_number() const noexcept { return std::holds_alternative<double>(data); }
    bool value::is_string() const noexcept { return std::holds_alternative<std::string>(data); }
    bool value::is_array() const noexcept { return std::holds_alternative<array_t>(data); }
    bool value::is_object() const noexcept { return std::holds_alternative<object_t>(data); }

    double value::get_number(double default_val) const noexcept {
        if (auto* p = std::get_if<double>(&data)) return *p;
        return default_val;
    }

    bool value::get_bool(bool default_val) const noexcept {
        if (auto* p = std::get_if<bool>(&data)) return *p;
        return default_val;
    }

    const std::string& value::get_string(const std::string& default_val) const noexcept {
        if (auto* p = std::get_if<std::string>(&data)) return *p;
        return default_val;
    }

    value& value::operator[](const std::string& key) noexcept {
        if (!is_object()) {
            data = object_t{};
        }
        return std::get<object_t>(data)[key];
    }

    value& value::operator[](size_t index) noexcept {
        static value dummy;
        if (is_array()) {
            auto& arr = std::get<array_t>(data);
            if (index < arr.size()) {
                return arr[index];
            }
        }
        return dummy;
    }

    bool value::has_path(const std::string& path) const noexcept {
        const value* cur = this;
        size_t n = path.size();
        size_t i = 0;
        // 허용: 선행 '/' 무시
        if (i < n && path[i] == '/') ++i;

        while (i <= n) {
            size_t j = i;
            while (j < n && path[j] != '/') ++j;
            size_t len = (j > i) ? (j - i) : 0;

            if (len == 0) {
                // 빈 컴포넌트는 건너뜀 (연속된 슬래시 또는 끝의 슬래시)
                if (j >= n) break;
                i = j + 1;
                continue;
            }

            std::string_view comp(path.data() + i, len);

            if (cur->is_object()) {
                const auto& obj = std::get<object_t>(cur->data);
                auto it = obj.find(std::string(comp));
                if (it == obj.end()) return false;
                cur = &it->second;
            } else if (cur->is_array()) {
                // 배열 인덱스는 숫자 문자열이어야 함
                size_t idx = 0;
                for (char c : comp) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                    idx = idx * 10 + static_cast<size_t>(c - '0');
                }
                const auto& arr = std::get<array_t>(cur->data);
                if (idx >= arr.size()) return false;
                cur = &arr[idx];
            } else {
                return false;
            }

            if (j >= n) break;
            i = j + 1;
        }

        return true;
    }

} // namespace mino::core::json
