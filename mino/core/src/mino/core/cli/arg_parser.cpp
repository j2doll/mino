#include <cmath>
#include <limits>
#include <cstdlib>
#include <cerrno>

#include "mino/core/string/encoding_function.hpp"
#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/string/string_basic.hpp"
#include "mino/core/string/print.hpp"
#include "mino/core/string/u8.hpp"

#include "mino/core/cli/arg_parser.hpp"

namespace mino::core::cli {

    // ================= option_builder =================
    option_builder::option_builder(option_def& opt) : opt_(opt) {}

    option_builder& option_builder::case_sensitive(bool enable) {
        opt_.is_case_sensitive = enable;
        return *this;
    }

    option_builder& option_builder::encoding(string_encoding enc) {
        opt_.encoding = enc;
        return *this;
    }

    option_builder& option_builder::precision(int decimal_places) {
        opt_.precision = decimal_places;
        return *this;
    }

    option_builder& option_builder::allow_inf(bool allow) {
        opt_.allow_inf = allow;
        return *this;
    }

    option_builder& option_builder::allow_nan(bool allow) {
        opt_.allow_nan = allow;
        return *this;
    }

    // ================= arg_parser =================
    arg_parser::arg_parser(std::string program_desc)
        : program_description_(std::move(program_desc)) {
        add_option("-h", "--help", U8("도움말 출력"), false);
    }

    option_builder arg_parser::add_option(const std::string& short_opt,
        const std::string& long_opt,
        const std::string& description,
        bool has_value,
        const std::string& default_val) {
        option_def def;
        def.short_opt = short_opt;
        def.long_opt = long_opt;
        def.description = description;
        def.has_value = has_value;
        def.default_val = default_val;
        def.encoding = string_encoding::utf8;

        options_.push_back(def);

        size_t idx = options_.size() - 1;
        if (!short_opt.empty()) short_map_[short_opt] = idx;
        if (!long_opt.empty())  long_map_[long_opt] = idx;
        if (!default_val.empty()) parsed_values_[long_opt] = default_val;

        return option_builder(options_.back());
    }

    bool arg_parser::parse(int argc, char* argv[]) {
        program_name_ = (argc > 0) ? argv[0] : "app";
        flags_.clear();
        positional_args_.clear();

        for (int i = 1; i < argc; ++i) {
            std::string token = argv[i];

            if (token == "--") {
                for (int j = i + 1; j < argc; ++j) positional_args_.emplace_back(argv[j]);
                break;
            }

            if (string::starts_with(token, "--")) {
                auto eq_pos = token.find('=');
                std::string key = token.substr(0, eq_pos);

                auto it = long_map_.find(key);
                if (it == long_map_.end()) {
                    last_error_ = "Unknown option: " + key;
                    return false;
                }

                const auto& def = options_[it->second];
                if (def.has_value) {
                    if (eq_pos != std::string::npos) {
                        parsed_values_[def.long_opt] = token.substr(eq_pos + 1);
                    }
                    else if (i + 1 < argc && argv[i + 1][0] != '-') {
                        parsed_values_[def.long_opt] = argv[++i];
                    }
                    else {
                        last_error_ = "Option requires an argument: " + key;
                        return false;
                    }
                }
                else {
                    flags_.insert(def.long_opt);
                }
            }
            else if (string::starts_with(token, "-") && token.size() > 1) {
                for (size_t c = 1; c < token.size(); ++c) {
                    std::string key = std::string("-") + token[c];
                    auto it = short_map_.find(key);
                    if (it == short_map_.end()) {
                        last_error_ = "Unknown option: " + key;
                        return false;
                    }

                    const auto& def = options_[it->second];
                    if (def.has_value) {
                        if (c + 1 < token.size()) {
                            parsed_values_[def.long_opt] = token.substr(c + 1);
                        }
                        else if (i + 1 < argc && argv[i + 1][0] != '-') {
                            parsed_values_[def.long_opt] = argv[++i];
                        }
                        else {
                            last_error_ = "Option requires an argument: " + key;
                            return false;
                        }
                        break;
                    }
                    else {
                        flags_.insert(def.long_opt);
                    }
                }
            }
            else {
                positional_args_.emplace_back(token);
            }
        }
        return true;
    }

    bool arg_parser::has_flag(const std::string& long_opt) const {
        return flags_.find(long_opt) != flags_.end();
    }

    const std::vector<std::string>& arg_parser::get_positional() const {
        return positional_args_;
    }

    void arg_parser::print_help() const {
        string::print::println("Usage: {} [options]", program_name_);
        if (!program_description_.empty()) {
            string::print::println("{}\n", program_description_);
        }
        string::print::println("Options:");

        for (const auto& opt : options_) {
            std::string opts = "  ";
            if (!opt.short_opt.empty()) opts += opt.short_opt + ", ";
            else opts += "    ";
            opts += opt.long_opt;
            if (opt.has_value) opts += " <value>";

            std::string padded_opts = string::pad_right(opts, 32);
            std::string desc = opt.description;
            if (!opt.default_val.empty()) {
                desc += " (default: " + opt.default_val + ")";
            }
            string::print::println("{}{}", padded_opts, desc);
        }
    }

    const std::string& arg_parser::get_error() const {
        return last_error_;
    }

    // ================= 타입별 Getter 구현 =================
    std::optional<std::string> arg_parser::get_string(const std::string& long_opt) const {
        auto it = parsed_values_.find(long_opt);
        if (it == parsed_values_.end()) return std::nullopt;

        const auto& def = options_[long_map_.at(long_opt)];
        std::string utf8_val;
        if (!convert_to_utf8(it->second, def.encoding, utf8_val)) {
            return std::nullopt;
        }

        if (!def.is_case_sensitive) {
            return string::to_lower(utf8_val);
        }
        return utf8_val;
    }

    std::optional<bool> arg_parser::get_bool(const std::string& long_opt) const {
        auto it = parsed_values_.find(long_opt);
        if (it == parsed_values_.end()) return std::nullopt;

        std::string s = string::to_lower(it->second);
        if (s == "1" || s == "true" || s == "yes" || s == "on")  return true;
        if (s == "0" || s == "false" || s == "no" || s == "off") return false;
        return std::nullopt;
    }

    std::optional<int64_t> arg_parser::get_int64(const std::string& long_opt) const {
        auto it = parsed_values_.find(long_opt);
        if (it == parsed_values_.end()) return std::nullopt;

        int64_t val = 0;
        if (string::try_parse_int64(it->second, val)) {
            return val;
        }
        return std::nullopt;
    }

    std::optional<int> arg_parser::get_int(const std::string& long_opt) const {
        auto val64 = get_int64(long_opt);
        if (!val64) return std::nullopt;

        if (*val64 < std::numeric_limits<int>::min() || *val64 > std::numeric_limits<int>::max()) {
            return std::nullopt;
        }
        return static_cast<int>(*val64);
    }

    std::optional<float> arg_parser::get_float(const std::string& long_opt) const {
        auto it = parsed_values_.find(long_opt);
        if (it == parsed_values_.end()) return std::nullopt;

        const auto& def = options_[long_map_.at(long_opt)];
        return parse_float(it->second, def);
    }

    std::optional<double> arg_parser::get_double(const std::string& long_opt) const {
        auto it = parsed_values_.find(long_opt);
        if (it == parsed_values_.end()) return std::nullopt;

        const auto& def = options_[long_map_.at(long_opt)];
        return parse_double(it->second, def);
    }

    // ================= 실수 전용 파서 구현 =================
    std::optional<float> arg_parser::parse_float(const std::string& str, const option_def& def) const {
        std::string lower_s = string::to_lower(str);

        bool is_negative = (!lower_s.empty() && lower_s[0] == '-');
        std::string_view sv = lower_s;
        if (!sv.empty() && (sv[0] == '+' || sv[0] == '-')) {
            sv.remove_prefix(1);
        }

        if (sv == "inf" || sv == "infinity") {
            if (!def.allow_inf) return std::nullopt;
            float inf_val = std::numeric_limits<float>::infinity();
            return is_negative ? -inf_val : inf_val;
        }

        if (sv == "nan") {
            if (!def.allow_nan) return std::nullopt;
            float nan_val = std::numeric_limits<float>::quiet_NaN();
            return is_negative ? -nan_val : nan_val;
        }

        char* end_ptr = nullptr;
        errno = 0;
        float val = std::strtof(str.c_str(), &end_ptr);
        if (end_ptr == str.c_str() || *end_ptr != '\0' || errno != 0) {
            return std::nullopt;
        }

        if (std::isinf(val) && !def.allow_inf) return std::nullopt;
        if (std::isnan(val) && !def.allow_nan) return std::nullopt;

        if (!std::isinf(val) && !std::isnan(val) && def.precision.has_value() && *def.precision >= 0) {
            float factor = std::pow(10.0f, static_cast<float>(*def.precision));
            val = std::round(val * factor) / factor;
        }

        return val;
    }

    std::optional<double> arg_parser::parse_double(const std::string& str, const option_def& def) const {
        std::string lower_s = string::to_lower(str);

        bool is_negative = (!lower_s.empty() && lower_s[0] == '-');
        std::string_view sv = lower_s;
        if (!sv.empty() && (sv[0] == '+' || sv[0] == '-')) {
            sv.remove_prefix(1);
        }

        if (sv == "inf" || sv == "infinity") {
            if (!def.allow_inf) return std::nullopt;
            double inf_val = std::numeric_limits<double>::infinity();
            return is_negative ? -inf_val : inf_val;
        }

        if (sv == "nan") {
            if (!def.allow_nan) return std::nullopt;
            double nan_val = std::numeric_limits<double>::quiet_NaN();
            return is_negative ? -nan_val : nan_val;
        }

        double val = 0.0;
        if (!string::try_parse_double(str, val)) {
            return std::nullopt;
        }

        if (std::isinf(val) && !def.allow_inf) return std::nullopt;
        if (std::isnan(val) && !def.allow_nan) return std::nullopt;

        if (!std::isinf(val) && !std::isnan(val) && def.precision.has_value() && *def.precision >= 0) {
            double factor = std::pow(10.0, *def.precision);
            val = std::round(val * factor) / factor;
        }

        return val;
    }

    bool arg_parser::convert_to_utf8(const std::string& input, string_encoding enc, std::string& out) {
        switch (enc) {
        case string_encoding::utf8:
            out = input;
            return true;
        case string_encoding::cp949:
            return string::cp949_to_utf8(input, out);
        case string_encoding::iso2022kr:
            return string::iso2022kr_to_utf8(input, out);
        case string_encoding::johab:
            return string::johab_to_utf8(input, out);
        case string_encoding::mackorean:
            return string::mackorean_to_utf8(input, out);
        case string_encoding::console:
            out = string::from_console_encoding(input);
            return true;
        }
        return false;
    }

} // namespace mino::core::cli
