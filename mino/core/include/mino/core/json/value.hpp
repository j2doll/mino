#pragma once

#include "json_fwd.hpp"
#include "value_type.hpp"
#include <string>
#include <variant>

namespace mino::core::json {

    class value {
    public:
        using variant_t = std::variant<
            std::monostate,
            bool,
            double,
            std::string,
            array_t,
            object_t
        >;

        variant_t data;

        value() noexcept;
        value(bool val) noexcept;
        value(double val) noexcept;
        value(int val) noexcept;
        value(const std::string& val);
        value(std::string&& val) noexcept;
        value(const char* val);
        value(const array_t& val);
        value(array_t&& val) noexcept;
        value(const object_t& val);
        value(object_t&& val) noexcept;

        value_type get_type() const noexcept;

        bool is_null() const noexcept;
        bool is_bool() const noexcept;
        bool is_number() const noexcept;
        bool is_string() const noexcept;
        bool is_array() const noexcept;
        bool is_object() const noexcept;

        double get_number(double default_val = 0.0) const noexcept;
        bool get_bool(bool default_val = false) const noexcept;
        const std::string& get_string(const std::string& default_val = "") const noexcept;

        value& operator[](const std::string& key) noexcept;
        value& operator[](size_t index) noexcept;
    };

} // namespace mino::core::json

