#include <stdexcept>

#include "mino/core/xml/xml_writer.hpp"

namespace mino::core::xml
{
    xml_writer::xml_writer(const write_options& options)
        : options_(options)
    {
    }

    std::string xml_writer::encode_entities(const std::string& s)
    {
        std::string result;
        result.reserve(s.size());

        for (char ch : s)
        {
            switch (ch)
            {
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '&':  result += "&amp;";  break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result.push_back(ch); break;
            }
        }
        return result;
    }

    void xml_writer::append_indent(std::string& out_xml, int indent_level) const
    {
        if (!options_.pretty_print || indent_level < 0)
            return;

        out_xml.append(static_cast<std::size_t>(indent_level * options_.indent_spaces), ' ');
    }

    void xml_writer::write_element(const xml_node& node, std::string& out_xml, int indent_level)
    {
        // 1. 시작 태그 들여쓰기
        append_indent(out_xml, indent_level);

        // 2. 태그 이름 빌드 (접두사 포함)
        out_xml += "<";
        if (!node.prefix.empty())
        {
            out_xml += node.prefix;
            out_xml += ":";
        }
        out_xml += node.name;

        // 3. 속성 출력
        for (const auto& attr : node.attributes)
        {
            out_xml += " ";
            if (!attr.prefix.empty())
            {
                out_xml += attr.prefix;
                out_xml += ":";
            }
            out_xml += attr.name;
            out_xml += "=\"";
            out_xml += encode_entities(attr.value);
            out_xml += "\"";
        }

        // 4. 자식 노드와 텍스트가 모두 없는 경우 -> 자가 종료 태그(Self-closing tag) 처리
        if (node.children.empty() && node.text.empty())
        {
            out_xml += " />";
            if (options_.pretty_print)
            {
                out_xml += "\n";
            }
            return;
        }

        out_xml += ">";

        // 5. 내부 콘텐츠(텍스트 혹은 자식 노드) 처리
        if (!node.text.empty())
        {
            // 텍스트가 있으면 에스케이프하여 출력 (인라인으로 배치)
            out_xml += encode_entities(node.text);
        }
        else if (!node.children.empty())
        {
            // 자식 노드가 있으면 줄바꿈 후 재귀 호출
            if (options_.pretty_print)
            {
                out_xml += "\n";
            }
            for (const auto& child : node.children)
            {
                if (child)
                {
                    write_element(*child, out_xml, indent_level + 1);
                }
            }
            // 종료 태그 앞에 들여쓰기 추가
            append_indent(out_xml, indent_level);
        }

        // 6. 종료 태그 빌드
        out_xml += "</";
        if (!node.prefix.empty())
        {
            out_xml += node.prefix;
            out_xml += ":";
        }
        out_xml += node.name;
        out_xml += ">";

        if (options_.pretty_print)
        {
            out_xml += "\n";
        }
    }

    std::string xml_writer::write_to_string(const xml_node& root)
    {
        std::string out_xml;

        // XML 선언문 추가
        if (options_.include_declaration)
        {
            out_xml += "<?xml version=\"1.0\" encoding=\"";
            out_xml += options_.encoding;
            out_xml += "\"?>";
            if (options_.pretty_print)
            {
                out_xml += "\n";
            }
        }

        // 루트 엘리먼트부터 생성 시작
        write_element(root, out_xml, 0);

        // 파서 내에 구현된 인코딩 변환 시스템이 존재하므로, 
        // 만약 UTF-8이 아닌 인코딩(예: EUC-KR)이 지정되었다면 변환하여 반환
        if (options_.encoding != "UTF-8" && options_.encoding != "UTF8")
        {
            return convert_text_encoding_to_utf8(out_xml, options_.encoding);
        }

        return out_xml;
    }

    bool xml_writer::write_to_file(const std::filesystem::path& file_path, const xml_node& root)
    {
        std::string xml_content = write_to_string(root);

        std::ofstream ofs(file_path, std::ios::out | std::ios::binary);
        if (!ofs)
        {
            return false; 
        }

        ofs.write(xml_content.data(), static_cast<std::streamsize>(xml_content.size()));
        if (!ofs)
        {
            return false;
        }
        return true;
    }

}  
