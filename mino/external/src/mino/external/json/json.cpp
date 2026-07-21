#include <nlohmann/json.hpp> 

#include "mino/external/json/json.hpp"

namespace mino::external::json {

    // 내부 헬퍼 구현
    const nj* get_node(const nj& j, const njj& ptr) noexcept {
        try {
            const nj& n = j.at(ptr);
            return &n;
        }
        catch (...) {
            return nullptr;
        }
    }

    nj* get_node_mutable(nj& j, const njj& ptr) noexcept {
        try {
            nj& n = j.at(ptr);
            return &n;
        }
        catch (...) {
            return nullptr;
        }
    }

    // 숫자 변환 템플릿 정의
    template <class T>
    bool try_number_cast(const nj& node, T& out) noexcept {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
        if (!node.is_number()) return false;

        if constexpr (std::is_floating_point_v<T>) {
            double v = node.get<double>();
            if (std::isnan(v) || std::isinf(v)) return false;
            out = static_cast<T>(v);
            return true;
        }
        else {
            long double v = node.is_number_float()
                ? static_cast<long double>(node.get<double>())
                : static_cast<long double>(node.get<long long>());
            if (std::isnan(v) || std::isinf(v)) return false;
            if (node.is_number_float() && std::floor(v) != v) return false;
            const long double lo = static_cast<long double>(std::numeric_limits<T>::min());
            const long double hi = static_cast<long double>(std::numeric_limits<T>::max());
            if (v < lo || v > hi) return false;
            out = static_cast<T>(v);
            return true;
        }
    }

    // value_or 구현
    std::string value_or(const nj& j, const njj& ptr, const std::string& defval) noexcept {
        if (const nj* n = get_node(j, ptr)) {
            if (n->is_string()) return n->get<std::string>();
        }
        return defval;
    }

    bool value_or(const nj& j, const njj& ptr, bool defval) noexcept {
        if (const nj* n = get_node(j, ptr)) {
            if (n->is_boolean()) return n->get<bool>();
        }
        return defval;
    }

    template <class T>
    T value_or(const nj& j, const njj& ptr, T defval) noexcept {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
        if (const nj* n = get_node(j, ptr)) {
            T out{};
            if (try_number_cast<T>(*n, out)) return out;
        }
        return defval;
    }

    // value_or_path 구현
    std::string value_or_path(const nj& j, const std::string& path, const std::string& defval) noexcept {
        try {
            njj ptr(path);
            return value_or(j, ptr, defval);
        }
        catch (...) {
            return defval;
        }
    }

    bool value_or_path(const nj& j, const std::string& path, bool defval) noexcept {
        try {
            njj ptr(path);
            return value_or(j, ptr, defval);
        }
        catch (...) {
            return defval;
        }
    }

    template <class T>
    T value_or_path(const nj& j, const std::string& path, T defval) noexcept {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
        try {
            njj ptr(path);
            return value_or<T>(j, ptr, defval);
        }
        catch (...) {
            return defval;
        }
    }

    // 외부 API 구현 (기존)
    bool exists(const nj& j, const std::string& path) noexcept {
        try {
            njj ptr(path);
            return get_node(j, ptr) != nullptr;
        }
        catch (...) {
            return false;
        }
    }

    std::string get_string(const nj& j,
        const std::string& path,
        const std::string& defval) noexcept {
        return value_or_path(j, path, defval);
    }

    int get_int(const nj& j,
        const std::string& path,
        int defval) noexcept {
        return value_or_path<int>(j, path, defval);
    }

    bool get_bool(const nj& j,
        const std::string& path,
        bool defval) noexcept {
        return value_or_path(j, path, defval);
    }

    double get_double(const nj& j,
        const std::string& path,
        double defval) noexcept {
        return value_or_path<double>(j, path, defval);
    }

    // 명시적 템플릿 인스턴스화 (필요 타입 추가 가능)
    template bool try_number_cast<int>(const nj& node, int& out) noexcept;
    template bool try_number_cast<long long>(const nj& node, long long& out) noexcept;
    template bool try_number_cast<double>(const nj& node, double& out) noexcept;

    template int value_or<int>(const nj& j, const njj& ptr, int defval) noexcept;
    template long long value_or<long long>(const nj& j, const njj& ptr, long long defval) noexcept;
    template double value_or<double>(const nj& j, const njj& ptr, double defval) noexcept;

    template int value_or_path<int>(const nj& j, const std::string& path, int defval) noexcept;
    template long long value_or_path<long long>(const nj& j, const std::string& path, long long defval) noexcept;
    template double value_or_path<double>(const nj& j, const std::string& path, double defval) noexcept;

} // namespace mino::core::json
