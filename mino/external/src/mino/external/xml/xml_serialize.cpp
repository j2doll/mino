#include <string>
#include <sstream>

#include "mino/external/xml/xml_serialize.hpp"

namespace mino::external::xml
{
    namespace {
        static std::string escape_attr(const std::string& s) {
            std::string out;
            out.reserve(s.size());
            for (unsigned char c : s) {
                switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '\"': out += "&quot;"; break;
                case '\'': out += "&apos;"; break;
                default: out.push_back(static_cast<char>(c)); break;
                }
            }
            return out;
        }

        static std::string escape_text(const std::string& s) {
            std::string out;
            out.reserve(s.size());
            for (unsigned char c : s) {
                switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                default: out.push_back(static_cast<char>(c)); break;
                }
            }
            return out;
        }

        static void serialize_node(const xml_node& node, std::ostringstream& ss, int indent) {
            for (int i = 0; i < indent; ++i) ss.put(' ');

            ss << '<' << node.name;

            for (const auto& a : node.attributes) {
                ss << ' ' << a.name << "=\"" << escape_attr(a.value) << '"';
            }

            bool has_children = !node.children.empty();
            bool has_text = !node.text.empty();

            if (!has_children && !has_text) {
                ss << " />\n";
                return;
            }

            ss << '>';

            if (has_text) {
                ss << escape_text(node.text);
            }

            if (has_children) {
                ss << '\n';
                for (const auto& child : node.children) {
                    serialize_node(*child, ss, indent + 2);
                }
                for (int i = 0; i < indent; ++i) ss.put(' ');
            }

            ss << "</" << node.name << ">\n";
        }
    } // namespace

    std::string serialize_xml(const xml_node& node, bool include_declaration) {
        std::ostringstream ss;
        if (include_declaration) {
            ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        }
        serialize_node(node, ss, 0);
        return ss.str();
    }


}