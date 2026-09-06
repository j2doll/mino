#include "mino/core/toml/toml_serializer.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace mino::core::toml {

    // ===========================================================================
    // Lexer 구현
    // ===========================================================================

    lexer::lexer(std::string_view source) : src(source) {}

    char lexer::peek(int offset) const {
        int64_t target = static_cast<int64_t>(pos) + offset;
        if (target < 0 || target >= static_cast<int64_t>(src.size())) return '\0';
        return src[target];
    }

    char lexer::advance() {
        if (pos >= src.size()) return '\0';
        char c = src[pos++];
        if (c == '\n') cur_line++;
        return c;
    }

    token lexer::next_token() {
        while (pos < src.size()) {
            char c = peek();

            if (c == ' ' || c == '\t' || c == '\r') {
                advance();
                continue;
            }

            if (c == '#') {
                while (pos < src.size() && peek() != '\n') {
                    advance();
                }
                continue;
            }

            if (c == '\n') {
                advance();
                return { token_type::newline, "\n", 0, 0.0, false, {}, cur_line - 1 };
            }

            if (c == '=') { advance(); return { token_type::equals, "=", 0, 0.0, false, {}, cur_line }; }
            if (c == '.') { advance(); return { token_type::dot, ".", 0, 0.0, false, {}, cur_line }; }
            if (c == ',') { advance(); return { token_type::comma, ",", 0, 0.0, false, {}, cur_line }; }
            if (c == '{') { advance(); return { token_type::lbrace, "{", 0, 0.0, false, {}, cur_line }; }
            if (c == '}') { advance(); return { token_type::rbrace, "}", 0, 0.0, false, {}, cur_line }; }

            if (c == '[') {
                if (peek(1) == '[') {
                    advance(); advance();
                    return { token_type::double_lbracket, "[[", 0, 0.0, false, {}, cur_line };
                }
                advance();
                return { token_type::lbracket, "[", 0, 0.0, false, {}, cur_line };
            }

            if (c == ']') {
                if (peek(1) == ']') {
                    advance(); advance();
                    return { token_type::double_rbracket, "]]", 0, 0.0, false, {}, cur_line };
                }
                advance();
                return { token_type::rbracket, "]", 0, 0.0, false, {}, cur_line };
            }

            if (c == '"' || c == '\'') {
                return parse_string();
            }

            return parse_bare_or_literal();
        }

        return { token_type::eof, "", 0, 0.0, false, {}, cur_line };
    }

    void lexer::append_utf8(std::string& out, uint32_t cp) {
        if (cp <= 0x7F) {
            out += static_cast<char>(cp);
        }
        else if (cp <= 0x7FF) {
            out += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp <= 0xFFFF) {
            out += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else {
            out += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    token lexer::parse_string() {
        char quote = advance();
        bool multiline = (peek() == quote && peek(1) == quote);
        if (multiline) {
            advance(); advance();
            if (peek() == '\n') advance();
        }

        std::string result;
        while (pos < src.size()) {
            if (multiline) {
                if (peek() == quote && peek(1) == quote && peek(2) == quote) {
                    advance(); advance(); advance();
                    return { token_type::string_literal, result, 0, 0.0, false, {}, cur_line };
                }
            }
            else {
                if (peek() == quote) {
                    advance();
                    return { token_type::string_literal, result, 0, 0.0, false, {}, cur_line };
                }
                if (peek() == '\n') {
                    throw std::runtime_error("Newline in single-line string at line " + std::to_string(cur_line));
                }
            }

            char cur = advance();
            if (quote == '"' && cur == '\\') {
                if (multiline && peek() == '\n') {
                    advance();
                    while (pos < src.size() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n')) {
                        advance();
                    }
                    continue;
                }
                char esc = advance();
                switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    uint32_t cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = advance();
                        cp = (cp << 4) | (std::isdigit(h) ? h - '0' : std::tolower(h) - 'a' + 10);
                    }
                    append_utf8(result, cp);
                    break;
                }
                case 'U': {
                    uint32_t cp = 0;
                    for (int i = 0; i < 8; ++i) {
                        char h = advance();
                        cp = (cp << 4) | (std::isdigit(h) ? h - '0' : std::tolower(h) - 'a' + 10);
                    }
                    append_utf8(result, cp);
                    break;
                }
                default: result += esc; break;
                }
            }
            else {
                result += cur;
            }
        }
        throw std::runtime_error("Unterminated string at line " + std::to_string(cur_line));
    }

    bool lexer::parse_rfc3339(const std::string& s, date_time& dt) {
        int y, M, d, h, m, sec;
        if (sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &sec) >= 6 ||
            sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &M, &d, &h, &m, &sec) >= 6) {
            dt.year = y; dt.month = M; dt.day = d;
            dt.hour = h; dt.minute = m; dt.second = sec;
            dt.has_date = true; dt.has_time = true;
            if (s.back() == 'Z') { dt.has_offset = true; dt.offset_minutes = 0; }
            return true;
        }
        if (sscanf(s.c_str(), "%d-%d-%d", &y, &M, &d) == 3) {
            dt.year = y; dt.month = M; dt.day = d;
            dt.has_date = true;
            return true;
        }
        if (sscanf(s.c_str(), "%d:%d:%d", &h, &m, &sec) == 3) {
            dt.hour = h; dt.minute = m; dt.second = sec;
            dt.has_time = true;
            return true;
        }
        return false;
    }

    token lexer::parse_bare_or_literal() {
        size_t start = pos;
        while (pos < src.size()) {
            char c = peek();
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '+' || c == ':' || c == 'Z' || c == 'T') {
                advance();
            }
            else if (c == '.' && pos > start) {
                if (std::isdigit(static_cast<unsigned char>(peek(-1)))) {
                    advance();
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
        std::string raw(src.substr(start, pos - start));

        if (raw == "true") return { token_type::boolean_literal, raw, 0, 0.0, true, {}, cur_line };
        if (raw == "false") return { token_type::boolean_literal, raw, 0, 0.0, false, {}, cur_line };
        if (raw == "inf" || raw == "+inf") return { token_type::float_literal, raw, 0, INFINITY, false, {}, cur_line };
        if (raw == "-inf") return { token_type::float_literal, raw, 0, -INFINITY, false, {}, cur_line };
        if (raw == "nan" || raw == "+nan") return { token_type::float_literal, raw, 0, NAN, false, {}, cur_line };
        if (raw == "-nan") return { token_type::float_literal, raw, 0, -NAN, false, {}, cur_line };

        if (raw.size() >= 8 && (raw.find('-') != std::string::npos || raw.find(':') != std::string::npos)) {
            date_time dt;
            if (parse_rfc3339(raw, dt)) {
                return { token_type::datetime_literal, raw, 0, 0.0, false, dt, cur_line };
            }
        }

        std::string cleaned;
        for (char ch : raw) if (ch != '_') cleaned += ch;

        char* end_ptr = nullptr;
        if (cleaned.rfind("0x", 0) == 0 || cleaned.rfind("0X", 0) == 0) {
            int64_t v = std::strtoll(cleaned.c_str() + 2, &end_ptr, 16);
            if (end_ptr == cleaned.c_str() + cleaned.size()) return { token_type::integer_literal, raw, v, 0.0, false, {}, cur_line };
        }
        else if (cleaned.rfind("0o", 0) == 0 || cleaned.rfind("0O", 0) == 0) {
            int64_t v = std::strtoll(cleaned.c_str() + 2, &end_ptr, 8);
            if (end_ptr == cleaned.c_str() + cleaned.size()) return { token_type::integer_literal, raw, v, 0.0, false, {}, cur_line };
        }
        else if (cleaned.rfind("0b", 0) == 0 || cleaned.rfind("0B", 0) == 0) {
            int64_t v = std::strtoll(cleaned.c_str() + 2, &end_ptr, 2);
            if (end_ptr == cleaned.c_str() + cleaned.size()) return { token_type::integer_literal, raw, v, 0.0, false, {}, cur_line };
        }

        int64_t i_val = std::strtoll(cleaned.c_str(), &end_ptr, 10);
        if (end_ptr == cleaned.c_str() + cleaned.size()) {
            return { token_type::integer_literal, raw, i_val, 0.0, false, {}, cur_line };
        }

        double d_val = std::strtod(cleaned.c_str(), &end_ptr);
        if (end_ptr == cleaned.c_str() + cleaned.size()) {
            return { token_type::float_literal, raw, 0, d_val, false, {}, cur_line };
        }

        return { token_type::identifier, raw, 0, 0.0, false, {}, cur_line };
    }

    // ===========================================================================
    // Parser 구현
    // ===========================================================================

    parser::parser(std::string_view src) : lex(src) {
        advance();
    }

    toml_table parser::parse() {
        toml_table root;
        toml_table* cur_table = &root;

        while (cur_tok.type != token_type::eof) {
            skip_newlines();
            if (cur_tok.type == token_type::eof) break;

            if (cur_tok.type == token_type::lbracket) {
                advance();
                auto keys = parse_key_path();
                consume(token_type::rbracket, "Expected ']'");
                cur_table = &navigate_to_table(root, keys);
            }
            else if (cur_tok.type == token_type::double_lbracket) {
                advance();
                auto keys = parse_key_path();
                consume(token_type::double_rbracket, "Expected ']]'");
                cur_table = &append_array_table(root, keys);
            }
            else {
                auto keys = parse_key_path();
                consume(token_type::equals, "Expected '='");
                toml_value val = parse_value();
                assign_value_at_path(*cur_table, keys, std::move(val));
            }
        }
        return root;
    }

    void parser::advance() {
        cur_tok = lex.next_token();
    }

    void parser::skip_newlines() {
        while (cur_tok.type == token_type::newline) {
            advance();
        }
    }

    void parser::consume(token_type type, const std::string& err) {
        if (cur_tok.type != type) {
            throw std::runtime_error(err + " at line " + std::to_string(cur_tok.line));
        }
        advance();
    }

    std::vector<std::string> parser::parse_key_path() {
        std::vector<std::string> keys;
        while (true) {
            if (cur_tok.type != token_type::identifier && cur_tok.type != token_type::string_literal) {
                throw std::runtime_error("Invalid key token at line " + std::to_string(cur_tok.line));
            }
            keys.push_back(cur_tok.text);
            advance();

            if (cur_tok.type == token_type::dot) {
                advance();
            }
            else {
                break;
            }
        }
        return keys;
    }

    toml_value parser::parse_value() {
        switch (cur_tok.type) {
        case token_type::string_literal: {
            std::string s = cur_tok.text;
            advance();
            return toml_value{ s };
        }
        case token_type::integer_literal: {
            int64_t v = cur_tok.int_val;
            advance();
            return toml_value{ v };
        }
        case token_type::float_literal: {
            double v = cur_tok.float_val;
            advance();
            return toml_value{ v };
        }
        case token_type::boolean_literal: {
            bool v = cur_tok.bool_val;
            advance();
            return toml_value{ v };
        }
        case token_type::datetime_literal: {
            date_time dt = cur_tok.dt_val;
            advance();
            return toml_value{ dt };
        }
        case token_type::lbracket:
            return parse_array();
        case token_type::lbrace:
            return parse_inline_table();
        default:
            throw std::runtime_error("Unexpected token in value at line " + std::to_string(cur_tok.line));
        }
    }

    toml_value parser::parse_array() {
        consume(token_type::lbracket, "Expected '['");
        toml_array arr;

        while (true) {
            skip_newlines();
            if (cur_tok.type == token_type::rbracket) break;

            arr.push_back(parse_value());
            skip_newlines();

            if (cur_tok.type == token_type::comma) {
                advance();
                skip_newlines();
            }
            else {
                break;
            }
        }
        consume(token_type::rbracket, "Expected ']'");
        return toml_value{ arr };
    }

    toml_value parser::parse_inline_table() {
        consume(token_type::lbrace, "Expected '{'");
        toml_table tbl;

        while (true) {
            if (cur_tok.type == token_type::rbrace) break;

            auto keys = parse_key_path();
            consume(token_type::equals, "Expected '='");
            toml_value val = parse_value();
            assign_value_at_path(tbl, keys, std::move(val));

            if (cur_tok.type == token_type::comma) {
                advance();
            }
            else {
                break;
            }
        }
        consume(token_type::rbrace, "Expected '}'");
        return toml_value{ tbl };
    }

    void parser::assign_value_at_path(toml_table& root, const std::vector<std::string>& path, toml_value val) {
        toml_table* cur = &root;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            const auto& key = path[i];
            if (!(*cur)[key].is<toml_table>()) {
                (*cur)[key] = toml_value{ toml_table{} };
            }
            cur = &(*cur)[key].as<toml_table>();
        }
        (*cur)[path.back()] = std::move(val);
    }

    toml_table& parser::navigate_to_table(toml_table& root, const std::vector<std::string>& path) {
        toml_table* cur = &root;
        for (const auto& key : path) {
            if (!(*cur)[key].is<toml_table>()) {
                (*cur)[key] = toml_value{ toml_table{} };
            }
            cur = &(*cur)[key].as<toml_table>();
        }
        return *cur;
    }

    toml_table& parser::append_array_table(toml_table& root, const std::vector<std::string>& path) {
        toml_table* cur = &root;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            const auto& key = path[i];
            if (!(*cur)[key].is<toml_table>()) {
                (*cur)[key] = toml_value{ toml_table{} };
            }
            cur = &(*cur)[key].as<toml_table>();
        }

        const auto& target_key = path.back();
        if (!(*cur)[target_key].is<toml_array>()) {
            (*cur)[target_key] = toml_value{ toml_array{} };
        }
        auto& arr = (*cur)[target_key].as<toml_array>();
        arr.push_back(toml_value{ toml_table{} });
        return arr.back().as<toml_table>();
    }

    // ===========================================================================
    // 직렬화 구현
    // ===========================================================================

    void serialize_date_time(std::ostream& os, const date_time& dt) {
        if (dt.has_date) {
            os << std::setfill('0') << std::setw(4) << dt.year << "-"
                << std::setw(2) << dt.month << "-"
                << std::setw(2) << dt.day;
        }
        if (dt.has_date && dt.has_time) os << "T";
        if (dt.has_time) {
            os << std::setfill('0') << std::setw(2) << dt.hour << ":"
                << std::setw(2) << dt.minute << ":"
                << std::setw(2) << dt.second;
        }
        if (dt.has_offset) {
            if (dt.offset_minutes == 0) {
                os << "Z";
            }
            else {
                int h = std::abs(dt.offset_minutes) / 60;
                int m = std::abs(dt.offset_minutes) % 60;
                os << (dt.offset_minutes >= 0 ? "+" : "-")
                    << std::setfill('0') << std::setw(2) << h << ":"
                    << std::setw(2) << m;
            }
        }
    }

    void serialize_string(std::ostream& os, const std::string& str) {
        os << '"';
        for (char c : str) {
            switch (c) {
            case '"': os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default: os << c; break;
            }
        }
        os << '"';
    }

    void serialize_value(std::ostream& os, const toml_value& val) {
        std::visit([&os](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                os << "\"\"";
            }
            else if constexpr (std::is_same_v<T, bool>) {
                os << (arg ? "true" : "false");
            }
            else if constexpr (std::is_same_v<T, int64_t>) {
                os << arg;
            }
            else if constexpr (std::is_same_v<T, double>) {
                if (std::isinf(arg)) os << (arg > 0 ? "inf" : "-inf");
                else if (std::isnan(arg)) os << "nan";
                else {
                    os << arg;
                    if (std::floor(arg) == arg) os << ".0";
                }
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                serialize_string(os, arg);
            }
            else if constexpr (std::is_same_v<T, date_time>) {
                serialize_date_time(os, arg);
            }
            else if constexpr (std::is_same_v<T, toml_array>) {
                os << "[";
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) os << ", ";
                    serialize_value(os, arg[i]);
                }
                os << "]";
            }
            else if constexpr (std::is_same_v<T, toml_table>) {
                os << "{";
                size_t idx = 0;
                for (const auto& [k, v] : arg) {
                    if (idx++ > 0) os << ", ";
                    os << k << " = ";
                    serialize_value(os, v);
                }
                os << "}";
            }
            }, val.data);
    }

    void dump_table_recursive(std::ostream& os, const toml_table& tbl, const std::string& prefix) {
        for (const auto& [key, val] : tbl) {
            if (!val.is<toml_table>() && !(val.is<toml_array>() && !val.as<toml_array>().empty() && val.as<toml_array>()[0].is<toml_table>())) {
                os << key << " = ";
                serialize_value(os, val);
                os << "\n";
            }
        }

        for (const auto& [key, val] : tbl) {
            if (val.is<toml_table>()) {
                std::string section = prefix.empty() ? key : prefix + "." + key;
                os << "\n[" << section << "]\n";
                dump_table_recursive(os, val.as<toml_table>(), section);
            }
        }

        for (const auto& [key, val] : tbl) {
            if (val.is<toml_array>() && !val.as<toml_array>().empty() && val.as<toml_array>()[0].is<toml_table>()) {
                std::string section = prefix.empty() ? key : prefix + "." + key;
                for (const auto& elem : val.as<toml_array>()) {
                    os << "\n[[" << section << "]]\n";
                    dump_table_recursive(os, elem.as<toml_table>(), section);
                }
            }
        }
    }

    // ===========================================================================
    // 공개 API 구현
    // ===========================================================================

    toml_table parse_toml(std::string_view content) {
        parser p(content);
        return p.parse();
    }

    toml_table load_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("Could not open file: " + path);
        std::stringstream ss;
        ss << file.rdbuf();
        return parse_toml(ss.str());
    }

    toml_table load_file(const std::filesystem::path& path) {
        return load_file(path.string());
    }

    std::string dump_toml(const toml_table& root) {
        std::ostringstream ss;
        dump_table_recursive(ss, root, "");
        return ss.str();
    }

    bool dump_file(const std::string& path, const toml_table& root) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << dump_toml(root);
        return true;
    }

    bool dump_file(const std::filesystem::path& path, const toml_table& root) {
        return dump_file(path.string(), root);
    }

} // namespace mino::core::toml
