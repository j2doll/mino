#include <fstream>
#include <sstream>
#include <charconv>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cctype>

#include "mino/core/ini/ini_parser.hpp"

namespace mino::core::ini {

    // 유틸
    std::string ini_parser::trim(std::string s)
    {
        auto is_space = [](char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        };
        std::size_t start = 0;
        while (start < s.size() && is_space(s[start])) { ++start; }
        if (start == s.size()) { return std::string(); }
        std::size_t end = s.size() - 1;
        while (end > start && is_space(s[end])) { --end; }
        return s.substr(start, end - start + 1);
    }

    // 변경: 인라인 주석을 잘라내고, 잘린 주석(구분자 포함)을 out_comment에 반환
    void ini_parser::strip_inline_comment(std::string& s, std::string& out_comment)
    {
        out_comment.clear();
        // 라인 전체가 주석인 경우는 호출자에서 처리됨.
        // 여기서는 첫 번째 유효한 ';' 또는 '#'을 찾아 그 뒤를 자른다.
        bool in_single = false;
        bool in_double = false;
        int last_non_space = -1;
        for (std::size_t i = 0; i < s.size(); ++i) {
            char c = s[i];

            if (c == '\'' && !in_double) {
                in_single = !in_single;
            } else if (c == '"' && !in_single) {
                in_double = !in_double;
            }

            if (!in_single && !in_double) {
                // check comment char first (so we don't treat the comment char itself as "last non-space")
                if (c == ';' || c == '#') {
                    // capture the comment including delimiter and any following space/text
                    out_comment = s.substr(i);
                    std::size_t cut = 0;
                    if (last_non_space >= 0) {
                        cut = static_cast<std::size_t>(last_non_space) + 1;
                    } else {
                        cut = 0;
                    }
                    // trim trailing spaces before comment
                    while (cut > 0 && std::isspace(static_cast<unsigned char>(s[cut - 1]))) {
                        --cut;
                    }
                    s.resize(cut);
                    return;
                }

                if (!std::isspace(static_cast<unsigned char>(c))) {
                    last_non_space = static_cast<int>(i);
                }
            }
        }
        // 전체가 주석이거나 주석 없음인 경우 그대로
    }

    std::string ini_parser::decode_escapes_for_string(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\\' && i + 1 < s.size()) {
                char nx = s[++i];
                switch (nx) {
                case 't': out.push_back('\t'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case '\\': out.push_back('\\'); break;
                case '\'': out.push_back('\''); break;
                case '"': out.push_back('"'); break;
                default:
                    // 알 수 없는 이스케이프는 그대로 보존(원래 문자 포함)
                    out.push_back('\\');
                    out.push_back(nx);
                    break;
                }
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    std::string ini_parser::encode_escapes_for_string(const std::string& s)
    {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            switch (c) {
            case '\t': out += "\\t"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            default: out.push_back(c); break;
            }
        }
        return out;
    }

    std::string ini_parser::unquote_and_unescape(const std::string& v, char quote_ch)
    {
        if (v.size() >= 2 && v.front() == quote_ch && v.back() == quote_ch) {
            std::string inner = v.substr(1, v.size() - 2);
            if (quote_ch == '\'') {
                // Raw: 이스케이프 시퀀스 해석
                return decode_escapes_for_string(inner);
            } else {
                // Literal: 큰따옴표 내부에서는 \" 만 해제
                std::string out;
                out.reserve(inner.size());
                for (std::size_t i = 0; i < inner.size(); ++i) {
                    if (inner[i] == '\\' && i + 1 < inner.size() && inner[i + 1] == '"') {
                        out.push_back('"');
                        ++i;
                    } else {
                        out.push_back(inner[i]);
                    }
                }
                return out;
            }
        }
        // 따옴표가 없으면 그대로 반환
        return v;
    }

    std::string ini_parser::quote_single_and_escape_raw(const std::string& v)
    {
        return std::string("'") + encode_escapes_for_string(v) + "'";
    }

    std::string ini_parser::quote_double_and_escape_literal(const std::string& v)
    {
        std::string out;
        out.reserve(v.size() + 8);
        for (char c : v) {
            if (c == '"') {
                out += "\\\"";
            } else {
                out.push_back(c);
            }
        }
        return std::string("\"") + out + "\"";
    }

    bool ini_parser::parse_bool(const std::string& v, bool& out)
    {
        std::string s = v;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
        if (s == "true") { out = true; return true; }
        if (s == "false") { out = false; return true; }
        return false;
    }

    bool ini_parser::parse_int(const std::string& v, int64_t& out)
    {
        if (v.empty()) { return false; }
        const char* str = v.c_str();
        char* end = nullptr;
        errno = 0;
        long long val = std::strtoll(str, &end, 0);
        if (end == str || *end != '\0' || errno == ERANGE) { return false; }
        out = static_cast<int64_t>(val);
        return true;
    }

    bool ini_parser::parse_double(const std::string& v, double& out)
    {
        if (v.empty()) { return false; }
        const char* str = v.c_str();
        char* end = nullptr;
        errno = 0;
        double val = std::strtod(str, &end);
        if (end == str || *end != '\0' || errno == ERANGE) { return false; }
        out = val;
        return true;
    }

    std::string ini_parser::format_double(double d)
    {
        std::ostringstream ss;
        ss.setf(std::ios::fmtflags(0), std::ios::floatfield);
        ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << d;
        return ss.str();
    }

    // 섹션/찾기/업서트
    section* ini_parser::ensure_section(const std::string& section_name)
    {
        auto it = sec_index_.find(section_name);
        if (it != sec_index_.end()) {
            return &sections_[it->second].second;
        }
        std::size_t idx = sections_.size();
        sections_.emplace_back(section_name, section{});
        sec_index_.emplace(section_name, idx);
        return &sections_.back().second;
    }

    const section* ini_parser::find_section(const std::string& section_name) const
    {
        auto it = sec_index_.find(section_name);
        if (it == sec_index_.end()) { return nullptr; }
        return &sections_[it->second].second;
    }

    section* ini_parser::find_section(const std::string& section_name)
    {
        auto it = sec_index_.find(section_name);
        if (it == sec_index_.end()) { return nullptr; }
        return &sections_[it->second].second;
    }

    void ini_parser::upsert_string(const std::string& section_name,
        const std::string& key,
        const std::string& value,
        bool literal_mode)
    {
        section* sec = ensure_section(section_name);
        auto it = sec->index.find(key);
        if (it != sec->index.end()) {
            entry& e = sec->entries[it->second];
            e.value = value;
            e.kind = value_kind::String;
            e.string_literal = literal_mode;
        } else {
            entry e;
            e.key = key;
            e.value = value;
            e.kind = value_kind::String;
            e.string_literal = literal_mode;
            sec->index.emplace(key, sec->entries.size());
            sec->entries.push_back(std::move(e));
        }
    }

    void ini_parser::upsert_numeric(const std::string& section_name,
        const std::string& key,
        const std::string& value,
        value_kind kind)
    {
        section* sec = ensure_section(section_name);
        auto it = sec->index.find(key);
        if (it != sec->index.end()) {
            entry& e = sec->entries[it->second];
            e.value = value;
            e.kind = kind;
            e.string_literal = false;
        } else {
            entry e;
            e.key = key;
            e.value = value;
            e.kind = kind;
            e.string_literal = false;
            sec->index.emplace(key, sec->entries.size());
            sec->entries.push_back(std::move(e));
        }
    }

    // 퍼블릭 API
    bool ini_parser::load(const std::string& path)
    {
        std::ifstream ifs(path);
        if (!ifs.is_open()) { return false; }

        sections_.clear();
        sec_index_.clear();

        std::string line;
        std::string current_section;

        std::vector<std::string> pending_comments; // accumulate whole-line comments until next section/header/entry

        while (std::getline(ifs, line)) {
            std::string raw = line;
            // 트리밍 한 뒤 빈 라인 처리
            std::string t = trim(raw);
            if (t.empty()) { continue; }

            // 전체 주석 라인: 보관(pending) 후 다음 유효 라인에 붙인다.
            if (t.front() == ';' || t.front() == '#') {
                // store trimmed comment (preserve delimiter at front)
                pending_comments.push_back(t);
                continue;
            }

            // 섹션
            if (t.front() == '[' && t.back() == ']') {
                std::string name = trim(t.substr(1, t.size() - 2));
                current_section = name;
                section* sec = ensure_section(current_section);
                // append pending comments to section leading comments
                if (!pending_comments.empty()) {
                    sec->leading_comments.insert(sec->leading_comments.end(),
                        pending_comments.begin(), pending_comments.end());
                    pending_comments.clear();
                }
                continue;
            }

            // 키=값 형태
            std::size_t pos = line.find('=');
            if (pos == std::string::npos) {
                // '=' 없는 라인은 무시
                continue;
            }

            std::string key = trim(line.substr(0, pos));
            std::string value = line.substr(pos + 1);
            std::string inline_comment;
            strip_inline_comment(value, inline_comment);
            value = trim(value);

            // prepare entry and attach pending_comments and inline_comment
            entry e;
            e.key = key;
            e.inline_comment = inline_comment;
            if (!pending_comments.empty()) {
                e.leading_comments = pending_comments;
                pending_comments.clear();
            }

            if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                // Raw string (single-quoted): 이스케이프 시퀀스 해석
                std::string v = unquote_and_unescape(value, '\'');
                e.value = v;
                e.kind = value_kind::String;
                e.string_literal = false;
            } else if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                // Literal string (double-quoted): \" 만 해제
                std::string v = unquote_and_unescape(value, '"');
                e.value = v;
                e.kind = value_kind::String;
                e.string_literal = true;
            } else {
                // 숫자/불리언 우선 파싱
                bool bval = false;
                if (parse_bool(value, bval)) {
                    e.value = bval ? "true" : "false";
                    e.kind = value_kind::Bool;
                    e.string_literal = false;
                } else {
                    int64_t ival = 0;
                    if (parse_int(value, ival)) {
                        e.value = std::to_string(ival);
                        e.kind = value_kind::Int;
                        e.string_literal = false;
                    } else {
                        double dval = 0.0;
                        if (parse_double(value, dval)) {
                            e.value = format_double(dval);
                            e.kind = value_kind::Double;
                            e.string_literal = false;
                        } else {
                            // Unquoted plain string: decode common escapes (t/n/r/\\/'/") and store as raw
                            std::string decoded = decode_escapes_for_string(value);
                            e.value = decoded;
                            e.kind = value_kind::String;
                            e.string_literal = false;
                        }
                    }
                }
            }

            // insert entry into current section (may be empty section name for global)
            section* sec = ensure_section(current_section);
            sec->index.emplace(e.key, sec->entries.size());
            sec->entries.push_back(std::move(e));
        }

        return true;
    }

    bool ini_parser::save(const std::string& path) const
    {
        std::ofstream ofs(path, std::ios::trunc);
        if (!ofs.is_open()) { return false; }

        for (std::size_t si = 0; si < sections_.size(); ++si) {
            const std::string& sec_name = sections_[si].first;
            const section& sec = sections_[si].second;

            // write leading comments for section
            for (const auto& c : sec.leading_comments) {
                ofs << c << '\n';
            }

            if (!sec_name.empty()) {
                ofs << "[" << sec_name << "]\n";
            }

            for (const entry& e : sec.entries) {
                // write leading comments for entry
                for (const auto& c : e.leading_comments) {
                    ofs << c << '\n';
                }

                ofs << e.key << '=';

                if (e.kind == value_kind::String) {
                    if (e.string_literal) {
                        ofs << quote_double_and_escape_literal(e.value);
                    } else {
                        ofs << quote_single_and_escape_raw(e.value);
                    }
                } else if (e.kind == value_kind::Bool) {
                    ofs << (e.value == "true" ? "true" : "false");
                } else if (e.kind == value_kind::Int) {
                    ofs << e.value;
                } else if (e.kind == value_kind::Double) {
                    ofs << e.value;
                }

                // inline comment (already includes delimiter)
                if (!e.inline_comment.empty()) {
                    ofs << ' ' << e.inline_comment;
                }

                ofs << '\n';
            }

            // 섹션 사이에 빈 줄 추가
            if (si + 1 < sections_.size()) { ofs << '\n'; }
        }

        return true;
    }

    bool ini_parser::has_section(const std::string& section_name) const
    {
        return sec_index_.find(section_name) != sec_index_.end();
    }

    bool ini_parser::has(const std::string& section_name, const std::string& key) const
    {
        const section* s = find_section(section_name);
        if (!s) { return false; }
        return s->index.find(key) != s->index.end();
    }

    std::optional<std::string> ini_parser::get_string(const std::string& section_name,
        const std::string& key) const
    {
        const section* s = find_section(section_name);
        if (!s) { return std::nullopt; }
        auto it = s->index.find(key);
        if (it == s->index.end()) { return std::nullopt; }
        const entry& e = s->entries[it->second];
        return e.value;
    }

    std::optional<bool> ini_parser::get_bool(const std::string& section_name,
        const std::string& key) const
    {
        const section* s = find_section(section_name);
        if (!s) { return std::nullopt; }
        auto it = s->index.find(key);
        if (it == s->index.end()) { return std::nullopt; }
        const entry& e = s->entries[it->second];
        if (e.kind == value_kind::Bool) {
            return e.value == "true";
        }
        bool out = false;
        if (parse_bool(e.value, out)) { return out; }
        return std::nullopt;
    }

    std::optional<int64_t> ini_parser::get_int(const std::string& section_name,
        const std::string& key) const
    {
        const section* s = find_section(section_name);
        if (!s) { return std::nullopt; }
        auto it = s->index.find(key);
        if (it == s->index.end()) { return std::nullopt; }
        const entry& e = s->entries[it->second];
        if (e.kind == value_kind::Int) {
            int64_t val = 0;
            if (parse_int(e.value, val)) { return val; }
            return std::nullopt;
        }
        int64_t val = 0;
        if (parse_int(e.value, val)) { return val; }
        return std::nullopt;
    }

    std::optional<double> ini_parser::get_double(const std::string& section_name,
        const std::string& key) const
    {
        const section* s = find_section(section_name);
        if (!s) { return std::nullopt; }
        auto it = s->index.find(key);
        if (it == s->index.end()) { return std::nullopt; }
        const entry& e = s->entries[it->second];
        if (e.kind == value_kind::Double) {
            double val = 0.0;
            if (parse_double(e.value, val)) { return val; }
            return std::nullopt;
        }
        double val = 0.0;
        if (parse_double(e.value, val)) { return val; }
        return std::nullopt;
    }

    void ini_parser::set_string(const std::string& section_name,
        const std::string& key,
        const std::string& value)
    {
        upsert_string(section_name, key, value, false);
    }

    void ini_parser::set_string_literal(const std::string& section_name,
        const std::string& key,
        const std::string& value)
    {
        upsert_string(section_name, key, value, true);
    }

    void ini_parser::set_bool(const std::string& section_name,
        const std::string& key,
        bool value)
    {
        upsert_numeric(section_name, key, value ? "true" : "false", value_kind::Bool);
    }

    void ini_parser::set_int(const std::string& section_name,
        const std::string& key,
        int64_t value)
    {
        upsert_numeric(section_name, key, std::to_string(value), value_kind::Int);
    }

    void ini_parser::set_double(const std::string& section_name,
        const std::string& key,
        double value)
    {
        upsert_numeric(section_name, key, format_double(value), value_kind::Double);
    }

    bool ini_parser::remove(const std::string& section_name, const std::string& key)
    {
        section* s = find_section(section_name);
        if (!s) { return false; }
        auto it = s->index.find(key);
        if (it == s->index.end()) { return false; }
        std::size_t idx = it->second;
        // entries 벡터에서 제거
        s->entries.erase(s->entries.begin() + static_cast<std::ptrdiff_t>(idx));
        s->index.erase(it);
        // 인덱스 재계산
        for (std::size_t i = 0; i < s->entries.size(); ++i) {
            s->index[s->entries[i].key] = i;
        }
        return true;
    }

    bool ini_parser::remove_section(const std::string& section_name)
    {
        auto it = sec_index_.find(section_name);
        if (it == sec_index_.end()) { return false; }
        std::size_t idx = it->second;
        sections_.erase(sections_.begin() + static_cast<std::ptrdiff_t>(idx));
        sec_index_.erase(it);
        // 섹션 인덱스 재계산
        for (std::size_t i = 0; i < sections_.size(); ++i) {
            sec_index_[sections_[i].first] = i;
        }
        return true;
    }

    std::vector<std::string> ini_parser::section_names() const
    {
        std::vector<std::string> out;
        out.reserve(sections_.size());
        for (const auto& p : sections_) { out.push_back(p.first); }
        return out;
    }

    std::vector<entry> ini_parser::entries(const std::string& section_name) const
    {
        const section* s = find_section(section_name);
        if (!s) { return {}; }
        return s->entries; // 복사 반환
    }

    std::vector<std::pair<std::string, std::vector<entry>>> ini_parser::all_sections() const
    {
        std::vector<std::pair<std::string, std::vector<entry>>> out;
        out.reserve(sections_.size());
        for (const auto& p : sections_) {
            out.emplace_back(p.first, p.second.entries); // 섹션 이름 + 엔트리 복사
        }
        return out;
    }

} // namespace mino::core::ini
