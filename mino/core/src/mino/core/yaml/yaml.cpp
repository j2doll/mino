#include "mino/core/yaml/yaml.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <charconv>
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace mino::core::yaml {

    // =============================================================
    // node 멤버 함수 구현 (순서 보존 vector<pair> 대응)
    // =============================================================
    node::node() : data(null_type{}) {}
    node::node(null_type v) : data(v) {}
    node::node(bool v) : data(v) {}
    node::node(int64_t v) : data(v) {}
    node::node(int v) : data(static_cast<int64_t>(v)) {}
    node::node(double v) : data(v) {}
    node::node(std::string v) : data(std::move(v)) {}
    node::node(const char* v) : data(std::string(v)) {}
    node::node(sequence v) : data(std::move(v)) {}
    node::node(mapping v) : data(std::move(v)) {}

    bool node::is_null()   const { return std::holds_alternative<null_type>(data); }
    bool node::is_bool()   const { return std::holds_alternative<bool>(data); }
    bool node::is_int()    const { return std::holds_alternative<int64_t>(data); }
    bool node::is_double() const { return std::holds_alternative<double>(data); }
    bool node::is_string() const { return std::holds_alternative<std::string>(data); }
    bool node::is_scalar() const { return data.index() >= 1 && data.index() <= 4; }
    bool node::is_seq()    const { return std::holds_alternative<sequence>(data); }
    bool node::is_map()    const { return std::holds_alternative<mapping>(data); }

    bool node::has_key(const std::string& key) const {
        if (!is_map()) return false;
        const auto& map = std::get<mapping>(data);
        return std::any_of(map.begin(), map.end(), [&](const auto& pair) {
            return pair.first == key;
            });
    }

    size_t node::size() const {
        if (is_seq()) return std::get<sequence>(data).size();
        if (is_map()) return std::get<mapping>(data).size();
        return is_null() ? 0 : 1;
    }

    node& node::operator[](const std::string& key) {
        if (is_null()) data = mapping{};
        if (!is_map()) {
            throw std::runtime_error("Node is not a mapping type");
        }
        auto& map = std::get<mapping>(data);
        for (auto& pair : map) {
            if (pair.first == key) return pair.second;
        }
        map.emplace_back(key, node{});
        return map.back().second;
    }

    const node& node::operator[](const std::string& key) const {
        if (!is_map()) {
            throw std::runtime_error("Node is not a mapping type");
        }
        const auto& map = std::get<mapping>(data);
        for (const auto& pair : map) {
            if (pair.first == key) return pair.second;
        }
        throw std::out_of_range("Key not found in YAML mapping: " + key);
    }

    node& node::operator[](size_t index) {
        if (!is_seq()) {
            throw std::runtime_error("Node is not a sequence type");
        }
        return std::get<sequence>(data).at(index);
    }

    const node& node::operator[](size_t index) const {
        if (!is_seq()) {
            throw std::runtime_error("Node is not a sequence type");
        }
        return std::get<sequence>(data).at(index);
    }

    void node::push_back(node item) {
        if (is_null()) data = sequence{};
        if (!is_seq()) {
            throw std::runtime_error("Node is not a sequence type");
        }
        std::get<sequence>(data).push_back(std::move(item));
    }

    // =============================================================
    // parser 클래스 구현
    // =============================================================
    std::string_view parser::trim(std::string_view s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    node parser::parse_scalar(std::string_view sv) {
        sv = trim(sv);
        if (sv.empty() || sv == "~" || sv == "null") return node{ null_type{} };

        // 1. 따옴표로 감싸진 문자열은 공백/숫자 여부와 무관하게 항상 string으로 취급
        if (sv.size() >= 2 && ((sv.front() == '"' && sv.back() == '"') || (sv.front() == '\'' && sv.back() == '\''))) {
            return node{ std::string(sv.substr(1, sv.size() - 2)) };
        }

        if (sv == "true") return node{ true };
        if (sv == "false") return node{ false };

        // 2. 정수 파싱
        int64_t int_val = 0;
        auto [p1, ec1] = std::from_chars(sv.data(), sv.data() + sv.size(), int_val);
        if (ec1 == std::errc{} && p1 == sv.data() + sv.size()) {
            return node{ int_val };
        }

        // 3. 실수 파싱 (소수점 '.' 또는 지수 'e/E'가 포함된 경우)
        if (sv.find('.') != std::string_view::npos || sv.find('e') != std::string_view::npos || sv.find('E') != std::string_view::npos) {
            try {
                size_t idx = 0;
                std::string s(sv);
                double d = std::stod(s, &idx);
                if (idx == s.size()) return node{ d };
            }
            catch (...) {}
        }

        // 4. 공백을 포함한 일반 텍스트 문자열
        return node{ std::string(sv) };
    }

    std::vector<parser::line> parser::tokenize_lines(std::string_view src) {
        std::vector<line> lines;
        size_t pos = 0;
        while (pos < src.size()) {
            size_t next = src.find('\n', pos);
            if (next == std::string_view::npos) next = src.size();
            std::string_view raw = src.substr(pos, next - pos);
            pos = next + 1;

            // 주석(#) 제거 (단, 따옴표 내부의 #은 무시하는 간단한 보호 로직)
            bool in_double_quote = false;
            bool in_single_quote = false;
            size_t comment_pos = std::string_view::npos;
            for (size_t i = 0; i < raw.size(); ++i) {
                if (raw[i] == '"' && !in_single_quote) in_double_quote = !in_double_quote;
                else if (raw[i] == '\'' && !in_double_quote) in_single_quote = !in_single_quote;
                else if (raw[i] == '#' && !in_double_quote && !in_single_quote) {
                    comment_pos = i;
                    break;
                }
            }

            if (comment_pos != std::string_view::npos) {
                raw = raw.substr(0, comment_pos);
            }

            size_t first_char = raw.find_first_not_of(" ");
            if (first_char != std::string_view::npos) {
                lines.push_back({ first_char, trim(raw) });
            }
        }
        return lines;
    }

    node parser::parse_block(const std::vector<line>& lines, size_t& idx, size_t base_indent) {
        bool is_seq = (lines[idx].text.rfind("- ", 0) == 0 || lines[idx].text == "-");
        node root;

        if (is_seq) {
            sequence seq;
            while (idx < lines.size()) {
                if (lines[idx].indent < base_indent) break;
                if (lines[idx].indent > base_indent) break;

                std::string_view item_text = lines[idx].text;
                if (item_text.rfind("- ", 0) == 0) {
                    item_text = trim(item_text.substr(2));
                }
                else if (item_text == "-") {
                    item_text = "";
                }
                else {
                    break;
                }

                idx++;
                if (!item_text.empty()) {
                    seq.push_back(parse_scalar(item_text));
                }
                else if (idx < lines.size() && lines[idx].indent > base_indent) {
                    seq.push_back(parse_block(lines, idx, lines[idx].indent));
                }
                else {
                    seq.push_back(node{ null_type{} });
                }
            }
            root = std::move(seq);
        }
        else {
            mapping map;
            while (idx < lines.size()) {
                if (lines[idx].indent < base_indent) break;

                size_t colon_pos = lines[idx].text.find(':');
                if (colon_pos == std::string_view::npos) break;

                std::string key(trim(lines[idx].text.substr(0, colon_pos)));
                std::string_view val_part = trim(lines[idx].text.substr(colon_pos + 1));

                idx++;
                if (!val_part.empty()) {
                    map.emplace_back(key, parse_scalar(val_part));
                }
                else if (idx < lines.size() && lines[idx].indent > base_indent) {
                    map.emplace_back(key, parse_block(lines, idx, lines[idx].indent));
                }
                else {
                    map.emplace_back(key, node{ null_type{} });
                }
            }
            root = std::move(map);
        }

        return root;
    }

    node parser::parse(std::string_view yaml_str) {
        auto lines = tokenize_lines(yaml_str);
        if (lines.empty()) return node{};
        size_t idx = 0;
        return parse_block(lines, idx, lines[0].indent);
    }

    node parser::parse_file(const std::filesystem::path& file_path) {
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open YAML file for reading: " + file_path.string());
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return parse(std::string_view(ss.str()));
    }

    // =============================================================
    // emitter 클래스 구현
    // =============================================================
    void emitter::emit_node(const node& n, std::ostream& out, int indent_level) {
        std::string indent(indent_level * 2, ' ');

        std::visit([&](auto&& arg) {
            using target_type = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<target_type, null_type>) {
                out << "null\n";
            }
            else if constexpr (std::is_same_v<target_type, bool>) {
                out << (arg ? "true\n" : "false\n");
            }
            else if constexpr (std::is_same_v<target_type, int64_t>) {
                out << arg << "\n";
            }
            else if constexpr (std::is_same_v<target_type, double>) {
                std::ostringstream dss;
                dss << arg;
                std::string s = dss.str();
                // 1.0 같은 값이 1로 출력되는 것을 방지하기 위해 .0 붙이기
                if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
                    s += ".0";
                }
                out << s << "\n";
            }
            else if constexpr (std::is_same_v<target_type, std::string>) {
                // 공백, 특수문자가 포함된 경우 따옴표로 감싸기
                bool need_quotes = arg.empty() ||
                    arg.find(' ') != std::string::npos ||
                    arg.find(':') != std::string::npos ||
                    arg.find('#') != std::string::npos ||
                    arg.find('\t') != std::string::npos ||
                    arg == "true" || arg == "false" || arg == "null" || arg == "~";
                if (need_quotes) {
                    out << "\"" << arg << "\"\n";
                }
                else {
                    out << arg << "\n";
                }
            }
            else if constexpr (std::is_same_v<target_type, sequence>) {
                for (const auto& item : arg) {
                    if (item.is_map() || item.is_seq()) {
                        out << indent << "-\n";
                        emit_node(item, out, indent_level + 1);
                    }
                    else {
                        out << indent << "- ";
                        emit_node(item, out, indent_level + 1);
                    }
                }
            }
            else if constexpr (std::is_same_v<target_type, mapping>) {
                for (const auto& [k, v] : arg) {
                    out << indent << k << ":";
                    if (v.is_map() || v.is_seq()) {
                        out << "\n";
                        emit_node(v, out, indent_level + 1);
                    }
                    else {
                        out << " ";
                        emit_node(v, out, indent_level + 1);
                    }
                }
            }
            }, n.data);
    }

    void emitter::dump(const node& root, std::ostream& out) {
        emit_node(root, out, 0);
    }

    std::string emitter::dump(const node& root) {
        std::ostringstream ss;
        dump(root, ss);
        return ss.str();
    }

    void emitter::dump_file(const node& root, const std::filesystem::path& file_path) {
        std::ofstream file(file_path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open YAML file for writing: " + file_path.string());
        }
        dump(root, file);
    }

} // namespace mino::core::yaml
