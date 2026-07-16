#include "mino/core/string/string_basic.hpp"

namespace { 
    // 내부 유틸: ASCII 대/소문자 변환 (유니코드는 별도 라이브러리 권장)
    static inline unsigned char sb_to_lower_ascii(unsigned char c) {
        if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A' + 'a');
        return c;
    }
    static inline unsigned char sb_to_upper_ascii(unsigned char c) {
        if (c >= 'a' && c <= 'z') return static_cast<unsigned char>(c - 'a' + 'A');
        return c;
    }

    // 내부 유틸: UTF-8 코드포인트 길이 계산 (간단 검증)
    static inline int sb_utf8_lead_len(unsigned char lead) {
        if ((lead & 0x80u) == 0x00u) return 1;
        if ((lead & 0xE0u) == 0xC0u) return 2;
        if ((lead & 0xF0u) == 0xE0u) return 3;
        if ((lead & 0xF8u) == 0xF0u) return 4;
        return -1; // invalid lead
    }
    static inline bool sb_utf8_is_cont(unsigned char c) {
        return (c & 0xC0u) == 0x80u;
    }
}

namespace mino::core::string
{
    // -------------------------
    // 트림 계열
    // -------------------------
    void ltrim(std::string& s)
    {
        auto it = std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
            });
        s.erase(s.begin(), it);
    }

    void rtrim(std::string& s)
    {
        auto it = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
            }).base();
        s.erase(it, s.end());
    }

    void trim(std::string& s)
    {
        ltrim(s);
        rtrim(s);
    }

    std::string ltrim_copy(std::string s)
    {
        ltrim(s);
        return s;
    }

    std::string rtrim_copy(std::string s)
    {
        rtrim(s);
        return s;
    }

    std::string trim_copy(std::string s)
    {
        trim(s);
        return s;
    }

    // -------------------------
    // 치환 계열
    // -------------------------
    std::string replace(const std::string& text,
        const std::string& find_token,
        const std::string& replace_token)
    {
        if (find_token.empty()) return text;

        std::string ret = text;
        std::size_t pos = 0;
        while ((pos = ret.find(find_token, pos)) != std::string::npos) {
            ret.replace(pos, find_token.size(), replace_token);
            pos += replace_token.size(); // 무한 루프 방지
        }
        return ret;
    }

    void replace_all_inplace(std::string& text,
        const std::string& find_token,
        const std::string& replace_token)
    {
        if (find_token.empty()) return;
        std::size_t pos = 0;
        while ((pos = text.find(find_token, pos)) != std::string::npos) {
            text.replace(pos, find_token.size(), replace_token);
            pos += replace_token.size();
        }
    }

    std::string replace_first(const std::string& text,
        const std::string& find_token,
        const std::string& replace_token)
    {
        if (find_token.empty()) return text;
        std::string ret = text;
        auto pos = ret.find(find_token);
        if (pos != std::string::npos) {
            ret.replace(pos, find_token.size(), replace_token);
        }
        return ret;
    }

    std::string replace_last(const std::string& text,
        const std::string& find_token,
        const std::string& replace_token)
    {
        if (find_token.empty()) return text;
        std::string ret = text;
        auto pos = ret.rfind(find_token);
        if (pos != std::string::npos) {
            ret.replace(pos, find_token.size(), replace_token);
        }
        return ret;
    }

    std::string replace_all_map(
        const std::string& text,
        const std::vector<std::pair<std::string, std::string>>& replacements)
    {
        std::string out = text;
        for (const auto& kv : replacements) {
            replace_all_inplace(out, kv.first, kv.second);
        }
        return out;
    }

    // -------------------------
    // 대소문자 변환 / 포함 / 접두/접미
    // -------------------------
    std::string to_lower(const std::string& s)
    {
        std::string ret = s;
        std::transform(ret.begin(), ret.end(), ret.begin(),
            [](unsigned char c) { return sb_to_lower_ascii(c); });
        return ret;
    }

    std::string to_upper(const std::string& s)
    {
        std::string ret = s;
        std::transform(ret.begin(), ret.end(), ret.begin(),
            [](unsigned char c) { return sb_to_upper_ascii(c); });
        return ret;
    }

    bool contains(const std::string& s, const std::string& token)
    {
        if (token.empty()) return true;
        return s.find(token) != std::string::npos;
    }

    bool starts_with(const std::string& s, const std::string& prefix)
    {
        if (s.size() < prefix.size()) return false;
        return std::equal(prefix.begin(), prefix.end(), s.begin());
    }

    bool ends_with(const std::string& s, const std::string& suffix)
    {
        if (s.size() < suffix.size()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
    }

    bool iequals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (sb_to_lower_ascii(static_cast<unsigned char>(a[i])) !=
                sb_to_lower_ascii(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    }

    bool icontains(const std::string& s, const std::string& token)
    {
        if (token.empty()) return true;
        std::string ls = to_lower(s);
        std::string lt = to_lower(token);
        return ls.find(lt) != std::string::npos;
    }

    bool istarts_with(const std::string& s, const std::string& prefix)
    {
        if (s.size() < prefix.size()) return false;
        for (std::size_t i = 0; i < prefix.size(); ++i) {
            if (sb_to_lower_ascii(static_cast<unsigned char>(s[i])) !=
                sb_to_lower_ascii(static_cast<unsigned char>(prefix[i])))
                return false;
        }
        return true;
    }

    bool iends_with(const std::string& s, const std::string& suffix)
    {
        if (s.size() < suffix.size()) return false;
        auto si = s.size();
        auto ti = suffix.size();
        for (std::size_t k = 0; k < ti; ++k) {
            if (sb_to_lower_ascii(static_cast<unsigned char>(s[si - 1 - k])) !=
                sb_to_lower_ascii(static_cast<unsigned char>(suffix[ti - 1 - k])))
                return false;
        }
        return true;
    }

    bool starts_with_any(const std::string& s, const std::vector<std::string>& prefixes)
    {
        for (const auto& p : prefixes) if (starts_with(s, p)) return true;
        return false;
    }

    bool ends_with_any(const std::string& s, const std::vector<std::string>& suffixes)
    {
        for (const auto& p : suffixes) if (ends_with(s, p)) return true;
        return false;
    }

    std::string common_prefix(const std::vector<std::string>& items)
    {
        if (items.empty()) return {};
        std::string pref = items.front();
        for (std::size_t i = 1; i < items.size(); ++i) {
            const auto& s = items[i];
            std::size_t k = 0;
            while (k < pref.size() && k < s.size() && pref[k] == s[k]) ++k;
            pref.resize(k);
            if (pref.empty()) break;
        }
        return pref;
    }

    std::string common_suffix(const std::vector<std::string>& items)
    {
        if (items.empty()) return {};
        std::string suff = items.front();
        for (std::size_t i = 1; i < items.size(); ++i) {
            const auto& s = items[i];
            std::size_t k = 0;
            while (k < suff.size() && k < s.size() &&
                suff[suff.size() - 1 - k] == s[s.size() - 1 - k]) ++k;
            suff.erase(0, suff.size() - k);
            if (suff.empty()) break;
        }
        return suff;
    }

    // -------------------------
    // 분할/결합
    // -------------------------
    std::vector<std::string> split(const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            tokens.push_back(item);
        }
        return tokens;
    }

    std::vector<std::string> split(const std::string& s,
        const std::string& delimiter,
        bool keep_empty)
    {
        std::vector<std::string> parts;
        if (delimiter.empty()) {
            parts.reserve(s.size());
            for (char c : s) parts.emplace_back(1, c);
            return parts;
        }

        std::size_t pos = 0, idx;
        while ((idx = s.find(delimiter, pos)) != std::string::npos) {
            if (keep_empty || idx > pos) {
                parts.emplace_back(s.substr(pos, idx - pos));
            }
            else if (keep_empty) {
                parts.emplace_back(std::string{});
            }
            pos = idx + delimiter.size();
        }
        if (keep_empty || pos < s.size()) {
            parts.emplace_back(s.substr(pos));
        }
        else if (keep_empty && s.empty()) {
            parts.emplace_back(std::string{});
        }
        return parts;
    }

    std::vector<std::string> tokenize_any_of(const std::string& s,
        const std::string& delims,
        bool keep_empty)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s) {
            if (!delims.empty() && delims.find(c) != std::string::npos) {
                if (keep_empty || !cur.empty()) out.push_back(cur);
                cur.clear();
            }
            else {
                cur.push_back(c);
            }
        }
        if (keep_empty || !cur.empty()) out.push_back(cur);
        return out;
    }

    std::vector<std::string> split_trimmed(const std::string& s,
        char delimiter,
        bool drop_empty)
    {
        auto parts = split(s, delimiter);
        std::vector<std::string> out;
        out.reserve(parts.size());
        for (auto& t : parts) {
            auto x = trim_copy(t);
            if (!drop_empty || !x.empty()) out.push_back(std::move(x));
        }
        return out;
    }

    std::vector<std::string> split_lines(const std::string& s, bool keep_empty)
    {
        std::vector<std::string> lines;
        std::string cur;
        cur.reserve(64);
        for (std::size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\r') {
                if (i + 1 < s.size() && s[i + 1] == '\n') ++i; // \r\n
                if (keep_empty || !cur.empty()) lines.push_back(cur);
                cur.clear();
            }
            else if (c == '\n') {
                if (keep_empty || !cur.empty()) lines.push_back(cur);
                cur.clear();
            }
            else {
                cur.push_back(c);
            }
        }
        if (keep_empty || !cur.empty()) lines.push_back(cur);
        return lines;
    }

    std::string join(const std::vector<std::string>& tokens, const std::string& delimiter)
    {
        if (tokens.empty()) return "";
        std::ostringstream oss;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) oss << delimiter;
            oss << tokens[i];
        }
        return oss.str();
    }

    // -------------------------
    // 공백/개행 정규화
    // -------------------------
    bool is_blank(const std::string& s)
    {
        return std::all_of(s.begin(), s.end(), [](unsigned char ch) {
            return std::isspace(ch);
            });
    }

    std::string collapse_spaces(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        bool in_space = false;

        for (unsigned char ch : s) {
            if (std::isspace(ch)) {
                if (!in_space) {
                    out.push_back(' ');
                    in_space = true;
                }
            }
            else {
                out.push_back(static_cast<char>(ch));
                in_space = false;
            }
        }
        return trim_copy(out);
    }

    std::string normalize_newlines(const std::string& s, const std::string& nl)
    {
        std::string tmp;
        tmp.reserve(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\r') {
                if (i + 1 < s.size() && s[i + 1] == '\n') {
                    tmp += nl; // \r\n → nl
                    ++i;
                }
                else {
                    tmp += nl; // \r → nl
                }
            }
            else if (c == '\n') {
                tmp += nl; // \n → nl
            }
            else {
                tmp.push_back(c);
            }
        }
        return tmp;
    }

    // -------------------------
    // 패딩/반복/삭제/감싸기
    // -------------------------
    std::string pad_left(const std::string& s, std::size_t width, char fill)
    {
        if (s.size() >= width) return s;
        return std::string(width - s.size(), fill) + s;
    }

    std::string pad_right(const std::string& s, std::size_t width, char fill)
    {
        if (s.size() >= width) return s;
        return s + std::string(width - s.size(), fill);
    }

    std::string pad_center(const std::string& s, std::size_t width, char fill)
    {
        if (s.size() >= width) return s;
        std::size_t total = width - s.size();
        std::size_t left = total / 2;
        std::size_t right = total - left;
        return std::string(left, fill) + s + std::string(right, fill);
    }

    std::string remove_chars(const std::string& s, const std::string& chars)
    {
        if (chars.empty()) return s;
        std::string ret;
        ret.reserve(s.size());
        for (char c : s) {
            if (chars.find(c) == std::string::npos) ret.push_back(c);
        }
        return ret;
    }

    std::string repeat(const std::string& s, std::size_t count)
    {
        if (count == 0 || s.empty()) return std::string();
        std::string ret;
        ret.reserve(s.size() * count);
        for (std::size_t i = 0; i < count; ++i) ret += s;
        return ret;
    }

    std::string strip_quotes(const std::string& s, char quote)
    {
        if (s.size() >= 2 && s.front() == quote && s.back() == quote) {
            return s.substr(1, s.size() - 2);
        }
        return s;
    }

    std::string quote(const std::string& s, char q, bool escape)
    {
        if (!escape) return std::string(1, q) + s + std::string(1, q);
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back(q);
        for (char c : s) {
            if (c == q || c == '\\') out.push_back('\\');
            out.push_back(c);
        }
        out.push_back(q);
        return out;
    }

    std::string surround_if_missing(const std::string& s,
        const std::string& prefix,
        const std::string& suffix)
    {
        std::string out = s;
        if (!starts_with(out, prefix)) out = prefix + out;
        if (!ends_with(out, suffix))   out += suffix;
        return out;
    }

    std::string indent_lines(const std::string& s, const std::string& prefix)
    {
        if (s.empty()) return prefix;
        std::string out;
        out.reserve(s.size() + prefix.size() * 8);
        out += prefix;
        for (std::size_t i = 0; i < s.size(); ++i) {
            out.push_back(s[i]);
            if (s[i] == '\n') out += prefix;
        }
        return out;
    }

    // -------------------------
    // 접두/접미 제거
    // -------------------------
    bool remove_prefix(std::string& s, const std::string& prefix)
    {
        if (starts_with(s, prefix)) {
            s.erase(0, prefix.size());
            return true;
        }
        return false;
    }

    bool remove_suffix(std::string& s, const std::string& suffix)
    {
        if (ends_with(s, suffix)) {
            s.erase(s.size() - suffix.size());
            return true;
        }
        return false;
    }

    std::string removed_prefix(const std::string& s, const std::string& prefix)
    {
        std::string t = s;
        remove_prefix(t, prefix);
        return t;
    }

    std::string removed_suffix(const std::string& s, const std::string& suffix)
    {
        std::string t = s;
        remove_suffix(t, suffix);
        return t;
    }

    // -------------------------
    // 안전 부분문자열/말줄임
    // -------------------------
    std::string safe_substr(const std::string& s, std::size_t pos, std::size_t count)
    {
        if (pos >= s.size()) return std::string();
        std::size_t max_count = s.size() - pos;
        if (count > max_count) count = max_count;
        return s.substr(pos, count);
    }

    std::string ellipsize(const std::string& s, std::size_t max_len, const std::string& ellipsis)
    {
        if (s.size() <= max_len) {
            return s;
        }
        if (ellipsis.size() >= max_len) {
            return ellipsis.substr(0, max_len);
        }
        return s.substr(0, max_len - ellipsis.size()) + ellipsis;
    }

    std::string ellipsize_utf8_safe(const std::string& s, std::size_t max_codepoints,
        const std::string& ellipsis)
    {
        if (max_codepoints == 0) {
            return std::string();
        }

        // 코드포인트 개수를 세며 경계에서 자르기
        std::size_t i = 0, cp_count = 0, last_boundary = 0;
        while (i < s.size()) {
            int len = sb_utf8_lead_len(static_cast<unsigned char>(s[i]));
            if (len < 0 || i + len > s.size()) {
                // 잘못된 바이트가 나오면 여기서 중단(안전하게 현재까지)
                break;
            }
            // 연속 바이트 검증
            for (int k = 1; k < len; ++k) {
                if (!sb_utf8_is_cont(static_cast<unsigned char>(s[i + k]))) {
                    len = 1; // 불량 → 단일 바이트로 취급하고 중단
                    break;
                }
            }
            ++cp_count;
            i += len;
            if (cp_count <= max_codepoints) last_boundary = i;
            if (cp_count > max_codepoints) break;
        }
        if (cp_count <= max_codepoints) return s;
        if (ellipsis.empty()) return s.substr(0, last_boundary);
        return s.substr(0, last_boundary) + ellipsis;
    }

    // -------------------------
    // 숫자 파싱/검증 (ASCII)
    // -------------------------
    bool is_digits(const std::string& s)
    {
        if (s.empty()) return false;
        for (unsigned char c : s) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    bool try_parse_int64(const std::string& s, std::int64_t& out, int base)
    {
        // 공백이나 접두/접미 공백 불허. sign, base 처리만 허용.
        const char* begin = s.data();
        const char* end = s.data() + s.size();
        auto res = std::from_chars(begin, end, out, base);
        return (res.ec == std::errc{} && res.ptr == end);
    }

    bool try_parse_double(const std::string& s, double& out)
    {
        // C++17에서 부동소수 from_chars는 구현체별 지원이 제한적일 수 있으므로
        // 안전하게 std::strtod 사용. 공백/잔여 문자를 엄격히 체크.
        char* p = nullptr;
        errno = 0;
        const double v = std::strtod(s.c_str(), &p);
        if (p != s.c_str() && *p == '\0' && errno == 0) {
            out = v;
            return true;
        }
        return false;
    }

    // -------------------------
    // 와일드카드 매칭 (*, ?) - ASCII
    // -------------------------
    static bool wildcard_impl(const char* text, const char* pat)
    {
        // 비백트래킹 구현(간단한 그리디 + 앵커 저장)
        const char* star = nullptr;
        const char* star_text = nullptr;

        while (*text) {
            if (*pat == '?' || *pat == *text) {
                ++text; ++pat;
            }
            else if (*pat == '*') {
                star = pat++;
                star_text = text;
            }
            else if (star) {
                pat = star + 1;
                text = ++star_text;
            }
            else {
                return false;
            }
        }
        while (*pat == '*') ++pat;
        return *pat == '\0';
    }

    bool wildcard_match(const std::string& text, const std::string& pattern)
    {
        return wildcard_impl(text.c_str(), pattern.c_str());
    }

    /**
     * @brief 숫자에 천 단위(3) 또는 만 단위(4) 구분자를 추가합니다.
     * @param str_num 숫자 문자열
     * @param step 구분 단위 (기본 3)
     * @param separator 구분자 문자 (기본 ',')
     */
    std::string add_separator(const std::string& str_num, int step, char separator) {
        if (str_num.empty() || step <= 0) return str_num;

        bool is_negative = (!str_num.empty() && str_num[0] == '-');
        size_t start_idx = is_negative ? 1 : 0;
        size_t dot_pos = str_num.find('.');

        std::string integer_part = str_num.substr(start_idx, (dot_pos == std::string::npos) ? std::string::npos : dot_pos - start_idx);
        std::string fractional_part = (dot_pos == std::string::npos) ? "" : str_num.substr(dot_pos);

        std::string result = "";
        int count = 0;
        for (int i = (int)integer_part.length() - 1; i >= 0; --i) {
            if (count > 0 && count % step == 0) result += separator;
            result += integer_part[i];
            count++;
        }

        std::reverse(result.begin(), result.end());
        return (is_negative ? "-" : "") + result + fractional_part;
    }

    /**
     * @brief 숫자를 한국인이 읽기 쉬운 거대 단위(만~대수)로 변환합니다.
     * @param str_num 숫자 문자열
     * @param include_comma 각 단위 내부(만 단위)에 콤마를 넣을지 여부
     */
    std::string to_human_readable_korean(const std::string& str_num, bool include_comma) {
        if (str_num.empty() || str_num == "0" || str_num == "0.0") return "0";

        bool is_negative = (!str_num.empty() && str_num[0] == '-');
        size_t start_idx = is_negative ? 1 : 0;
        size_t dot_pos = str_num.find('.');

        std::string integer_part = str_num.substr(start_idx, (dot_pos == std::string::npos) ? std::string::npos : dot_pos - start_idx);
        std::string fractional_part = (dot_pos == std::string::npos) ? "" : str_num.substr(dot_pos);

        static const char* units[] = {
            "", "만", "억", "조", "경", "해", "자", "양", "구", "간", "정", "재", "극",
            "항하사", "아승기", "나유타", "불가사의", "무량대수", "대수"
        };
        const int max_unit_idx = (sizeof(units) / sizeof(units[0])) - 1;

        std::vector<std::string> parts;
        int len = (int)integer_part.length();
        int unit_idx = 0;

        for (int i = len; i > 0; i -= 4) {
            int start = std::max(0, i - 4);
            std::string section = integer_part.substr(start, i - start);
            long long section_val = std::stoll(section);

            if (section_val > 0) {
                std::string section_str = std::to_string(section_val);
                if (include_comma && section_val >= 1000) {
                    section_str.insert(section_str.length() - 3, ",");
                }
                if (unit_idx <= max_unit_idx) {
                    parts.push_back(section_str + units[unit_idx]);
                }
            }
            unit_idx++;
            if (unit_idx > max_unit_idx) break;
        }

        if (parts.empty()) return "0" + fractional_part;

        std::reverse(parts.begin(), parts.end());
        std::string result = "";
        for (size_t i = 0; i < parts.size(); ++i) {
            result += parts[i] + (i == parts.size() - 1 ? "" : " ");
        }

        return (is_negative ? "-" : "") + result + fractional_part;
    }


} // namespace mino::core::string
