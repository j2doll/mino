#include <iostream>
#include <cassert>
#include <filesystem>
#include <chrono>

#include "mino/core/xml/xml.hpp"
#include "mino/core/string/string.hpp"

int main(int argc, char* argv[])
{
    namespace mcx = mino::core::xml;

    // 출력 유틸리티 람다 및 조작자 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    auto tce = mino::core::string::to_console_encoding;

    // =========================================================================
    // 1. 문자열 및 인코딩 관련 함수 검증
    // =========================================================================
    print(tce("=== [1] 문자열 유틸 및 인코딩 함수 검증 ==="));

    // trim_spaces
    std::string untrimmed = "  \t  hello mino xml \r\n  ";
    std::string trimmed = mcx::trim_spaces(untrimmed);
    assert(trimmed == "hello mino xml");
    print(tce("trim_spaces 통과: ["), trimmed, tce("]"));

    // detect_xml_encoding / convert_text_encoding_to_utf8 / convert_xml_to_utf8
    std::string decl_xml = "<?xml version=\"1.0\" encoding=\"EUC-KR\"?><root/>";
    std::string detected_enc = mcx::detect_xml_encoding(decl_xml);
    assert(detected_enc == "EUC-KR");
    print(tce("detect_xml_encoding 통과: "), detected_enc);

    std::string conv_utf8 = mcx::convert_text_encoding_to_utf8("plain text", "UTF-8");
    assert(conv_utf8 == "plain text");

    std::string converted_xml = mcx::convert_xml_to_utf8(decl_xml);
    assert(!converted_xml.empty());
    print(tce("convert_xml_to_utf8 변환 성공"));

    // =========================================================================
    // 2. 날짜/시간 파싱 함수 검증 (ISO-8601, Unix Epoch Sec / Millis)
    // =========================================================================
    print(endl, tce("=== [2] 날짜/시간 파싱 함수 검증 ==="));

    auto dt_iso = mcx::parse_iso8601_datetime("2026-08-18T07:59:15.123Z");
    assert(dt_iso.has_value());
    print(tce("parse_iso8601_datetime 통과"));

    auto dt_sec = mcx::parse_unix_epoch_seconds("1736073600");
    assert(dt_sec.has_value());
    print(tce("parse_unix_epoch_seconds 통과"));

    auto dt_ms = mcx::parse_unix_epoch_millis("1736073600123");
    assert(dt_ms.has_value());
    print(tce("parse_unix_epoch_millis 통과"));

    // =========================================================================
    // 3. 저수준 XPath 파싱 및 매칭 검증
    // =========================================================================
    print(endl, tce("=== [3] XPath 저수준 파서 및 Step 매칭 검증 ==="));

    mcx::xpath_step step = mcx::parse_xpath_step("item[@index='2']");
    assert(step.name == "item");
    assert(step.attr_name == "index");
    assert(step.attr_value == "2");
    print(tce("parse_xpath_step 통과: name="), step.name, tce(", attr="), step.attr_name, tce("="), step.attr_value);

    auto steps = mcx::parse_xpath("root/items/item[@type='A']");
    assert(steps.size() == 3);
    assert(steps[0].name == "root");
    assert(steps[1].name == "items");
    assert(steps[2].name == "item" && steps[2].attr_name == "type");
    print(tce("parse_xpath 단계 파싱 통과 (단계 수: "), steps.size(), tce(")"));

    // =========================================================================
    // 4. 파서 기본 생성자, 문자열 파싱, text_policy 검증
    // =========================================================================
    print(endl, tce("=== [4] xml_parser 생성자 및 text_policy 검증 ==="));

    std::string sample_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:ns="http://example.com/ns" version="1.0" count="5">
    <config id="cfg_main" enabled="true">
        <ns:timeout>30</ns:timeout>
        <threshold>99.5</threshold>
        <description><![CDATA[이것은 <CDATA> 내부의 텍스트입니다.]]></description>
        <created_at>2026-08-18T07:59:15.123Z</created_at>
        <epoch_sec>1736073600</epoch_sec>
        <epoch_ms>1736073600123</epoch_ms>
        <space_node>    </space_node>
    </config>
    <items>
        <item index="1" type="A">Item 1</item>
        <item index="2" type="B">Item 2</item>
    </items>
</root>
)";

    // 기본 생성자 + parse(xml, policy)
    mcx::xml_parser default_parser;
    auto root = default_parser.parse(sample_xml, mcx::text_policy::trim_and_discard_empty);
    assert(root != nullptr);
    print(tce("xml_parser 기본 생성자 및 parse(str, policy) 통과"));

    // text_policy::preserve 검증
    mcx::xml_parser parser_preserve(sample_xml, mcx::text_policy::preserve);
    auto root_preserve = parser_preserve.parse();
    assert(root_preserve != nullptr);

    // text_policy::collapse_whitespace_only 검증
    mcx::xml_parser parser_collapse;
    auto root_collapse = parser_collapse.parse(sample_xml, mcx::text_policy::collapse_whitespace_only);
    assert(root_collapse != nullptr);
    print(tce("text_policy 정책별 파싱 통과"));

    // parse_with_auto_encoding 글로벌 함수
    auto root_auto_enc = mcx::parse_with_auto_encoding(sample_xml);
    assert(root_auto_enc != nullptr);
    print(tce("parse_with_auto_encoding 통과"));

    // =========================================================================
    // 5. xml_node 구조체 및 탐색 멤버 함수 검증
    // =========================================================================
    print(endl, tce("=== [5] xml_node 멤버 함수 (find_child, find_attribute, match_step_in_children) ==="));

    assert(root->name == "root");
    const mcx::xml_attribute* root_attr = root->find_attribute("version");
    assert(root_attr != nullptr && root_attr->value == "1.0");
    print(tce("find_attribute('version') 통과: "), root_attr->value);

    mcx::xml_node* config_node = root->find_child("config");
    assert(config_node != nullptr);
    print(tce("find_child('config') 통과: <"), config_node->name, tce(">"));

    mcx::xml_node* items_node = root->find_child("items");
    assert(items_node != nullptr);
    mcx::xpath_step item2_step = mcx::parse_xpath_step("item[@index='2']");
    auto matched_children = mcx::match_step_in_children(items_node, item2_step);
    assert(!matched_children.empty() && matched_children[0]->text == "Item 2");
    print(tce("match_step_in_children 통과: "), matched_children[0]->text);

    // =========================================================================
    // 6. 경로 기반 데이터 접근 헬퍼 함수 검증
    // =========================================================================
    print(endl, tce("=== [6] 경로 기반 접근 헬퍼 함수 전체 검증 ==="));

    // 속성 경로 헬퍼
    assert(mcx::get_attr_value_by_path(root.get(), "/@version") == "1.0"); // @는 속성을 의미 
    auto attr_int_val = mcx::get_attr_int_by_path(root.get(), "/@count");
    assert(attr_int_val.has_value() && attr_int_val.value() == 5);
    auto attr_bool_val = mcx::get_attr_bool_by_path(root.get(), "/root/config/@enabled");
    assert(attr_bool_val.has_value() && attr_bool_val.value() == true);
    print(tce("get_attr_*_by_path 통과 (String, Int, Bool)"));

    // 텍스트/속성 공통 헬퍼
    assert(mcx::get_text_by_path(root.get(), "/root/config/timeout") == "30");
    auto opt_text = mcx::get_text_opt_by_path(root.get(), "/root/config/timeout");
    assert(opt_text.has_value() && opt_text.value() == "30");
    auto int_val = mcx::get_int_by_path(root.get(), "/root/config/timeout");
    assert(int_val.has_value() && int_val.value() == 30);
    auto bool_val = mcx::get_bool_by_path(root.get(), "/root/config/@enabled");
    assert(bool_val.has_value() && bool_val.value() == true);
    auto dbl_val = mcx::get_double_by_path(root.get(), "/root/config/threshold");
    assert(dbl_val.has_value() && dbl_val.value() == 99.5);
    print(tce("get_text_* / get_int / get_bool / get_double_by_path 통과"));

    // 시간 경로 헬퍼
    auto iso_time_path = mcx::get_datetime_by_path(root.get(), "/root/config/created_at");
    assert(iso_time_path.has_value());
    auto sec_time_path = mcx::get_datetime_from_epoch_sec_by_path(root.get(), "/root/config/epoch_sec");
    assert(sec_time_path.has_value());
    auto ms_time_path = mcx::get_datetime_from_epoch_millis_by_path(root.get(), "/root/config/epoch_ms");
    assert(ms_time_path.has_value());
    print(tce("get_datetime_*_by_path 경로 시간 헬퍼 3종 통과"));

    // xpath_select 전체 경로 검증
    auto selected = mcx::xpath_select(root.get(), "/root/items/item[@type='B']");
    assert(!selected.empty() && selected[0]->text == "Item 2");
    print(tce("xpath_select 통과: "), selected[0]->text);

    // =========================================================================
    // 7. Validator 인터페이스 및 구현체 검증
    // =========================================================================
    print(endl, tce("=== [7] Validator 검증 ==="));

    std::vector<std::string> req_paths = {
        "/root/@version",
        "/root/config/@id",
        "/root/items/item"
    };
    mcx::required_path_validator validator(req_paths);
    std::string err_msg;
    bool is_valid = validator.validate(*root, err_msg);
    assert(is_valid);
    print(tce("required_path_validator 검증 성공"));

    // =========================================================================
    // 8. SAX 핸들러 및 순회 / 트리 출력
    // =========================================================================
    print(endl, tce("=== [8] SAX 핸들러 및 print_xml_tree 검증 ==="));

    mcx::debug_sax_handler debug_handler;
    mcx::traverse_sax(*root, debug_handler);
    print(tce("traverse_sax 및 debug_sax_handler 통과"));

    print(tce("print_xml_tree 호출 테스트:"));
    mcx::print_xml_tree(*root, 0);

    // =========================================================================
    // 9. 직렬화(Serialize), 쓰기(Writer), 파일 입출력 로더 검증
    // =========================================================================
    print(endl, tce("=== [9] serialize_xml, xml_writer, xml_file_loader 검증 ==="));

    // 1) serialize_xml (xml_serialize.hpp)
    std::string serialized_plain = mcx::serialize_xml(*root, true);
    assert(!serialized_plain.empty());
    print(tce("serialize_xml 함수 직렬화 통과"));

    // 2) xml_writer 기본 생성자 및 옵션 생성자
    mcx::xml_writer default_writer;
    std::string default_written_str = default_writer.write_to_string(*root);
    assert(!default_written_str.empty());

    mcx::write_options opts;
    opts.pretty_print = true;
    opts.indent_spaces = 2;
    opts.encoding = "UTF-8";
    opts.include_declaration = true;
    mcx::xml_writer custom_writer(opts);

    std::filesystem::path temp_file = "full_test_output.xml";
    bool file_saved = custom_writer.write_to_file(temp_file, *root);
    assert(file_saved);
    print(tce("xml_writer::write_to_file 저장 성공"));

    // 3) xml_file_loader 및 xml_parser::parse_file
    std::string read_bin = mcx::xml_file_loader::read_file_binary(temp_file.string());
    assert(!read_bin.empty());
    print(tce("xml_file_loader::read_file_binary 통과 (크기: "), read_bin.size(), tce(" bytes)"));

    auto loaded_node = mcx::xml_file_loader::parse_file_with_auto_encoding(temp_file.string());
    assert(loaded_node != nullptr && loaded_node->name == "root");
    print(tce("xml_file_loader::parse_file_with_auto_encoding 통과"));

    auto parsed_from_file = default_parser.parse_file(temp_file);
    assert(parsed_from_file != nullptr && parsed_from_file->name == "root");
    print(tce("xml_parser::parse_file 통과"));

    // 임시 파일 삭제
    if (std::filesystem::exists(temp_file))
    {
        std::filesystem::remove(temp_file);
    }

    print(endl, tce("================================================="));
    print(tce(" 모든 퍼블릭 멤버 및 API 검증이 완료되었습니다. "));
    print(tce("================================================="));

    return 0;
}
