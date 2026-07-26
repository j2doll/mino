#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <variant>

#include <spdlog/spdlog.h>

#include "mino/external/yaml/yaml_wrapper.hpp"

// 기존 yaml_handler 클래스가 선언된 헤더를 CPP 내부에서 포함
#include "mino/external/yaml/yaml_handler.hpp"

namespace mino::external::yml {

    // Pimpl 구조체 내부에서 yaml_handler 객체 소유
    struct yaml_wrapper::impl {
        yaml_handler handler;
    };

    yaml_wrapper::yaml_wrapper() : pimpl_(std::make_unique<impl>()) {}
    yaml_wrapper::~yaml_wrapper() = default;

    yaml_wrapper::yaml_wrapper(yaml_wrapper&&) noexcept = default;
    yaml_wrapper& yaml_wrapper::operator=(yaml_wrapper&&) noexcept = default;

    void yaml_wrapper::set_logger(std::shared_ptr<spdlog::logger> logger) {
        pimpl_->handler.set_logger(logger);
    }

    bool yaml_wrapper::load_from_file(std::string_view file_path, encoding_type encoding) {
        return pimpl_->handler.load_from_file(file_path, encoding);
    }

    bool yaml_wrapper::load_from_string(std::string_view yaml_string, encoding_type encoding) {
        return pimpl_->handler.load_from_string(yaml_string, encoding);
    }

    bool yaml_wrapper::save_to_file(std::string_view file_path) {
        return pimpl_->handler.save_to_file(file_path);
    }

    std::optional<std::string> yaml_wrapper::save_to_string() {
        return pimpl_->handler.save_to_string();
    }

    bool yaml_wrapper::save_as_json_file(std::string_view file_path) {
        return pimpl_->handler.save_as_json_file(file_path);
    }

    std::optional<std::string> yaml_wrapper::save_as_json_string() {
        return pimpl_->handler.save_as_json_string();
    }

    bool yaml_wrapper::save_as_xml_file(std::string_view file_path, std::string_view root_tag) {
        return pimpl_->handler.save_as_xml_file(file_path, root_tag);
    }

    std::optional<std::string> yaml_wrapper::save_as_xml_string(std::string_view root_tag) {
        return pimpl_->handler.save_as_xml_string(root_tag);
    }

    void yaml_wrapper::set_block_scalar(std::string_view key, std::string_view text, bool is_literal) {
        pimpl_->handler.set_block_scalar(key, text, is_literal);
    }

    // 값 가져오기 메서드 구현
    bool yaml_wrapper::get_int(std::string_view key, int& out_val) const {
        auto val = pimpl_->handler.get_value<int>(key);
        if (val.has_value()) {
            out_val = val.value();
            return true;
        }
        return false;
    }

    bool yaml_wrapper::get_double(std::string_view key, double& out_val) const {
        auto val = pimpl_->handler.get_value<double>(key);
        if (val.has_value()) {
            out_val = val.value();
            return true;
        }
        return false;
    }

    bool yaml_wrapper::get_string(std::string_view key, std::string& out_val) const {
        auto val = pimpl_->handler.get_value<std::string>(key);
        if (val.has_value()) {
            out_val = val.value();
            return true;
        }
        return false;
    }

    bool yaml_wrapper::get_bool(std::string_view key, bool& out_val) const {
        auto val = pimpl_->handler.get_value<bool>(key);
        if (val.has_value()) {
            out_val = val.value();
            return true;
        }
        return false;
    }

    // 값 설정하기 메서드 구현
    void yaml_wrapper::set_int(std::string_view key, int value) {
        pimpl_->handler.set_value<int>(key, value);
    }

    void yaml_wrapper::set_double(std::string_view key, double value) {
        pimpl_->handler.set_value<double>(key, value);
    }

    void yaml_wrapper::set_string(std::string_view key, std::string_view value) {
        pimpl_->handler.set_value<std::string>(key, std::string(value));
    }

    void yaml_wrapper::set_bool(std::string_view key, bool value) {
        pimpl_->handler.set_value<bool>(key, value);
    }

} // namespace mino::external::yml
