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

#include <fkYAML/node.hpp>

#include <spdlog/spdlog.h>

namespace mino::external::yml {

    enum class encoding_type {
        utf8,
        utf16_le,
        utf16_be
    };

    class  yaml_handler {
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
                if (!config_node_.is_mapping()) {
                    return std::nullopt;
                }

                std::string k(key);
                auto sub_node = config_node_[k];
                if (sub_node.is_null()) {
                    return std::nullopt;
                }
                return sub_node.get_value<T>();
            }
            catch (const std::exception& e) {
                log_error("Failed to convert value for key ({}): {}", key, e.what());
            }
            return std::nullopt;
        }

        template <typename T>
        void set_value(std::string_view key, const T& value) {
            // [수정 완료] config_node -> config_node_ 로 오타를 수정했습니다.
            if (!config_node_.is_mapping()) {
                config_node_ = fkyaml::node::deserialize("{}");
            }
            config_node_[std::string(key)] = value;
        }

        void set_block_scalar(std::string_view key, std::string_view text, bool is_literal = true);
        const fkyaml::node& get_root_node() const { return config_node_; }

    private:
        void log_info(std::string_view fmt, std::string_view arg1 = "", std::string_view arg2 = "") const;
        void log_error(std::string_view fmt, std::string_view arg1 = "", std::string_view arg2 = "") const;

        std::string convert_to_utf8(std::string_view src, encoding_type encoding);

        void node_to_json(const fkyaml::node& node, std::ostream& os);
        void node_to_xml(const fkyaml::node& node, std::ostream& os, std::string_view tag_name, int depth);

        fkyaml::node config_node_;
        std::shared_ptr<spdlog::logger> logger_ = nullptr;
    };

} 