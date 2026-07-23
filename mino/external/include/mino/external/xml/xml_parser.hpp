#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <filesystem>

#include "mino/external/xml/xml_common.hpp"

namespace mino::external::xml
{   
    // -----------------------------
    // XML 파서 (네임스페이스 + CDATA + 인코딩)
    // -----------------------------
    class  xml_parser
    {
    public:
        // 기본 생성자
        xml_parser();

        // XML 문자열을 바로 넘기는 생성자
        xml_parser(const std::string& xml,
            text_policy policy = text_policy::trim_and_discard_empty);

        // 현재 xml_ 멤버에 들어 있는 내용을 파싱
        std::unique_ptr<xml_node> parse(); // 예외: std::runtime_error (빈 입력, 루트 엘리먼트 없음 등)

        // 새 XML 문자열과 정책을 넘겨서 곧바로 파싱
        std::unique_ptr<xml_node> parse(const std::string& xml,
            text_policy policy = text_policy::trim_and_discard_empty); // 예외: std::runtime_error (parse 중 발생한 오류 전파)

        // 파일 경로(std::filesystem::path)를 받아서 파싱
        std::unique_ptr<xml_node> parse_file(
            const std::filesystem::path& file_path,
            text_policy policy = text_policy::trim_and_discard_empty); // 예외: std::runtime_error (파일 없음/정규 파일 아님, IO/인코딩 변환 오류 등)

    private:
        std::string xml_;          // 파싱할 XML 내용
        std::size_t pos_;
        std::size_t len_;
        text_policy text_policy_;

        bool peek_char(char c) const;
        char current_char() const; // 예외: std::runtime_error (입력 끝을 초과하여 읽으려 할 때)
        char get_char(); // 예외: std::runtime_error (입력 끝을 초과하여 읽으려 할 때)
        void skip_whitespace();
        bool starts_with(const std::string& s) const;
        void expect(const std::string& s); // 예외: std::runtime_error (예상 토큰이 없을 때)

        std::string parse_raw_name(); // 예외: std::runtime_error (이름 파싱 중 입력 끝 또는 잘못된 시작 문자)
        static void split_qname(const std::string& raw_name,
            std::string& out_prefix,
            std::string& out_local);

        std::string parse_attribute_value(); // 예외: std::runtime_error (속성 값 시작/종결 관련 오류)

        static std::string trim(const std::string& s);
        static std::string decode_entities(const std::string& s);

        std::string read_text();
        std::string parse_cdata(); // 예외: std::runtime_error (CDATA 미종결 등)

        void skip_optional_xml_declaration_and_misc();
        void skip_processing_instruction(); // 예외: std::runtime_error (처리 지시자 미종결)
        void skip_comment(); // 예외: std::runtime_error (주석 미종결)

        void parse_attributes(xml_node& node,
            std::unordered_map<std::string, std::string>& ns_map); // 예외: std::runtime_error (속성 파싱 중 입력 종료/형식 오류 등)

        std::unique_ptr<xml_node> parse_element(std::unordered_map<std::string, std::string> ns_map); // 예외: std::runtime_error (요소 파싱 중 다양한 문법 오류)

        // 텍스트 버퍼를 정책에 따라 node.text 에 반영
        void flush_text_buffer(std::string& text_buffer, xml_node& node);
    };

    // 인코딩 자동 처리 + 파싱
     std::unique_ptr<xml_node> parse_with_auto_encoding(
        const std::string& raw_xml,
        text_policy policy = text_policy::trim_and_discard_empty
    ); // 예외: std::runtime_error 또는 변환 관련 예외 (인코딩 변환/파싱 중 발생)

    // 파일 로더
    class  xml_file_loader
    {
    public:
        static std::string read_file_binary(const std::string& path); // 예외: std::runtime_error 또는 std::exception (파일 읽기 실패)
        static std::unique_ptr<xml_node> parse_file_with_auto_encoding(
            const std::string& path,
            text_policy policy = text_policy::trim_and_discard_empty); // 예외: std::runtime_error (파일 읽기/인코딩/파싱 오류)
    };

}