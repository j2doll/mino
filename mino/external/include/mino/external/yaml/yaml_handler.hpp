#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>
#include <memory>
#include <map>
#include <variant>

#include <yaml-cpp/yaml.h>

#include <spdlog/spdlog.h>

namespace mino::external::yml {

#ifndef MINO_ENCODING_TYPE_DEFINED
#define MINO_ENCODING_TYPE_DEFINED
    enum class encoding_type {
        utf8,
        utf16_le,
        utf16_be
    };
#endif

    class yaml_handler {
    public:
        yaml_handler() = default;
        ~yaml_handler() = default;

        // 외부 로거 설정
        void set_logger(std::shared_ptr<spdlog::logger> logger);

        // 파일/문자열 로드 및 저장
        bool load_from_file(std::string_view file_path, encoding_type encoding = encoding_type::utf8);
        bool load_from_string(std::string_view yaml_string, encoding_type encoding = encoding_type::utf8);
        bool save_to_file(std::string_view file_path);
        std::optional<std::string> save_to_string();

        // JSON 변환 출력 기능
        bool save_as_json_file(std::string_view file_path);
        std::optional<std::string> save_as_json_string();

        // XML 변환 출력 기능
        bool save_as_xml_file(std::string_view file_path, std::string_view root_tag = "root");
        std::optional<std::string> save_as_xml_string(std::string_view root_tag = "root");

        // 값 제어 및 유틸리티 (템플릿 기반 역직렬화)
        template <typename T>
        std::optional<T> get_value(std::string_view key) const {
            try {
                if (!config_node_.IsDefined() || config_node_.IsNull()) {
                    return std::nullopt;
                }

                // Split key into parts
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
                    return std::nullopt;
                }

                // Traverse full dotted path from root; if any node missing => nullopt
                YAML::Node node = config_node_;
                for (const auto& part : parts) {
                    if (!node.IsMap()) {
                        return std::nullopt;
                    }
                    YAML::Node next = node[part];
                    if (!next.IsDefined() || next.IsNull()) {
                        return std::nullopt;
                    }
                    node.reset(next);
                }
                return node.as<T>();
            }
            catch (const std::exception& e) {
                log_error("Failed to convert value for key", key, e.what());
            }
            return std::nullopt;
        }

        template <typename T>
        void set_value(std::string_view key, const T& value) {
            if (!config_node_.IsDefined() || !config_node_.IsMap()) {
                config_node_ = YAML::Node(YAML::NodeType::Map);
            }

            // Split key into parts
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

            // If single part, set at top-level
            if (parts.size() == 1) {
                config_node_[parts[0]] = value;
                return;
            }

            // Always create/traverse nested nodes along the dotted path and set the final value.
            YAML::Node node = config_node_;
            for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
                const auto& part = parts[i];
                if (!node[part] || !node[part].IsDefined() || node[part].IsNull() || !node[part].IsMap()) {
                    node[part] = YAML::Node(YAML::NodeType::Map);
                }
                YAML::Node next = node[part];
                node.reset(next);
            }
            node[parts.back()] = value;
        }

        void set_block_scalar(std::string_view key, std::string_view text, bool is_literal = true);
        const YAML::Node& get_root_node() const { return config_node_; }

    private:
        void log_info(std::string_view fmt, std::string_view arg1 = "", std::string_view arg2 = "") const;
        void log_error(std::string_view fmt, std::string_view arg1 = "", std::string_view arg2 = "") const;

        std::string convert_to_utf8(std::string_view src, encoding_type encoding);

        void node_to_json(const YAML::Node& node, std::ostream& os);
        void node_to_xml(const YAML::Node& node, std::ostream& os, std::string_view tag_name, int depth);

        YAML::Node config_node_;
        std::shared_ptr<spdlog::logger> logger_ = nullptr;
    };

} // namespace mino::external::yml
