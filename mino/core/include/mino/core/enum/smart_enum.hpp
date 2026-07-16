#pragma once

#include <string_view>
#include <array>
#include <optional>

namespace mino::core::enums {

    template <typename T>
    struct EnumEntry {
        T value;
        std::string_view name;
    };

    namespace internal {
        // Simple compile-time string trimmer
        constexpr std::string_view trim(std::string_view str) {
            while (!str.empty() && (str.front() == ' ' || str.front() == '\t' || str.front() == '\n' || str.front() == '\r')) str.remove_prefix(1);
            while (!str.empty() && (str.back() == ' ' || str.back() == '\t' || str.back() == '\n' || str.back() == '\r')) str.remove_suffix(1);
            return str;
        }

        // Count how many enumerators exist by counting commas
        constexpr size_t count_elements(std::string_view str) {
            if (str.empty()) return 0;
            size_t count = 1;
            for (char c : str) { if (c == ',') count++; }
            return count;
        }

        // Full compile-time parser for "Name = Value, Name2" strings
        template <typename T, size_t N>
        constexpr auto parse_entries(std::string_view raw) {
            std::array<EnumEntry<T>, N> result{};
            size_t idx = 0;
            size_t start = 0;
            int auto_val = 0;

            for (size_t i = 0; i <= raw.size(); ++i) {
                if (i == raw.size() || raw[i] == ',') {
                    std::string_view segment = trim(raw.substr(start, i - start));
                    if (!segment.empty()) {
                        std::string_view name = segment;
                        size_t eq_pos = segment.find('=');

                        if (eq_pos != std::string_view::npos) {
                            name = trim(segment.substr(0, eq_pos));
                            std::string_view val_str = trim(segment.substr(eq_pos + 1));

                            // Simple compile-time base-10 integer parser
                            int parsed_val = 0;
                            bool sign = false;
                            if (!val_str.empty() && val_str.front() == '-') { sign = true; val_str.remove_prefix(1); }
                            for (char c : val_str) {
                                if (c >= '0' && c <= '9') parsed_val = parsed_val * 10 + (c - '0');
                            }
                            auto_val = sign ? -parsed_val : parsed_val;
                        }

                        result[idx++] = EnumEntry<T>{ static_cast<T>(auto_val), name };
                        auto_val++;
                    }
                    start = i + 1;
                }
            }
            return result;
        }

        template <typename T> struct EnumMap;
    }

    // Public Meta Interface
    template <typename T>
    constexpr auto enum_entries() {
        return internal::EnumMap<T>::values;
    }

    template <typename T>
    constexpr std::string_view enum_name(T value) {
        for (const auto& entry : internal::EnumMap<T>::values) {
            if (entry.value == value) return entry.name;
        }
        return "";
    }

    template <typename T>
    constexpr std::optional<T> enum_cast(std::string_view name) {
        for (const auto& entry : internal::EnumMap<T>::values) {
            if (entry.name == name) return entry.value;
        }
        return std::nullopt;
    }

} // namespace mino::core::enums

// ==========================================
// [User API] Super Simple Macros
// ==========================================

#define DEFINE_ENUM_NAMESPACE(Namespace, EnumName, ...) \
    namespace Namespace { \
        enum class EnumName { __VA_ARGS__ }; \
    } \
    namespace mino::core::enums::internal { \
        template <> struct EnumMap<::Namespace::EnumName> { \
            static constexpr size_t size = count_elements(#__VA_ARGS__); \
            static constexpr auto values = parse_entries<::Namespace::EnumName, size>(#__VA_ARGS__); \
        }; \
    }

#define DEFINE_ENUM_GLOBAL(EnumName, ...) \
    enum class EnumName { __VA_ARGS__ }; \
    namespace mino::core::enums::internal { \
        template <> struct EnumMap<::EnumName> { \
            static constexpr size_t size = count_elements(#__VA_ARGS__); \
            static constexpr auto values = parse_entries<::EnumName, size>(#__VA_ARGS__); \
        }; \
    }

    