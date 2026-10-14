#include "mino/core/file/file_size_formatter.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace mino::core::file {

    std::string file_size_formatter::byte_to_string(uint64_t bytes, metric_type metric, int precision) noexcept {
        const double base = static_cast<double>(metric);
        const char* suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };

        if (bytes == 0) return "0 B";

        int i = static_cast<int>(std::floor(std::log(bytes) / std::log(base)));
        if (i >= 7) i = 6;

        double value = bytes / std::pow(base, i);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision);
        ss << value;

        std::string str = ss.str();
        if (str.find('.') != std::string::npos) {
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (str.back() == '.') str.pop_back();
        }

        return str + " " + suffixes[i];
    }

    std::optional<uint64_t> file_size_formatter::string_to_bytes(std::string_view input_str, metric_type metric) noexcept {
        std::string clean_str;
        clean_str.reserve(input_str.size());
        for (char ch : input_str) {
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                clean_str.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            }
        }

        if (clean_str.empty()) return std::nullopt;

        // Find unit start (first alpha char)
        size_t pos = 0;
        while (pos < clean_str.size() && (std::isdigit(static_cast<unsigned char>(clean_str[pos])) || clean_str[pos] == '.')) {
            ++pos;
        }

        if (pos == 0) return std::nullopt; // no numeric part

        std::string numpart = clean_str.substr(0, pos);
        std::string unit = clean_str.substr(pos);

        if (numpart.empty()) return std::nullopt;

        // Validate numeric format (at most one '.')
        size_t dot_count = std::count(numpart.begin(), numpart.end(), '.');
        if (dot_count > 1) return std::nullopt;

        // split integer and fractional parts
        std::string int_part_s;
        std::string frac_part_s;
        auto dot_pos = numpart.find('.');
        if (dot_pos == std::string::npos) {
            int_part_s = numpart;
        }
        else {
            int_part_s = numpart.substr(0, dot_pos);
            frac_part_s = numpart.substr(dot_pos + 1);
        }

        if (int_part_s.empty()) int_part_s = "0";

        // limit fractional digits to 9 to keep intermediate safe
        if (frac_part_s.size() > 9) frac_part_s = frac_part_s.substr(0, 9);

        // parse integer part safely
        uint64_t int_part = 0;
        try {
            size_t idx = 0;
            int_part = std::stoull(int_part_s, &idx);
            if (idx != int_part_s.size()) return std::nullopt;
        }
        catch (...) {
            return std::nullopt;
        }

        uint64_t frac_numer = 0;
        uint64_t frac_pow10 = 1;
        if (!frac_part_s.empty()) {
            try {
                frac_numer = std::stoull(frac_part_s);
                for (size_t i = 0; i < frac_part_s.size(); ++i) frac_pow10 *= 10ULL;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        // determine exponent from unit
        int exponent = 0;
        if (unit.empty() || unit == "B") exponent = 0;
        else if (unit == "KB") exponent = 1;
        else if (unit == "MB") exponent = 2;
        else if (unit == "GB") exponent = 3;
        else if (unit == "TB") exponent = 4;
        else if (unit == "PB") exponent = 5;
        else if (unit == "EB") exponent = 6;
        else return std::nullopt;

        const uint64_t MAX_U64 = std::numeric_limits<uint64_t>::max();
        // compute multiplier as uint64_t safely
        uint64_t base = static_cast<uint64_t>(metric);
        uint64_t multiplier = 1;
        for (int i = 0; i < exponent; ++i) {
            // check overflow when multiplying multiplier * base
            if (multiplier > 0 && base > 0 && multiplier > MAX_U64 / base) {
                return std::nullopt;
            }
            multiplier *= base;
        }

        // check integer part overflow: int_part * multiplier
        if (int_part != 0 && multiplier > 0 && int_part > MAX_U64 / multiplier) {
            return std::nullopt;
        }
        uint64_t integer_bytes = int_part * multiplier;

        // compute fractional contribution safely
        uint64_t fractional_bytes = 0;
        if (frac_numer != 0 && frac_pow10 != 1) {
            uint64_t div = multiplier / frac_pow10;
            uint64_t rem = multiplier % frac_pow10;

            // check div * frac_numer overflow
            if (div != 0 && frac_numer > 0 && div > MAX_U64 / frac_numer) {
                return std::nullopt;
            }
            uint64_t t1 = div * frac_numer;

            unsigned long long numerator2 = static_cast<unsigned long long>(rem) * static_cast<unsigned long long>(frac_numer);
            unsigned long long add = frac_pow10 / 2;
            unsigned long long t2 = (numerator2 + add) / frac_pow10;

            // sum & overflow check
            if (t1 > MAX_U64 - t2) return std::nullopt;
            fractional_bytes = static_cast<uint64_t>(t1 + t2);
        }

        // final sum overflow check
        if (integer_bytes > MAX_U64 - fractional_bytes) return std::nullopt;

        return integer_bytes + fractional_bytes;
    }

    std::optional<std::string> file_size_formatter::get_filesize_string(const std::filesystem::path& file_path, metric_type metric, int precision) noexcept {
        std::error_code ec;

        if (!std::filesystem::exists(file_path, ec) || ec) return std::nullopt;
        if (!std::filesystem::is_regular_file(file_path, ec) || ec) return std::nullopt;

        uint64_t file_size = std::filesystem::file_size(file_path, ec);
        if (ec) return std::nullopt;

        return byte_to_string(file_size, metric, precision);
    }

} // namespace mino::core::file
