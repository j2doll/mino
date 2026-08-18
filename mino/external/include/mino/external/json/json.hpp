#pragma once

#include <string>
#include <type_traits>

#include <nlohmann/json.hpp>

namespace mino::external::json {

    using nj  = nlohmann::json;
    using njj = nlohmann::json::json_pointer;

    // Node access (defined in .cpp)
    const nj* get_node(const nj& j, const njj& ptr) noexcept;
    nj* get_node_mutable(nj& j, const njj& ptr) noexcept;

    // Numeric conversion template (defined in .cpp and explicitly instantiated there)
    template <class T>
    bool try_number_cast(const nj& node, T& out) noexcept;

    // value_or overloads (defined in .cpp)
    std::string value_or(const nj& j, const njj& ptr, const std::string& defval) noexcept;
    bool value_or(const nj& j, const njj& ptr, bool defval) noexcept;

    template <class T>
    T value_or(const nj& j, const njj& ptr, T defval) noexcept;

    // value_or_path overloads (defined in .cpp)
    std::string value_or_path(const nj& j, const std::string& path, const std::string& defval) noexcept;
    bool value_or_path(const nj& j, const std::string& path, bool defval) noexcept;

    template <class T>
    T value_or_path(const nj& j, const std::string& path, T defval) noexcept;

    // Existence and convenience getters (defined in .cpp)
    bool exists(const nj& j, const std::string& path) noexcept;

    std::string get_string(const nj& j, const std::string& path, const std::string& defval) noexcept;
    int         get_int   (const nj& j, const std::string& path, int defval) noexcept;
    bool        get_bool  (const nj& j, const std::string& path, bool defval) noexcept;
    double      get_double(const nj& j, const std::string& path, double defval) noexcept;

} // namespace mino::external::json
