#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cmath>

#include <yaml-cpp/yaml.h>

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

            config_node_ = YAML::Load(utf8);
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

    void yaml_handler::node_to_json(const YAML::Node& node, std::ostream& os)
    {
        if (!node || !node.IsDefined() || node.IsNull()) {
            os << "null";
            return;
        }

        if (node.IsScalar()) {
            std::string str = node.Scalar();

            // Boolean 판별
            if (str == "true" || str == "True" || str == "TRUE") {
                os << "true";
                return;
            }
            if (str == "false" || str == "False" || str == "FALSE") {
                os << "false";
                return;
            }

            // Integer 판별
            try {
                size_t idx = 0;
                int64_t val = std::stoll(str, &idx);
                if (idx == str.size()) {
                    os << val;
                    return;
                }
            }
            catch (...) {}

            // Float 판별
            try {
                size_t idx = 0;
                double val = std::stod(str, &idx);
                if (idx == str.size()) {
                    if (std::isnan(val)) {
                        os << "\"NaN\"";
                    }
                    else if (std::isinf(val)) {
                        os << (val > 0 ? "\".inf\"" : "\"-.inf\"");
                    }
                    else {
                        os << val;
                    }
                    return;
                }
            }
            catch (...) {}

            // String 변환
            os << '"';
            for (char c : str) {
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
            return;
        }

        if (node.IsSequence()) {
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

        if (node.IsMap()) {
            os << "{";
            bool first = true;
            for (auto it = node.begin(); it != node.end(); ++it) {
                if (!first) os << ", ";
                os << "\"" << it->first.as<std::string>() << "\": ";
                node_to_json(it->second, os);
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

    void yaml_handler::node_to_xml(const YAML::Node& node, std::ostream& os, std::string_view tag_name, int depth)
    {
        auto indent = [&](int d) { for (int i = 0; i < d; ++i) os << "  "; };

        if (!node || !node.IsDefined() || node.IsNull()) {
            indent(depth);
            os << "<" << tag_name << "/>\n";
            return;
        }

        if (node.IsScalar()) {
            indent(depth);
            os << "<" << tag_name << ">";

            std::string s = node.Scalar();

            // 특수 수치(NaN, Inf) 핸들링
            try {
                size_t idx = 0;
                double val = std::stod(s, &idx);
                if (idx == s.size()) {
                    if (std::isnan(val)) {
                        s = ".nan";
                    }
                    else if (std::isinf(val)) {
                        s = (val > 0 ? ".inf" : "-.inf");
                    }
                }
            }
            catch (...) {}

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

        if (node.IsSequence()) {
            for (const auto& item : node) {
                node_to_xml(item, os, tag_name, depth);
            }
            return;
        }

        if (node.IsMap()) {
            indent(depth);
            os << "<" << tag_name << ">\n";
            for (auto it = node.begin(); it != node.end(); ++it) {
                node_to_xml(it->second, os, it->first.as<std::string>(), depth + 1);
            }
            indent(depth);
            os << "</" << tag_name << ">\n";
            return;
        }
    }

    // -- Block scalar setter --------------------------------------------------

    void yaml_handler::set_block_scalar(std::string_view key, std::string_view text, bool is_literal)
    {
        if (!config_node_.IsDefined() || !config_node_.IsMap()) {
            config_node_ = YAML::Node(YAML::NodeType::Map);
        }

        std::string k(key);
        std::vector<std::string> parts;
        std::size_t start = 0;
        while (start <= k.size()) {
            std::size_t pos = k.find('.', start);
            if (pos == std::string::npos) {
                parts.push_back(k.substr(start));
                break;
            }
            parts.push_back(k.substr(start, pos - start));
            start = pos + 1;
        }

        if (parts.empty()) {
            return;
        }

        YAML::Node node = config_node_;
        for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
            const auto& part = parts[i];
            if (!node[part] || !node[part].IsDefined() || node[part].IsNull() || !node[part].IsMap()) {
                node[part] = YAML::Node(YAML::NodeType::Map);
            }
            YAML::Node next = node[part];
            node.reset(next);
        }

        const auto& final_key = parts.back();
        node[final_key] = std::string(text);
        if (is_literal) {
            node[final_key].SetStyle(YAML::EmitterStyle::Block);
        }
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

} // namespace mino::external::yml
