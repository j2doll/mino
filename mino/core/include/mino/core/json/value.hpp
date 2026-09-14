#pragma once

#include "mino/core/json/json_fwd.hpp"
#include "mino/core/json/value_type.hpp"
#include <string>
#include <memory>
#include <variant>

namespace mino::core::json {

    class value {
    public:
        struct impl;

        value() noexcept;
        ~value();
        value(const value& other);
        value(value&& other) noexcept;
        value& operator=(const value& other);
        value& operator=(value&& other) noexcept;

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

        const array_t& get_array() const noexcept;
        array_t& get_array() noexcept;
        const object_t& get_object() const noexcept;
        object_t& get_object() noexcept;

        value& operator[](const std::string& key) noexcept;
        value& operator[](size_t index) noexcept;

        bool has_path(const std::string& path) const noexcept;

        impl& get_impl() noexcept { return *pimpl_; }
        const impl& get_impl() const noexcept { return *pimpl_; }

    private:
        std::unique_ptr<impl> pimpl_;
    };

    // value 클래스가 완전히 정의되었으므로, array_t와 object_t를 안전하게 인스턴스화할 수 있습니다.
    struct value::impl {
        using variant_t = std::variant<
            std::monostate,
            bool,
            double,
            std::string,
            array_t,
            object_t
        >;

        variant_t data;

        impl() noexcept : data(std::monostate{}) {}
        impl(variant_t v) : data(std::move(v)) {}
    };

    using variant_t = value::impl::variant_t;

} // namespace mino::core::json