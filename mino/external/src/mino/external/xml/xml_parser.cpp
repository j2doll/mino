#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "mino/external/xml/xml_parser.hpp"

#ifndef _WIN32
    #include <iconv.h>
    #include <errno.h>
#else
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif 
    #include <windows.h>
#endif

namespace mino::external::xml
{
    // -----------------------------
    // xml_parser 구현
    // -----------------------------

    // 기본 생성자
    xml_parser::xml_parser()
        : xml_(),
        pos_(0),
        len_(0),
        text_policy_(text_policy::trim_and_discard_empty)
    {
    }

    // XML 문자열을 바로 넘기는 생성자
    xml_parser::xml_parser(const std::string& xml, text_policy policy)
        : xml_(xml),
        pos_(0),
        len_(xml.size()),
        text_policy_(policy)
    {
    }

    bool xml_parser::peek_char(char c) const
    {
        return (pos_ < len_ && xml_[pos_] == c);
    }

    char xml_parser::current_char() const
    {
        if (pos_ >= len_)
            throw std::runtime_error("Attempted to read past end of input.");
        return xml_[pos_];
    }

    char xml_parser::get_char()
    {
        if (pos_ >= len_)
            throw std::runtime_error("Attempted to read past end of input.");
        return xml_[pos_++];
    }

    void xml_parser::skip_whitespace()
    {
        while (pos_ < len_ && std::isspace(static_cast<unsigned char>(xml_[pos_])))
        {
            ++pos_;
        }
    }

    bool xml_parser::starts_with(const std::string& s) const
    {
        if (pos_ + s.size() > len_)
            return false;
        return std::equal(s.begin(), s.end(), xml_.begin() + static_cast<std::ptrdiff_t>(pos_));
    }

    void xml_parser::expect(const std::string& s)
    {
        if (!starts_with(s))
            throw std::runtime_error("Unexpected token: " + s);
        pos_ += s.size();
    }

    std::string xml_parser::parse_raw_name()
    {
        if (pos_ >= len_)
            throw std::runtime_error("Unexpected end of input while parsing name");

        std::size_t start = pos_;

        auto is_name_char = [](char ch) {
            return std::isalnum(static_cast<unsigned char>(ch)) ||
                ch == '_' || ch == '-' || ch == ':' || ch == '.';
            };

        if (!is_name_char(xml_[pos_]))
            throw std::runtime_error("Invalid start character for name");

        ++pos_;
        while (pos_ < len_ && is_name_char(xml_[pos_]))
            ++pos_;

        return xml_.substr(start, pos_ - start);
    }

    void xml_parser::split_qname(const std::string& raw_name,
        std::string& out_prefix,
        std::string& out_local)
    {
        auto pos = raw_name.find(':');
        if (pos == std::string::npos)
        {
            out_prefix.clear();
            out_local = raw_name;
        }
        else
        {
            out_prefix = raw_name.substr(0, pos);
            out_local = raw_name.substr(pos + 1);
        }
    }

    std::string xml_parser::parse_attribute_value()
    {
        skip_whitespace();
        if (pos_ >= len_)
            throw std::runtime_error("Attribute value start not found.");

        char quote = xml_[pos_];
        if (quote != '"' && quote != '\'')
            throw std::runtime_error("Attribute value must be quoted.");

        ++pos_;
        std::size_t start = pos_;

        while (pos_ < len_ && xml_[pos_] != quote)
            ++pos_;

        if (pos_ >= len_)
            throw std::runtime_error("Attribute value not terminated.");

        std::string value = xml_.substr(start, pos_ - start);
        ++pos_;

        return decode_entities(value);
    }

    std::string xml_parser::trim(const std::string& s)
    {
        return trim_spaces(s);
    }

    std::string xml_parser::decode_entities(const std::string& s)
    {
        std::string result;
        result.reserve(s.size());

        for (std::size_t i = 0; i < s.size();)
        {
            if (s[i] == '&')
            {
                if (s.compare(i, 4, "&lt;") == 0)
                {
                    result.push_back('<');
                    i += 4;
                }
                else if (s.compare(i, 4, "&gt;") == 0)
                {
                    result.push_back('>');
                    i += 4;
                }
                else if (s.compare(i, 5, "&amp;") == 0)
                {
                    result.push_back('&');
                    i += 5;
                }
                else if (s.compare(i, 6, "&quot;") == 0)
                {
                    result.push_back('"');
                    i += 6;
                }
                else if (s.compare(i, 6, "&apos;") == 0)
                {
                    result.push_back('\'');
                    i += 6;
                }
                else
                {
                    result.push_back(s[i]);
                    ++i;
                }
            }
            else
            {
                result.push_back(s[i]);
                ++i;
            }
        }

        return result;
    }

    std::string xml_parser::read_text()
    {
        std::size_t start = pos_;
        while (pos_ < len_ && xml_[pos_] != '<')
            ++pos_;
        return xml_.substr(start, pos_ - start);
    }

    std::string xml_parser::parse_cdata()
    {
        expect("<![CDATA[");
        std::size_t start = pos_;
        while (pos_ + 2 < len_ && !starts_with("]]>"))
            ++pos_;

        if (pos_ + 2 >= len_)
            throw std::runtime_error("CDATA section not terminated.");

        std::string value = xml_.substr(start, pos_ - start);
        expect("]]>");
        return value;
    }

    void xml_parser::skip_optional_xml_declaration_and_misc()
    {
        while (true)
        {
            skip_whitespace();
            if (!peek_char('<'))
                return;

            if (starts_with("<?"))
            {
                skip_processing_instruction();
            }
            else if (starts_with("<!--"))
            {
                skip_comment();
            }
            else
            {
                return;
            }
        }
    }

    void xml_parser::skip_processing_instruction()
    {
        expect("<?");
        while (pos_ + 1 < len_ && !starts_with("?>"))
            ++pos_;

        if (pos_ + 1 >= len_)
            throw std::runtime_error("Processing instruction not terminated.");

        expect("?>");
    }

    void xml_parser::skip_comment()
    {
        expect("<!--");
        while (pos_ + 2 < len_ && !starts_with("-->"))
            ++pos_;

        if (pos_ + 2 >= len_)
            throw std::runtime_error("Comment not terminated.");

        expect("-->");
    }

    void xml_parser::parse_attributes(xml_node& node,
        std::unordered_map<std::string, std::string>& ns_map)
    {
        while (true)
        {
            skip_whitespace();
            if (pos_ >= len_)
                throw std::runtime_error("Unexpected end of input before tag end");

            char c = xml_[pos_];
            if (c == '/' || c == '>')
                break;

            std::string raw_name = parse_raw_name();
            skip_whitespace();

            if (!peek_char('='))
                throw std::runtime_error("Attribute missing '='");

            get_char();
            std::string value = parse_attribute_value();

            if (raw_name == "xmlns")
            {
                ns_map[""] = value;
            }
            else if (raw_name.rfind("xmlns:", 0) == 0)
            {
                std::string prefix = raw_name.substr(6);
                ns_map[prefix] = value;
            }
            else
            {
                xml_attribute attr;
                std::string attr_prefix;
                std::string attr_local;

                split_qname(raw_name, attr_prefix, attr_local);

                attr.name = attr_local;
                attr.prefix = attr_prefix;
                if (!attr_prefix.empty())
                {
                    auto it = ns_map.find(attr_prefix);
                    if (it != ns_map.end())
                        attr.ns_uri = it->second;
                }
                attr.value = value;
                node.attributes.push_back(std::move(attr));
            }
        }
    }

    void xml_parser::flush_text_buffer(std::string& text_buffer, xml_node& node)
    {
        if (text_buffer.empty())
            return;

        std::string raw = text_buffer;
        text_buffer.clear();

        switch (text_policy_)
        {
        case text_policy::trim_and_discard_empty:
        {
            std::string trimmed = trim(raw);
            if (trimmed.empty())
                return;

            std::string decoded = decode_entities(trimmed);
            if (!node.text.empty())
                node.text.push_back(' ');
            node.text += decoded;
            break;
        }
        case text_policy::preserve:
        {
            std::string decoded = decode_entities(raw);
            node.text += decoded;
            break;
        }
        case text_policy::collapse_whitespace_only:
        {
            bool all_space = true;
            for (char ch : raw)
            {
                if (!std::isspace(static_cast<unsigned char>(ch)))
                {
                    all_space = false;
                    break;
                }
            }

            if (all_space)
            {
                if (!node.text.empty())
                    node.text.push_back(' ');
                else
                    node.text.push_back(' ');
            }
            else
            {
                std::string trimmed = trim(raw);
                if (trimmed.empty())
                    return;
                std::string decoded = decode_entities(trimmed);
                if (!node.text.empty())
                    node.text.push_back(' ');
                node.text += decoded;
            }
            break;
        }
        }
    }

    std::unique_ptr<xml_node> xml_parser::parse_element(std::unordered_map<std::string, std::string> ns_map)
    {
        if (!peek_char('<'))
            throw std::runtime_error("Element start '<' not found.");

        get_char(); // '<'

        if (peek_char('/'))
            throw std::runtime_error("Unexpected end tag.");

        std::string raw_name = parse_raw_name();
        std::string prefix;
        std::string local_name;
        split_qname(raw_name, prefix, local_name);

        auto node = std::make_unique<xml_node>();
        node->name = local_name;
        node->prefix = prefix;

        skip_whitespace();
        parse_attributes(*node, ns_map);

        {
            std::string key = node->prefix;
            auto it = ns_map.find(key);
            if (it != ns_map.end())
                node->ns_uri = it->second;
        }

        if (starts_with("/>"))
        {
            expect("/>");
            return node;
        }

        if (!peek_char('>'))
            throw std::runtime_error("Tag '>' expected.");

        get_char(); // '>'

        std::string text_buffer;

        while (pos_ < len_)
        {
            if (peek_char('<'))
            {
                flush_text_buffer(text_buffer, *node);

                if (starts_with("</"))
                {
                    expect("</");
                    std::string end_raw = parse_raw_name();
                    std::string end_prefix;
                    std::string end_local;
                    split_qname(end_raw, end_prefix, end_local);

                    skip_whitespace();
                    if (!peek_char('>'))
                        throw std::runtime_error("End tag '>' expected.");
                    get_char(); // '>'

                    if (end_local != node->name)
                        throw std::runtime_error("Start/end tag names do not match: " +
                            node->name + " vs " + end_local);

                    break;
                }
                else if (starts_with("<!--"))
                {
                    skip_comment();
                }
                else if (starts_with("<?"))
                {
                    skip_processing_instruction();
                }
                else if (starts_with("<![CDATA["))
                {
                    std::string cdata_text = parse_cdata();
                    if (!node->text.empty())
                        node->text.push_back(' ');
                    node->text += cdata_text;
                }
                else
                {
                    auto child = parse_element(ns_map);
                    node->children.push_back(std::move(child));
                }
            }
            else
            {
                text_buffer += read_text();
            }
        }

        flush_text_buffer(text_buffer, *node);

        return node;
    }

    // 현재 xml_ 멤버에 들어 있는 내용을 파싱
    std::unique_ptr<xml_node> xml_parser::parse()
    {
        if (xml_.empty())
            throw std::runtime_error("XML string to parse is empty.");

        pos_ = 0;
        len_ = xml_.size();

        skip_whitespace();
        skip_optional_xml_declaration_and_misc();
        skip_whitespace();

        if (!peek_char('<'))
            throw std::runtime_error("Root element not found.");

        std::unordered_map<std::string, std::string> ns_map;
        auto root = parse_element(ns_map);
        skip_whitespace();
        return root;
    }

    // 새 XML 문자열과 정책을 넘겨서 곧바로 파싱
    std::unique_ptr<xml_node> xml_parser::parse(const std::string& xml,
        text_policy policy)
    {
        xml_ = xml;
        text_policy_ = policy;
        return parse();
    }

    // 파일 경로(std::filesystem::path)를 받아서 파싱
    std::unique_ptr<xml_node> xml_parser::parse_file(
        const std::filesystem::path& file_path,
        text_policy policy)
    {
        namespace fs = std::filesystem;

        const std::string path_str = file_path.string();

        if (!fs::exists(file_path))
        {
            throw std::runtime_error("XML file does not exist: " + path_str);
        }
        if (!fs::is_regular_file(file_path))
        {
            throw std::runtime_error("XML path is not a regular file: " + path_str);
        }

        std::string raw_xml;
        try
        {
            raw_xml = xml_file_loader::read_file_binary(path_str);
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(
                std::string("Error reading XML file: ") +
                path_str + " / details: " + ex.what());
        }

        std::string utf8_xml;
        try
        {
            utf8_xml = convert_xml_to_utf8(raw_xml);
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(
                std::string("Error converting XML encoding: ") +
                path_str + " / details: " + ex.what());
        }

        return parse(utf8_xml, policy);
    }


    std::unique_ptr<xml_node> parse_with_auto_encoding(const std::string& raw_xml,
        text_policy policy)
    {
        std::string utf8_xml = convert_xml_to_utf8(raw_xml);

        xml_parser parser(utf8_xml, policy);
        return parser.parse();
    }

}  