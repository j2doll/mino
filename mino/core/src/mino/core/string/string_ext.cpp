#include "mino/core/string/string_ext.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

namespace mino::core::string {

    namespace {
        // ---------------------------------------------------------------------
        // UTF-8 디코더 / 인코더 헬퍼
        // ---------------------------------------------------------------------
        bool next_cp(std::string_view s, std::size_t& i, char32_t& cp) {
            if (i >= s.size()) return false;
            unsigned char b0 = static_cast<unsigned char>(s[i]);
            if ((b0 & 0x80u) == 0) {
                cp = b0;
                i += 1;
                return true;
            }
            else if ((b0 & 0xE0u) == 0xC0u) {
                if (i + 1 >= s.size()) { cp = 0xFFFD; i += 1; return true; }
                unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
                cp = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
                i += 2;
                return true;
            }
            else if ((b0 & 0xF0u) == 0xE0u) {
                if (i + 2 >= s.size()) { cp = 0xFFFD; i += 1; return true; }
                unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
                unsigned char b2 = static_cast<unsigned char>(s[i + 2]);
                cp = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
                i += 3;
                return true;
            }
            else if ((b0 & 0xF8u) == 0xF0u) {
                if (i + 3 >= s.size()) { cp = 0xFFFD; i += 1; return true; }
                unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
                unsigned char b2 = static_cast<unsigned char>(s[i + 2]);
                unsigned char b3 = static_cast<unsigned char>(s[i + 3]);
                cp = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
                i += 4;
                return true;
            }
            cp = 0xFFFD;
            i += 1;
            return true;
        }

        void append_cp(std::string& out, char32_t cp) {
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            }
            else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0xFFFF) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0x10FFFF) {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }

        std::vector<char32_t> to_codepoints(std::string_view sv) {
            std::vector<char32_t> cps;
            std::size_t i = 0;
            char32_t cp = 0;
            while (next_cp(sv, i, cp)) cps.push_back(cp);
            return cps;
        }

        // 한글 유니코드 기본 상수
        constexpr char32_t HANGUL_BASE = 0xAC00;
        constexpr char32_t HANGUL_END = 0xD7A3;

        // 초성 호환 자모 표 (19자)
        const char32_t CHOSUNG_COMPAT[19] = {
            U'ㄱ', U'ㄲ', U'ㄴ', U'ㄷ', U'ㄸ', U'ㄹ', U'ㅁ', U'ㅂ', U'ㅃ',
            U'ㅅ', U'ㅆ', U'ㅇ', U'ㅈ', U'ㅉ', U'ㅊ', U'ㅋ', U'ㅌ', U'ㅍ', U'ㅎ'
        };

        // 중성 호환 자모 표 (21자)
        const char32_t JUNGSUNG_COMPAT[21] = {
            U'ㅏ', U'ㅐ', U'ㅑ', U'ㅒ', U'ㅓ', U'ㅔ', U'ㅕ', U'ㅖ', U'ㅗ', U'ㅘ',
            U'ㅙ', U'ㅚ', U'ㅛ', U'ㅜ', U'ㅝ', U'ㅞ', U'ㅟ', U'ㅠ', U'ㅡ', U'ㅢ', U'ㅣ'
        };

        // 종성 호환 자모 표 (28자, 0은 종성 없음)
        const char32_t JONGSUNG_COMPAT[28] = {
            0,     U'ㄱ', U'ㄲ', U'ㄳ', U'ㄴ', U'ㄵ', U'ㄶ', U'ㄷ', U'ㄹ', U'ㄺ',
            U'ㄻ', U'ㄼ', U'ㄽ', U'ㄾ', U'ㄿ', U'ㅀ', U'ㅁ', U'ㅂ', U'ㅄ', U'ㅅ',
            U'ㅆ', U'ㅇ', U'ㅈ', U'ㅊ', U'ㅋ', U'ㅌ', U'ㅍ', U'ㅎ'
        };

        int get_chosung_index(char32_t cp) {
            for (int i = 0; i < 19; ++i) {
                if (CHOSUNG_COMPAT[i] == cp || (0x1100 + i) == cp) return i;
            }
            return -1;
        }
        int get_jungsung_index(char32_t cp) {
            for (int i = 0; i < 21; ++i) {
                if (JUNGSUNG_COMPAT[i] == cp || (0x1161 + i) == cp) return i;
            }
            return -1;
        }
        int get_jongsung_index(char32_t cp) {
            for (int i = 1; i < 28; ++i) {
                if (JONGSUNG_COMPAT[i] == cp || (0x11A7 + i) == cp) return i;
            }
            return 0;
        }
    } // namespace

    // =========================================================================
    // 1. 한국어 특화 기능 구현
    // =========================================================================

    std::string extract_chosung(std::string_view utf8_text) {
        std::string result;
        std::size_t i = 0;
        char32_t cp = 0;
        while (next_cp(utf8_text, i, cp)) {
            if (cp >= HANGUL_BASE && cp <= HANGUL_END) {
                char32_t chosung = CHOSUNG_COMPAT[(cp - HANGUL_BASE) / 588];
                append_cp(result, chosung);
            }
            else {
                append_cp(result, cp);
            }
        }
        return result;
    }

    bool matches_chosung(std::string_view target, std::string_view chosung_pattern) {
        auto t_cps = to_codepoints(target);
        auto p_cps = to_codepoints(chosung_pattern);
        if (p_cps.empty()) return true;
        if (t_cps.size() < p_cps.size()) return false;

        for (std::size_t start = 0; start <= t_cps.size() - p_cps.size(); ++start) {
            bool matched = true;
            for (std::size_t k = 0; k < p_cps.size(); ++k) {
                char32_t t = t_cps[start + k];
                char32_t p = p_cps[k];

                if (t >= HANGUL_BASE && t <= HANGUL_END) {
                    char32_t t_cho = CHOSUNG_COMPAT[(t - HANGUL_BASE) / 588];
                    if (p != t && p != t_cho) {
                        matched = false;
                        break;
                    }
                }
                else {
                    if (t != p) {
                        matched = false;
                        break;
                    }
                }
            }
            if (matched) return true;
        }
        return false;
    }

    std::string decompose_hangul(std::string_view utf8_text, bool use_compat_jamo) {
        std::string result;
        std::size_t i = 0;
        char32_t cp = 0;
        while (next_cp(utf8_text, i, cp)) {
            if (cp >= HANGUL_BASE && cp <= HANGUL_END) {
                int s_idx = cp - HANGUL_BASE;
                int cho = s_idx / 588;
                int jung = (s_idx % 588) / 28;
                int jong = s_idx % 28;

                if (use_compat_jamo) {
                    append_cp(result, CHOSUNG_COMPAT[cho]);
                    append_cp(result, JUNGSUNG_COMPAT[jung]);
                    if (jong > 0) append_cp(result, JONGSUNG_COMPAT[jong]);
                }
                else {
                    append_cp(result, 0x1100 + cho);
                    append_cp(result, 0x1161 + jung);
                    if (jong > 0) append_cp(result, 0x11A7 + jong);
                }
            }
            else {
                append_cp(result, cp);
            }
        }
        return result;
    }

    std::string compose_hangul(std::string_view utf8_text) {
        auto cps = to_codepoints(utf8_text);
        std::string result;
        std::size_t n = cps.size();
        std::size_t i = 0;

        while (i < n) {
            int cho = get_chosung_index(cps[i]);
            if (cho != -1 && (i + 1 < n)) {
                int jung = get_jungsung_index(cps[i + 1]);
                if (jung != -1) {
                    int jong = 0;
                    std::size_t step = 2;
                    if (i + 2 < n) {
                        int candidate_jong = get_jongsung_index(cps[i + 2]);
                        if (candidate_jong > 0) {
                            if (i + 3 < n && get_jungsung_index(cps[i + 3]) != -1) {
                                jong = 0;
                            }
                            else {
                                jong = candidate_jong;
                                step = 3;
                            }
                        }
                    }
                    char32_t composed = HANGUL_BASE + (cho * 588) + (jung * 28) + jong;
                    append_cp(result, composed);
                    i += step;
                    continue;
                }
            }
            append_cp(result, cps[i]);
            i += 1;
        }
        return result;
    }

    // =========================================================================
    // 2. 유니코드 및 전각/반각 변환 구현
    // =========================================================================

    std::string to_full_width(std::string_view utf8_text) {
        std::string result;
        std::size_t i = 0;
        char32_t cp = 0;
        while (next_cp(utf8_text, i, cp)) {
            if (cp == 0x20) {
                append_cp(result, 0x3000); // 반각 공백 -> 전각 공백
            }
            else if (cp >= 0x21 && cp <= 0x7E) {
                append_cp(result, cp + 0xFEE0);
            }
            else {
                append_cp(result, cp);
            }
        }
        return result;
    }

    std::string to_half_width(std::string_view utf8_text) {
        std::string result;
        std::size_t i = 0;
        char32_t cp = 0;
        while (next_cp(utf8_text, i, cp)) {
            if (cp == 0x3000) {
                append_cp(result, 0x20); // 전각 공백 -> 반각 공백
            }
            else if (cp >= 0xFF01 && cp <= 0xFF5E) {
                append_cp(result, cp - 0xFEE0);
            }
            else {
                append_cp(result, cp);
            }
        }
        return result;
    }

    std::size_t utf8_grapheme_count(std::string_view utf8_text) {
        std::size_t count = 0;
        std::size_t i = 0;
        char32_t cp = 0;
        while (next_cp(utf8_text, i, cp)) {
            if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1160 && cp <= 0x11FF)) {
                continue; // 결합 자모 및 다이아크리틱 결합 기호는 독립 글자로 계산 제외
            }
            ++count;
        }
        return count;
    }

    std::string ellipsize_grapheme_safe(std::string_view utf8_text, std::size_t max_graphemes, std::string_view ellipsis) {
        if (max_graphemes == 0) return std::string(ellipsis);

        std::size_t count = 0;
        std::size_t i = 0;
        std::size_t last_valid_byte = 0;
        char32_t cp = 0;

        while (i < utf8_text.size()) {
            std::size_t prev_i = i;
            if (!next_cp(utf8_text, i, cp)) break;

            if (!((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1160 && cp <= 0x11FF))) {
                if (count == max_graphemes) {
                    return std::string(utf8_text.substr(0, last_valid_byte)) + std::string(ellipsis);
                }
                ++count;
            }
            last_valid_byte = i;
        }
        return std::string(utf8_text);
    }

    // =========================================================================
    // 3. 네이밍 스타일 및 텍스트 변환 구현
    // =========================================================================

    namespace {
        std::vector<std::string> extract_words(std::string_view s) {
            std::vector<std::string> words;
            std::string cur;
            for (std::size_t i = 0; i < s.size(); ++i) {
                char c = s[i];
                if (c == '_' || c == '-' || std::isspace(static_cast<unsigned char>(c))) {
                    if (!cur.empty()) { words.push_back(cur); cur.clear(); }
                }
                else if (std::isupper(static_cast<unsigned char>(c))) {
                    if (!cur.empty() && (i + 1 < s.size()) && std::islower(static_cast<unsigned char>(s[i + 1])) && !std::isupper(static_cast<unsigned char>(cur.back()))) {
                        words.push_back(cur);
                        cur.clear();
                    }
                    cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                }
                else {
                    cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                }
            }
            if (!cur.empty()) words.push_back(cur);
            return words;
        }
    }

    std::string to_snake_case(std::string_view s) {
        auto words = extract_words(s);
        std::string res;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i > 0) res += '_';
            res += words[i];
        }
        return res;
    }

    std::string to_camel_case(std::string_view s) {
        auto words = extract_words(s);
        std::string res;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i == 0) {
                res += words[i];
            }
            else if (!words[i].empty()) {
                words[i][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(words[i][0])));
                res += words[i];
            }
        }
        return res;
    }

    std::string to_pascal_case(std::string_view s) {
        auto words = extract_words(s);
        std::string res;
        for (auto& w : words) {
            if (!w.empty()) {
                w[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(w[0])));
                res += w;
            }
        }
        return res;
    }

    std::string to_kebab_case(std::string_view s) {
        auto words = extract_words(s);
        std::string res;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i > 0) res += '-';
            res += words[i];
        }
        return res;
    }

    std::string to_slug(std::string_view s, char separator) {
        std::string result;
        std::size_t i = 0;
        char32_t cp = 0;
        bool last_was_sep = true;

        while (next_cp(s, i, cp)) {
            bool is_alphanumeric = (cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9');
            bool is_upper = (cp >= 'A' && cp <= 'Z');
            bool is_hangul = (cp >= HANGUL_BASE && cp <= HANGUL_END) || (cp >= 0x3131 && cp <= 0x318E);

            if (is_upper) {
                result.push_back(static_cast<char>(std::tolower(static_cast<char>(cp))));
                last_was_sep = false;
            }
            else if (is_alphanumeric) {
                result.push_back(static_cast<char>(cp));
                last_was_sep = false;
            }
            else if (is_hangul) {
                append_cp(result, cp);
                last_was_sep = false;
            }
            else {
                if (!last_was_sep) {
                    result.push_back(separator);
                    last_was_sep = true;
                }
            }
        }
        while (!result.empty() && result.back() == separator) {
            result.pop_back();
        }
        return result;
    }

    // =========================================================================
    // 4. 문자열 유사도 및 퍼지 검색 구현
    // =========================================================================

    std::size_t levenshtein_distance(std::string_view a, std::string_view b) {
        auto ca = to_codepoints(a);
        auto cb = to_codepoints(b);

        const std::size_t m = ca.size();
        const std::size_t n = cb.size();
        if (m == 0) return n;
        if (n == 0) return m;

        std::vector<std::size_t> dp(n + 1);
        for (std::size_t j = 0; j <= n; ++j) dp[j] = j;

        for (std::size_t i = 1; i <= m; ++i) {
            std::size_t prev = dp[0];
            dp[0] = i;
            for (std::size_t j = 1; j <= n; ++j) {
                std::size_t temp = dp[j];
                if (ca[i - 1] == cb[j - 1]) {
                    dp[j] = prev;
                }
                else {
                    dp[j] = 1 + std::min({ prev, dp[j], dp[j - 1] });
                }
                prev = temp;
            }
        }
        return dp[n];
    }

    double similarity_ratio(std::string_view a, std::string_view b) {
        std::size_t len_a = to_codepoints(a).size();
        std::size_t len_b = to_codepoints(b).size();
        std::size_t max_len = std::max(len_a, len_b);
        if (max_len == 0) return 1.0;

        std::size_t dist = levenshtein_distance(a, b);
        return 1.0 - (static_cast<double>(dist) / static_cast<double>(max_len));
    }

    bool fuzzy_match(std::string_view pattern, std::string_view target, bool ignore_case) {
        auto p_cps = to_codepoints(pattern);
        auto t_cps = to_codepoints(target);

        std::size_t p_idx = 0;
        std::size_t t_idx = 0;

        while (p_idx < p_cps.size() && t_idx < t_cps.size()) {
            char32_t pc = p_cps[p_idx];
            char32_t tc = t_cps[t_idx];

            if (ignore_case) {
                if (pc < 128) pc = std::tolower(static_cast<char>(pc));
                if (tc < 128) tc = std::tolower(static_cast<char>(tc));
            }

            if (pc == tc) {
                ++p_idx;
            }
            ++t_idx;
        }
        return (p_idx == p_cps.size());
    }

    // =========================================================================
    // 5. 제로-할당 뷰 기반 유틸리티 구현
    // =========================================================================

    std::string_view ltrim_view(std::string_view sv) {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
            sv.remove_prefix(1);
        }
        return sv;
    }

    std::string_view rtrim_view(std::string_view sv) {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
            sv.remove_suffix(1);
        }
        return sv;
    }

    std::string_view trim_view(std::string_view sv) {
        return rtrim_view(ltrim_view(sv));
    }

    std::vector<std::string_view> split_view(std::string_view s, char delimiter, bool keep_empty) {
        std::vector<std::string_view> result;
        std::size_t start = 0;
        while (start <= s.size()) {
            std::size_t pos = s.find(delimiter, start);
            if (pos == std::string_view::npos) {
                std::string_view token = s.substr(start);
                if (keep_empty || !token.empty()) result.push_back(token);
                break;
            }
            std::string_view token = s.substr(start, pos - start);
            if (keep_empty || !token.empty()) result.push_back(token);
            start = pos + 1;
        }
        return result;
    }

    std::vector<std::string_view> split_view(std::string_view s, std::string_view delimiter, bool keep_empty) {
        std::vector<std::string_view> result;
        if (delimiter.empty()) {
            if (!s.empty()) result.push_back(s);
            return result;
        }
        std::size_t start = 0;
        while (start <= s.size()) {
            std::size_t pos = s.find(delimiter, start);
            if (pos == std::string_view::npos) {
                std::string_view token = s.substr(start);
                if (keep_empty || !token.empty()) result.push_back(token);
                break;
            }
            std::string_view token = s.substr(start, pos - start);
            if (keep_empty || !token.empty()) result.push_back(token);
            start = pos + delimiter.size();
        }
        return result;
    }

    // =========================================================================
    // 6. 구조화 데이터 안전 토크나이저 구현
    // =========================================================================

    std::vector<std::string> split_csv(std::string_view line, char delim, char quote) {
        std::vector<std::string> fields;
        std::string current;
        bool in_quotes = false;

        for (std::size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (in_quotes) {
                if (c == quote) {
                    if (i + 1 < line.size() && line[i + 1] == quote) {
                        current.push_back(quote);
                        ++i;
                    }
                    else {
                        in_quotes = false;
                    }
                }
                else {
                    current.push_back(c);
                }
            }
            else {
                if (c == quote) {
                    in_quotes = true;
                }
                else if (c == delim) {
                    fields.push_back(std::move(current));
                    current.clear();
                }
                else {
                    current.push_back(c);
                }
            }
        }
        fields.push_back(std::move(current));
        return fields;
    }

    std::string format_map(
        std::string_view tmpl,
        const std::unordered_map<std::string, std::string>& vars,
        std::string_view prefix,
        std::string_view suffix)
    {
        std::string result;
        std::size_t pos = 0;

        while (pos < tmpl.size()) {
            std::size_t start = tmpl.find(prefix, pos);
            if (start == std::string_view::npos) {
                result.append(tmpl.substr(pos));
                break;
            }
            result.append(tmpl.substr(pos, start - pos));

            std::size_t key_start = start + prefix.size();
            std::size_t end = tmpl.find(suffix, key_start);
            if (end == std::string_view::npos) {
                result.append(tmpl.substr(start));
                break;
            }

            std::string key(tmpl.substr(key_start, end - key_start));
            auto it = vars.find(key);
            if (it != vars.end()) {
                result.append(it->second);
            }
            else {
                result.append(tmpl.substr(start, (end + suffix.size()) - start));
            }
            pos = end + suffix.size();
        }
        return result;
    }

} // namespace mino::core::string
