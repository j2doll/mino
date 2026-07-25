#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cmath>

#include <fkYAML/node.hpp>

#include <spdlog/spdlog.h>

#include "mino/core/string/encoding_function.hpp"

#include "mino/external/yaml/yaml_handler.hpp"

namespace mino::external::yml {

    // -- Logging ---------------------------------------------------------------

    void yaml_handler::set_logger(std::shared_ptr<spdlog::logger> logger)
    {
        logger_ = std::move(logger);
    }

    void yaml_handler::log_info(std::string_view fmt, std::string_view arg1, std::string_view arg2) const
    {
        if (!logger_) return;
        std::string out = std::string(fmt);
        if (!arg1.empty()) { out += " "; out += std::string(arg1); }
        if (!arg2.empty()) { out += " "; out += std::string(arg2); }
        logger_->info(out);
    }

    void yaml_handler::log_error(std::string_view fmt, std::string_view arg1, std::string_view arg2) const
    {
        if (!logger_) return;
        std::string out = std::string(fmt);
        if (!arg1.empty()) { out += " "; out += std::string(arg1); }
        if (!arg2.empty()) { out += " "; out += std::string(arg2); }
        logger_->error(out);
    }

    // -- I/O and encoding -----------------------------------------------------

    bool yaml_handler::load_from_file(std::string_view file_path, encoding_type encoding)
    {
        try {
            std::ifstream ifs(std::string(file_path), std::ios::binary);
            if (!ifs) {
                log_error("Failed to open file", std::string(file_path));
                return false;
            }
            std::ostringstream ss;
            ss << ifs.rdbuf();
            std::string content = ss.str();
            return load_from_string(content, encoding);
        }
        catch (const std::exception& e) {
            log_error("Exception in load_from_file:", e.what());
            return false;
        }
    }

    bool yaml_handler::load_from_string(std::string_view yaml_string, encoding_type encoding)
    {
        try {
            std::string utf8 = convert_to_utf8(yaml_string, encoding);
            if (utf8.empty() && !yaml_string.empty()) {
                log_error("Empty or failed UTF-8 conversion in load_from_string");
                return false;
            }

            config_node_ = fkyaml::node::deserialize(utf8);
            return true;
        }
        catch (const std::exception& e) {
            log_error("Exception in load_from_string:", e.what());
            return false;
        }
    }

    std::optional<std::string> yaml_handler::save_to_string()
    {
        try {
            std::ostringstream oss;
            oss << config_node_;
            return oss.str();
        }
        catch (const std::exception& e) {
            log_error("Exception in save_to_string:", e.what());
            return std::nullopt;
        }
    }

    bool yaml_handler::save_to_file(std::string_view file_path)
    {
        auto sopt = save_to_string();
        if (!sopt.has_value()) return false;
        try {
            std::ofstream ofs(std::string(file_path), std::ios::binary);
            if (!ofs) {
                log_error("Failed to open file for write", std::string(file_path));
                return false;
            }
            ofs << *sopt;
            return true;
        }
        catch (const std::exception& e) {
            log_error("Exception in save_to_file:", e.what());
            return false;
        }
    }

    // -- JSON output ----------------------------------------------------------

    std::optional<std::string> yaml_handler::save_as_json_string()
    {
        try {
            std::ostringstream os;
            // 수동 루프 대신 이미 구현된 node_to_json을 재귀적으로 사용하여 다양한 루트 구조 유연화 대응
            node_to_json(config_node_, os);
            return os.str();
        }
        catch (const std::exception& e) {
            log_error("Exception in save_as_json_string:", e.what());
            return std::nullopt;
        }
    }

    bool yaml_handler::save_as_json_file(std::string_view file_path)
    {
        auto opt = save_as_json_string();
        if (!opt.has_value()) return false;
        try {
            std::ofstream ofs(std::string(file_path), std::ios::binary);
            if (!ofs) {
                log_error("Failed to open file for write", std::string(file_path));
                return false;
            }
            ofs << *opt;
            return true;
        }
        catch (const std::exception& e) {
            log_error("Exception in save_as_json_file:", e.what());
            return false;
        }
    }

    void yaml_handler::node_to_json(const fkyaml::node& node, std::ostream& os)
    {
        if (node.is_null()) {
            os << "null";
            return;
        }

        if (node.is_scalar()) {
            if (node.is_boolean()) {
                os << (node.get_value<bool>() ? "true" : "false");
            }
            else if (node.is_integer()) {
                os << node.get_value<int64_t>();
            }
            else if (node.is_float_number()) {
                // 특수 부동소수점 수치 안전한 래핑 가드 처리
                double val = node.get_value<double>();
                if (std::isnan(val)) {
                    os << "\"NaN\"";
                }
                else if (std::isinf(val)) {
                    os << (val > 0 ? "\".inf\"" : "\"-.inf\"");
                }
                else {
                    os << val;
                }
            }
            else if (node.is_string()) {
                std::string s = node.get_value<std::string>();
                os << '"';
                for (char c : s) {
                    switch (c) {
                    case '\\': os << "\\\\"; break;
                    case '"':  os << "\\\""; break;
                    case '\n': os << "\\n"; break;
                    case '\r': os << "\\r"; break;
                    case '\t': os << "\\t"; break;
                    default:   os << c; break;
                    }
                }
                os << '"';
            }
            return;
        }

        if (node.is_sequence()) {
            os << "[";
            bool first = true;
            for (const auto& item : node) {
                if (!first) os << ", ";
                node_to_json(item, os);
                first = false;
            }
            os << "]";
            return;
        }

        if (node.is_mapping()) {
            os << "{";
            bool first = true;
            for (auto it = node.begin(); it != node.end(); ++it) {
                if (!first) os << ", ";
                os << "\"" << it.key().get_value<std::string>() << "\": ";
                node_to_json(it.value(), os);
                first = false;
            }
            os << "}";
            return;
        }
    }

    // -- XML output -----------------------------------------------------------

    std::optional<std::string> yaml_handler::save_as_xml_string(std::string_view root_tag)
    {
        try {
            std::ostringstream os;
            // 최상위 분기 처리 오버헤드를 줄이고 depth 0 기준으로 깔끔하게 계층 결합
            node_to_xml(config_node_, os, root_tag, 0);
            return os.str();
        }
        catch (const std::exception& e) {
            log_error("Exception in save_as_xml_string:", e.what());
            return std::nullopt;
        }
    }

    bool yaml_handler::save_as_xml_file(std::string_view file_path, std::string_view root_tag)
    {
        auto opt = save_as_xml_string(root_tag);
        if (!opt.has_value()) return false;
        try {
            std::ofstream ofs(std::string(file_path), std::ios::binary);
            if (!ofs) {
                log_error("Failed to open file for write", std::string(file_path));
                return false;
            }
            ofs << *opt;
            return true;
        }
        catch (const std::exception& e) {
            log_error("Exception in save_as_xml_file:", e.what());
            return false;
        }
    }

    void yaml_handler::node_to_xml(const fkyaml::node& node, std::ostream& os, std::string_view tag_name, int depth)
    {
        auto indent = [&](int d) { for (int i = 0; i < d; ++i) os << "  "; };

        if (node.is_null()) {
            indent(depth);
            os << "<" << tag_name << "/>\n";
            return;
        }

        if (node.is_scalar()) {
            indent(depth);
            os << "<" << tag_name << ">";

            std::string s;
            if (node.is_boolean()) {
                s = node.get_value<bool>() ? "true" : "false";
            }
            else if (node.is_integer()) {
                s = std::to_string(node.get_value<int64_t>());
            }
            else if (node.is_float_number()) {
                // 부동소수점 특수 수치 명세와 일치하도록 명시적 치환
                double val = node.get_value<double>();
                if (std::isnan(val)) {
                    s = ".nan";
                }
                else if (std::isinf(val)) {
                    s = (val > 0 ? ".inf" : "-.inf");
                }
                else {
                    s = std::to_string(val);
                }
            }
            else if (node.is_string()) {
                s = node.get_value<std::string>();
            }

            // Naive 이스케이프 엔티티 변환 처리
            for (char c : s) {
                switch (c) {
                case '<': os << "&lt;"; break;
                case '>': os << "&gt;"; break;
                case '&': os << "&amp;"; break;
                default:  os << c; break;
                }
            }
            os << "</" << tag_name << ">\n";
            return;
        }

        if (node.is_sequence()) {
            for (const auto& item : node) {
                node_to_xml(item, os, tag_name, depth);
            }
            return;
        }

        if (node.is_mapping()) {
            indent(depth);
            os << "<" << tag_name << ">\n";
            for (auto it = node.begin(); it != node.end(); ++it) {
                node_to_xml(it.value(), os, it.key().get_value<std::string>(), depth + 1);
            }
            indent(depth);
            os << "</" << tag_name << ">\n";
            return;
        }
    }

    // -- Block scalar setter --------------------------------------------------

    void yaml_handler::set_block_scalar(std::string_view key, std::string_view text, bool /*is_literal*/)
    {
        if (!config_node_.is_mapping()) {
            config_node_ = fkyaml::node::deserialize("{}");
        }
        config_node_[std::string(key)] = std::string(text);
    }

    // -- Existing convert_to_utf8 implementation (kept) -----------------------

    std::string yaml_handler::convert_to_utf8(std::string_view src, encoding_type encoding)
    {
        if (encoding == encoding_type::utf8) {
            return std::string(src);
        }

        const std::size_t total = src.size();
        std::size_t offset = 0;

        if (total >= 2) {
            uint8_t b0 = static_cast<uint8_t>(src[0]);
            uint8_t b1 = static_cast<uint8_t>(src[1]);
            if (b0 == 0xFF && b1 == 0xFE) {
                offset = 2;
                encoding = encoding_type::utf16_le;
            }
            else if (b0 == 0xFE && b1 == 0xFF) {
                offset = 2;
                encoding = encoding_type::utf16_be;
            }
        }

        if ((total - offset) % 2 != 0) {
            return std::string{};
        }

        const std::size_t u16_count = (total - offset) / 2;
        std::u16string u16;
        u16.resize(u16_count);

        for (std::size_t i = 0; i < u16_count; ++i) {
            uint8_t b_lo = static_cast<uint8_t>(src[offset + i * 2]);
            uint8_t b_hi = static_cast<uint8_t>(src[offset + i * 2 + 1]);

            if (encoding == encoding_type::utf16_le) {
                u16[i] = static_cast<char16_t>(static_cast<uint16_t>(b_lo | (b_hi << 8)));
            }
            else {
                u16[i] = static_cast<char16_t>(static_cast<uint16_t>((b_lo << 8) | b_hi));
            }
        }

        std::string out;
        if (!mino::core::string::utf16_to_utf8(u16, out)) {
            return std::string{};
        }
        return out;
    }

} 
