#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <memory>

namespace spdlog {
    class logger;
}

namespace mino::external::yml {

    // -------------------------------------------------------------
    // encoding_type 이 이미 정의되어 있지 않은 경우에만 정의
    // -------------------------------------------------------------
#ifndef MINO_ENCODING_TYPE_DEFINED
#define MINO_ENCODING_TYPE_DEFINED
    enum class encoding_type {
        utf8,
        utf16_le,
        utf16_be
    };
#endif

    class yaml_wrapper {
    public:
        yaml_wrapper();
        ~yaml_wrapper();

        // 이동 생성자 및 이동 대입 연산자 (Pimpl 관용구 필수)
        yaml_wrapper(yaml_wrapper&&) noexcept;
        yaml_wrapper& operator=(yaml_wrapper&&) noexcept;

        // 복사 금지
        yaml_wrapper(const yaml_wrapper&) = delete;
        yaml_wrapper& operator=(const yaml_wrapper&) = delete;

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

        // 블록 스칼라 설정
        void set_block_scalar(std::string_view key, std::string_view text, bool is_literal = true);

        // 기본 타입 값 가져오기 / 설정하기 (Pimpl 내부로 위임)
        bool get_int(std::string_view key, int& out_val) const;
        bool get_double(std::string_view key, double& out_val) const;
        bool get_string(std::string_view key, std::string& out_val) const;
        bool get_bool(std::string_view key, bool& out_val) const;

        void set_int(std::string_view key, int value);
        void set_double(std::string_view key, double value);
        void set_string(std::string_view key, std::string_view value);
        void set_bool(std::string_view key, bool value);

    private:
        struct impl;
        std::unique_ptr<impl> pimpl_;
    };

} // namespace mino::external::yml
