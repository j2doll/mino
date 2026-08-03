
#include "mino/core/json/value.hpp"

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

} // namespace mino::core::json
