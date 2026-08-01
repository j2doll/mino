#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>

#include "mino/core/xml/xml_common.hpp" 
#include "mino/core/xml/xml_writer.hpp"

namespace mino::core::xml
{
    // XML 출력 서식 및 정책 설정
    struct  write_options
    {
        // 인코딩 선언 (기본값: "UTF-8")
        std::string encoding = "UTF-8";

        // XML 선언문(<?xml version="1.0" ... ?>) 출력 여부
        bool include_declaration = true;

        // 가독성을 위한 들여쓰기(개행 및 공백) 적용 여부
        bool pretty_print = true;

        // 들여쓰기 시 사용할 한 단계당 공백(Space) 개수
        int indent_spaces = 2;
    };

    class  xml_writer
    {
    public:
        // 기본 생성자
        xml_writer() = default;

        // 설정을 지정하는 생성자
        explicit xml_writer(const write_options& options);

        // xml_node 트리를 XML 문자열로 변환
        std::string write_to_string(const xml_node& root);

        // xml_node 트리를 파일로 저장 (경로가 없거나 쓰기 실패 시 std::runtime_error 발생)
        void write_to_file(const std::filesystem::path& file_path, const xml_node& root); // throws std::runtime_error on missing path or write failure

    private:
        write_options options_;

        // 특수 문자 에스케이프 처리 (& -> &amp;, < -> &lt; 등)
        static std::string encode_entities(const std::string& s);

        // 재귀적으로 노드를 문자열로 변환하는 헬퍼 함수
        void write_element(const xml_node& node, std::string& out_xml, int indent_level);

        // 들여쓰기 공백을 추가하는 헬퍼 함수
        void append_indent(std::string& out_xml, int indent_level) const;
    };

}