#include "mino/core/json/value.hpp"
#include <string_view>
#include <cctype>

namespace mino::core::json {

    value::value() noexcept : pimpl_(std::make_unique<impl>(std::monostate{})) {}
    value::~value() = default;
    value::value(value&& other) noexcept = default;
    value& value::operator=(value&& other) noexcept = default;

    value::value(const value& other)
        : pimpl_(other.pimpl_ ? std::make_unique<impl>(*other.pimpl_) : std::make_unique<impl>()) {}

    value& value::operator=(const value& other) {
        if (this != &other) {
            pimpl_ = other.pimpl_ ? std::make_unique<impl>(*other.pimpl_) : std::make_unique<impl>();
        }
        return *this;
    }

    value::value(bool val) noexcept : pimpl_(std::make_unique<impl>(val)) {}
    value::value(double val) noexcept : pimpl_(std::make_unique<impl>(val)) {}
    value::value(int val) noexcept : pimpl_(std::make_unique<impl>(static_cast<double>(val))) {}
    value::value(const std::string& val) : pimpl_(std::make_unique<impl>(val)) {}
    value::value(std::string&& val) noexcept : pimpl_(std::make_unique<impl>(std::move(val))) {}
    value::value(const char* val) : pimpl_(std::make_unique<impl>(std::string(val))) {}
    value::value(const array_t& val) : pimpl_(std::make_unique<impl>(val)) {}
    value::value(array_t&& val) noexcept : pimpl_(std::make_unique<impl>(std::move(val))) {}
    value::value(const object_t& val) : pimpl_(std::make_unique<impl>(val)) {}
    value::value(object_t&& val) noexcept : pimpl_(std::make_unique<impl>(std::move(val))) {}

    value_type value::get_type() const noexcept {
        return static_cast<value_type>(pimpl_->data.index());
    }

    bool value::is_null() const noexcept { return std::holds_alternative<std::monostate>(pimpl_->data); }
    bool value::is_bool() const noexcept { return std::holds_alternative<bool>(pimpl_->data); }
    bool value::is_number() const noexcept { return std::holds_alternative<double>(pimpl_->data); }
    bool value::is_string() const noexcept { return std::holds_alternative<std::string>(pimpl_->data); }
    bool value::is_array() const noexcept { return std::holds_alternative<array_t>(pimpl_->data); }
    bool value::is_object() const noexcept { return std::holds_alternative<object_t>(pimpl_->data); }

    double value::get_number(double default_val) const noexcept {
        if (auto* p = std::get_if<double>(&pimpl_->data)) return *p;
        return default_val;
    }

    bool value::get_bool(bool default_val) const noexcept {
        if (auto* p = std::get_if<bool>(&pimpl_->data)) return *p;
        return default_val;
    }

    const std::string& value::get_string(const std::string& default_val) const noexcept {
        if (auto* p = std::get_if<std::string>(&pimpl_->data)) return *p;
        return default_val;
    }

    const array_t& value::get_array() const noexcept {
        static const array_t empty_arr{};
        if (auto* p = std::get_if<array_t>(&pimpl_->data)) return *p;
        return empty_arr;
    }

    array_t& value::get_array() noexcept {
        if (!is_array()) {
            pimpl_->data = array_t{};
        }
        return std::get<array_t>(pimpl_->data);
    }

    const object_t& value::get_object() const noexcept {
        static const object_t empty_obj{};
        if (auto* p = std::get_if<object_t>(&pimpl_->data)) return *p;
        return empty_obj;
    }

    object_t& value::get_object() noexcept {
        if (!is_object()) {
            pimpl_->data = object_t{};
        }
        return std::get<object_t>(pimpl_->data);
    }

    value& value::operator[](const std::string& key) noexcept {
        if (!is_object()) {
            pimpl_->data = object_t{};
        }
        return std::get<object_t>(pimpl_->data)[key];
    }

    value& value::operator[](size_t index) noexcept {
        static value dummy;
        if (is_array()) {
            auto& arr = std::get<array_t>(pimpl_->data);
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
        if (i < n && path[i] == '/') ++i;

        while (i <= n) {
            size_t j = i;
            while (j < n && path[j] != '/') ++j;
            size_t len = (j > i) ? (j - i) : 0;

            if (len == 0) {
                if (j >= n) break;
                i = j + 1;
                continue;
            }

            std::string_view comp(path.data() + i, len);

            if (cur->is_object()) {
                const auto& obj = std::get<object_t>(cur->pimpl_->data);
                auto it = obj.find(std::string(comp));
                if (it == obj.end()) return false;
                cur = &it->second;
            } else if (cur->is_array()) {
                size_t idx = 0;
                for (char c : comp) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                    idx = idx * 10 + static_cast<size_t>(c - '0');
                }
                const auto& arr = std::get<array_t>(cur->pimpl_->data);
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