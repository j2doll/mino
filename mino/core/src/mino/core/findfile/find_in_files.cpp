#include "mino/core/string/string.hpp"
#include "mino/core/findfile/find_in_files.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <iconv.h>
#include <cstring>
#endif

namespace mino::core::findfile {

    namespace fs = std::filesystem;

    // ----------------------------------------------------------------------------
    // 인코딩 변환기 구현부 (Windows Win32 API / Linux iconv)
    // ----------------------------------------------------------------------------
    namespace encoding_util {

#if defined(_WIN32)

        std::string cp949_to_utf8(std::string_view cp949_str) {
            if (cp949_str.empty()) return {};

            // CP949(Code Page 949) -> UTF-16
            int wlen = MultiByteToWideChar(949, 0, cp949_str.data(), static_cast<int>(cp949_str.size()), nullptr, 0);
            if (wlen <= 0) return std::string(cp949_str);

            std::wstring wstr(wlen, 0);
            MultiByteToWideChar(949, 0, cp949_str.data(), static_cast<int>(cp949_str.size()), &wstr[0], wlen);

            // UTF-16 -> UTF-8
            int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, nullptr, 0, nullptr, nullptr);
            if (ulen <= 0) return std::string(cp949_str);

            std::string utf8_str(ulen, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, &utf8_str[0], ulen, nullptr, nullptr);
            return utf8_str;
        }

        std::string utf8_to_cp949(std::string_view utf8_str) {
            if (utf8_str.empty()) return {};

            int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
            if (wlen <= 0) return std::string(utf8_str);

            std::wstring wstr(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), &wstr[0], wlen);

            int mblen = WideCharToMultiByte(949, 0, wstr.data(), wlen, nullptr, 0, nullptr, nullptr);
            if (mblen <= 0) return std::string(utf8_str);

            std::string cp949_str(mblen, 0);
            WideCharToMultiByte(949, 0, wstr.data(), wlen, &cp949_str[0], mblen, nullptr, nullptr);
            return cp949_str;
        }

        fs::path string_to_path(const std::string& raw_path, text_encoding enc) {
            if (enc == text_encoding::utf8) {
                return fs::u8path(raw_path);
            }
            // CP949인 경우 MultiByteToWideChar로 변환 후 fs::path 생성
            int wlen = MultiByteToWideChar(949, 0, raw_path.data(), static_cast<int>(raw_path.size()), nullptr, 0);
            if (wlen <= 0) return fs::path(raw_path);
            std::wstring wstr(wlen, 0);
            MultiByteToWideChar(949, 0, raw_path.data(), static_cast<int>(raw_path.size()), &wstr[0], wlen);
            return fs::path(wstr);
        }

        std::string path_to_utf8_string(const fs::path& p) {
            auto u8str = p.u8string();
            return std::string(u8str.begin(), u8str.end());
        }

#else // Linux, macOS (iconv 기반)

        static std::string iconv_convert(std::string_view input, const char* from_enc, const char* to_enc) {
            if (input.empty()) return {};

            iconv_t cd = iconv_open(to_enc, from_enc);
            if (cd == (iconv_t)-1) {
                return std::string(input);
            }

            size_t in_bytes = input.size();
            char* in_buf = const_cast<char*>(input.data());

            size_t out_bytes = in_bytes * 4 + 16;
            std::string output(out_bytes, 0);
            char* out_buf = &output[0];
            size_t out_avail = out_bytes;

            if (iconv(cd, &in_buf, &in_bytes, &out_buf, &out_avail) == (size_t)-1) {
                iconv_close(cd);
                return std::string(input);
            }

            iconv_close(cd);
            output.resize(out_bytes - out_avail);
            return output;
        }

        std::string cp949_to_utf8(std::string_view cp949_str) {
            return iconv_convert(cp949_str, "CP949", "UTF-8");
        }

        std::string utf8_to_cp949(std::string_view utf8_str) {
            return iconv_convert(utf8_str, "UTF-8", "CP949");
        }

        fs::path string_to_path(const std::string& raw_path, text_encoding enc) {
            if (enc == text_encoding::cp949) {
                return fs::u8path(cp949_to_utf8(raw_path));
            }
            return fs::u8path(raw_path);
        }

        std::string path_to_utf8_string(const fs::path& p) {
            return p.string(); // Linux/POSIX는 기본 UTF-8
        }

#endif

    } // namespace encoding_util

    // ----------------------------------------------------------------------------
    // 일시 정규식 분할 보조
    // ----------------------------------------------------------------------------
    namespace {

        inline std::string digit_range_to_regex(int low, int high) {
            if (low == high) return std::to_string(low);
            if (low == 0 && high == 9) return R"(\d)";
            return "[" + std::to_string(low) + "-" + std::to_string(high) + "]";
        }

        void split_numeric_range_to_regex(
            const std::string& start_str,
            const std::string& end_str,
            size_t idx,
            const std::string& current_prefix,
            std::vector<std::string>& patterns)
        {
            if (idx >= start_str.size()) {
                patterns.push_back(current_prefix);
                return;
            }

            int start_digit = start_str[idx] - '0';
            int end_digit = end_str[idx] - '0';

            if (start_digit == end_digit) {
                split_numeric_range_to_regex(
                    start_str, end_str, idx + 1,
                    current_prefix + std::to_string(start_digit), patterns
                );
                return;
            }

            std::string lower_bound_end = start_str;
            for (size_t i = idx + 1; i < lower_bound_end.size(); ++i) lower_bound_end[i] = '9';
            split_numeric_range_to_regex(start_str, lower_bound_end, idx + 1, current_prefix + std::to_string(start_digit), patterns);

            if (start_digit + 1 <= end_digit - 1) {
                size_t remaining_digits = start_str.size() - 1 - idx;
                std::string mid_pattern = current_prefix + digit_range_to_regex(start_digit + 1, end_digit - 1);
                if (remaining_digits > 0) mid_pattern += R"(\d{)" + std::to_string(remaining_digits) + "}";
                patterns.push_back(mid_pattern);
            }

            std::string upper_bound_start = end_str;
            for (size_t i = idx + 1; i < upper_bound_start.size(); ++i) upper_bound_start[i] = '0';
            split_numeric_range_to_regex(upper_bound_start, end_str, idx + 1, current_prefix + std::to_string(end_digit), patterns);
        }

    } // anonymous namespace

    std::string build_datetime_range_regex(
        const std::string& start_datetime,
        const std::string& end_datetime,
        const std::string& prefix_pattern,
        const std::string& suffix_pattern)
    {
        if (start_datetime.size() != end_datetime.size() || start_datetime > end_datetime || start_datetime.empty()) {
            return "";
        }

        std::vector<std::string> sub_patterns;
        split_numeric_range_to_regex(start_datetime, end_datetime, 0, "", sub_patterns);

        std::string combined;
        for (size_t i = 0; i < sub_patterns.size(); ++i) {
            if (i > 0) combined += "|";
            combined += sub_patterns[i];
        }

        return prefix_pattern + "(" + combined + ")" + suffix_pattern;
    }

    std::string build_datetime_range_regex(
        splited_datetime start,
        splited_datetime end,
        const std::string& prefix_pattern,
        const std::string& suffix_pattern)
    {
        std::ostringstream start_ss, end_ss;

        start_ss << std::setw(4) << std::setfill('0') << start.date.year
                 << std::setw(2) << std::setfill('0') << start.date.month
                 << std::setw(2) << std::setfill('0') << start.date.day
                 << std::setw(2) << std::setfill('0') << start.time.hour
                 << std::setw(2) << std::setfill('0') << start.time.minute
                 << std::setw(2) << std::setfill('0') << start.time.second;

        end_ss << std::setw(4) << std::setfill('0') << end.date.year
               << std::setw(2) << std::setfill('0') << end.date.month
               << std::setw(2) << std::setfill('0') << end.date.day
               << std::setw(2) << std::setfill('0') << end.time.hour
               << std::setw(2) << std::setfill('0') << end.time.minute
               << std::setw(2) << std::setfill('0') << end.time.second;

        return build_datetime_range_regex(start_ss.str(), end_ss.str(), prefix_pattern, suffix_pattern);
    }

    // ----------------------------------------------------------------------------
    // file_filter
    // ----------------------------------------------------------------------------

    file_filter::file_filter(const search_options& options)
        : max_file_size_bytes_(options.max_file_size_bytes)
    {
        // 1. 와일드카드 패턴을 std::wregex로 변환하여 등록
        for (const auto& pattern : options.include_wildcards) {
            compiled_includes_.push_back(wildcard_to_regex(pattern));
        }
        for (const auto& pattern : options.exclude_wildcards) {
            compiled_excludes_.push_back(wildcard_to_regex(pattern));
        }

        // 2. 포함/제외 정규식 패턴(UTF-8)을 utf8_to_utf16 변환 후 std::wregex로 컴파일
        for (const auto& reg_str : options.include_regex_patterns) {
            try {
                std::wstring wreg_str = mino::core::string::utf8_to_utf16(reg_str);
                compiled_includes_.emplace_back(wreg_str, std::regex_constants::icase);
            }
            catch (const std::regex_error& e) {
                std::cerr << "[Warning] Invalid include regex: " << reg_str << " (" << e.what() << ")\n";
            }
        }
        for (const auto& reg_str : options.exclude_regex_patterns) {
            try {
                std::wstring wreg_str = mino::core::string::utf8_to_utf16(reg_str);
                compiled_excludes_.emplace_back(wreg_str, std::regex_constants::icase);
            }
            catch (const std::regex_error& e) {
                std::cerr << "[Warning] Invalid exclude regex: " << reg_str << " (" << e.what() << ")\n";
            }
        }
        has_includes_ = !compiled_includes_.empty();
    }

    bool file_filter::should_skip_directory(const std::string& dir_name_utf8) const {
        return matches_any(dir_name_utf8, compiled_excludes_);
    }

    bool file_filter::is_target_file(const fs::directory_entry& entry, const std::string& filename_utf8) const {
        if (!entry.is_regular_file()) {
            return false;
        }

        // 1. 제외 패턴 검사 (std::wregex 기반)
        if (!compiled_excludes_.empty() && matches_any(filename_utf8, compiled_excludes_)) {
            return false;
        }

        // 2. 파일 크기 제한 검사
        if (max_file_size_bytes_.has_value()) {
            std::error_code ec;
            uintmax_t size = entry.file_size(ec);
            if (!ec && size > max_file_size_bytes_.value()) {
                return false;
            }
        }

        // 3. 포함 패턴 검사 (단락 평가 적용: 패턴이 없으면 통과, 있으면 matches_any 평가)
        if (!has_includes_ || matches_any(filename_utf8, compiled_includes_)) {
            return true;
        }

        return false;
    }

    std::wregex file_filter::wildcard_to_regex(const std::string& pattern) {
        std::wstring wpattern = mino::core::string::utf8_to_utf16(pattern);
        std::wstring regex_str = L"^";

        for (wchar_t ch : wpattern) {
            switch (ch) {
            case L'*': regex_str += L".*"; break;
            case L'?': regex_str += L"."; break;
            case L'.': case L'^': case L'$': case L'+':
            case L'(': case L')': case L'[': case L']':
            case L'{': case L'}': case L'|': case L'\\':
                regex_str += L'\\';
                regex_str += ch;
                break;
            default:
                regex_str += ch;
                break;
            }
        }
        regex_str += L"$";
        return std::wregex(regex_str, std::regex_constants::icase);
    }

    bool file_filter::matches_any(const std::string& name, const std::vector<std::wregex>& regexes) {
        if (regexes.empty()) {
            return false;
        }

        // 1. UTF-8 문자열을 UTF-16 std::wstring으로 변환
        std::wstring wname = mino::core::string::utf8_to_utf16(name);

        if (wname.empty() && !name.empty()) {
            return false;
        }

        // 2. std::wregex를 사용하여 한글/유니코드 매칭 수행
        for (const auto& reg : regexes) {
            if (std::regex_match(wname, reg)) {
                return true;
            }
        }
        return false;
    }

    // ----------------------------------------------------------------------------
    // file_searcher
    // ----------------------------------------------------------------------------

    file_searcher::file_searcher(search_options options)
        : options_(std::move(options)), filter_(options_) {
    }

    std::string file_searcher::to_lower_ascii(std::string_view str) {
        std::string lower;
        lower.reserve(str.size());
        for (unsigned char ch : str) {
            lower.push_back(static_cast<char>(std::tolower(ch)));
        }
        return lower;
    }

    std::vector<search_match> file_searcher::search(const std::string& root_dir, const std::string& query) const {
        std::vector<search_match> results;

        // 1. root_dir을 경로 인코딩 설정에 맞추어 std::filesystem::path로 파싱
        fs::path root_path = encoding_util::string_to_path(root_dir, options_.file_path_encoding);

        // [방어 코드 1] 경로가 존재하지 않거나 디렉터리가 아닌 경우 조기 리턴
        std::error_code check_ec_1, check_ec_2;
        auto is_root_path_exists = fs::exists(root_path, check_ec_1);
        auto is_root_path_directory = fs::is_directory(root_path, check_ec_2);
        if (!is_root_path_exists || !is_root_path_directory) {
            std::cerr 
                << "[Error] Root path does not exist or is not a directory: " 
                << root_path.string() << " (ec: " << check_ec_1.message() << " / " << check_ec_2.message() << ")\n";
            return results;
        }        

        // 2. 쿼리를 내부 표준 UTF-8로 정규화
        std::string query_utf8 = (options_.file_content_encoding == text_encoding::cp949)
            ? encoding_util::cp949_to_utf8(query)
            : query;

        std::wregex query_regex;
        std::string lower_query_utf8;

        if (options_.use_regex) {
            auto flags = std::regex_constants::ECMAScript;
            if (!options_.case_sensitive) flags |= std::regex_constants::icase;
            try {
                // 한글/유니코드 정규식 지원을 위해 std::wstring으로 변환 후 wregex 컴파일
                std::wstring wquery = mino::core::string::utf8_to_utf16(query_utf8);
                query_regex = std::wregex(wquery, flags);
            }
            catch (const std::regex_error& e) {
                std::cerr << "Invalid query regex: " << e.what() << "\n";
                return results;
            }
        }
        else {
            if (!options_.case_sensitive) {
                lower_query_utf8 = to_lower_ascii(query_utf8);
            }
        }

        std::error_code ec;
        fs::recursive_directory_iterator iter(root_path, fs::directory_options::skip_permission_denied, ec);
        const fs::recursive_directory_iterator end_iter;

        while (iter != end_iter) {
            if (ec) {
                iter.increment(ec);
                continue;
            }

            const auto& entry = *iter;
            auto filename = entry.path().filename();
            std::string name_utf8 = encoding_util::path_to_utf8_string(filename);

            if (entry.is_directory()) {
                if (filter_.should_skip_directory(name_utf8)) {
                    iter.disable_recursion_pending();
                }
                iter.increment(ec);
                continue;
            }

            if (filter_.is_target_file(entry, name_utf8)) {
                if (options_.use_regex) {
                    search_regex(entry.path(), name_utf8, query_regex, results);
                }
                else {
                    search_plain(entry.path(), name_utf8, query_utf8, lower_query_utf8, results);
                }
            }

            iter.increment(ec);
        }

        return results;
    }

    void file_searcher::search_plain(
        const fs::path& path,
        const std::string& filename_utf8,
        const std::string& query_utf8,
        const std::string& lower_query_utf8,
        std::vector<search_match>& results) const
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;

        std::string raw_line;
        size_t line_number = 1;
        std::string full_path_utf8 = encoding_util::path_to_utf8_string(path);

        while (std::getline(file, raw_line)) {
            if (!raw_line.empty() && raw_line.back() == '\r') {
                raw_line.pop_back(); // CRLF 개행 제거
            }

            // 본문 인코딩이 CP949인 경우 UTF-8로 변환
            std::string line_utf8 = (options_.file_content_encoding == text_encoding::cp949)
                ? encoding_util::cp949_to_utf8(raw_line)
                : raw_line;

            std::string target_line = options_.case_sensitive ? line_utf8 : to_lower_ascii(line_utf8);
            const std::string& target_query = options_.case_sensitive ? query_utf8 : lower_query_utf8;

            size_t col_pos = target_line.find(target_query);
            while (col_pos != std::string::npos) {
                results.push_back({
                    filename_utf8,
                    full_path_utf8,
                    line_number,
                    col_pos + 1,
                    line_utf8
                    });
                col_pos = target_line.find(target_query, col_pos + target_query.length());
            }
            ++line_number;
        }
    }

    void file_searcher::search_regex(
        const fs::path& path,
        const std::string& filename_utf8,
        const std::wregex& query_regex,
        std::vector<search_match>& results) const
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;

        std::string raw_line;
        size_t line_number = 1;
        std::string full_path_utf8 = encoding_util::path_to_utf8_string(path);

        while (std::getline(file, raw_line)) {
            if (!raw_line.empty() && raw_line.back() == '\r') {
                raw_line.pop_back();
            }

            std::string line_utf8 = (options_.file_content_encoding == text_encoding::cp949)
                ? encoding_util::cp949_to_utf8(raw_line)
                : raw_line;

            // 한글/유니코드 매칭을 위해 행(Line)을 std::wstring으로 변환
            std::wstring wline = mino::core::string::utf8_to_utf16(line_utf8);

            auto words_begin = std::wsregex_iterator(wline.begin(), wline.end(), query_regex);
            auto words_end = std::wsregex_iterator();

            for (auto it = words_begin; it != words_end; ++it) {
                const std::wsmatch& match = *it;
                results.push_back({
                    filename_utf8,
                    full_path_utf8,
                    line_number,
                    static_cast<size_t>(match.position() + 1),
                    line_utf8
                    });
            }
            ++line_number;
        }
    }

} // namespace mino::core::findfile
