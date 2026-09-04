#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <regex>
#include <filesystem>
#include <cstdint>
#include <optional>

namespace mino::core::findfile {

    // 지원하는 문자열 인코딩 열거형
    enum class text_encoding {
        utf8,
        cp949
    };

    // 검색 결과 정보 구조체 (file_name, file_path, matched_line 모두 UTF-8로 정규화되어 반환)
    struct search_match {
        std::string file_name;
        std::string file_path;
        size_t line_number;
        size_t column_number;
        std::string matched_line;
    };

    // 검색 상세 설정 옵션 구조체
    struct search_options {
        // 1. 인코딩 설정
        text_encoding file_path_encoding = text_encoding::utf8;    // 파일명/경로 인코딩 (Windows 한글 기본: cp949)
        text_encoding file_content_encoding = text_encoding::utf8; // 파일 본문 인코딩 (Windows 텍스트 기본: cp949)

        // 2. 파일/디렉터리 필터링 (UTF-8 문자열 기준)
        std::vector<std::string> include_wildcards;// 검색 대상 파일명 와일드카드 패턴 (예: "*.txt", "log_*.log")
        std::vector<std::string> include_regex_patterns; // 검색 대상 파일명 정규식 패턴 (예: R"(log_\d{8}\.txt$)")
        std::vector<std::string> exclude_wildcards; // 검색 제외 파일명 와일드카드 패턴 (예: "temp_*", "*.bak")
        std::vector<std::string> exclude_regex_patterns; // 검색 제외 파일명 정규식 패턴 (예: R"(.*temp_.*)", R"(.*\.bak$)")

        // 3. 파일 크기 제한
        std::optional<uintmax_t> max_file_size_bytes = std::nullopt;

        // 4. 본문 검색 옵션
        bool case_sensitive = true; // 대소문자 구분 여부
        bool use_regex = false; // 본문 검색 시 정규식 사용 여부

        // 11. 등록된 포함 정규식 중 첫 번째(혹은 생성된 정규식) 원본 문자열 반환
        std::string get_first_include_regex() const {
            if (!include_regex_patterns.empty()) {
                return include_regex_patterns.front();
            }
            return "";
        }

        // 12. 등록된 모든 포함 정규식 문자열을 OR(|)로 결합하여 하나의 정규식 문자열로 반환
        std::string get_combined_include_regex() const {
            if (include_regex_patterns.empty()) {
                return "";
            }
            if (include_regex_patterns.size() == 1) {
                return include_regex_patterns.front();
            }

            std::string combined;
            for (size_t i = 0; i < include_regex_patterns.size(); ++i) {
                if (i > 0) combined += "|";
                combined += "(?:" + include_regex_patterns[i] + ")";
            }
            return combined;
        }
    };

    // 인코딩 변환 헬퍼 네임스페이스
    namespace encoding_util {
        std::string cp949_to_utf8(std::string_view cp949_str);
        std::string utf8_to_cp949(std::string_view utf8_str);
        std::filesystem::path string_to_path(const std::string& raw_path, text_encoding enc);
        std::string path_to_utf8_string(const std::filesystem::path& p);
    }

    // 일시 범위 정규식 생성 함수
    std::string build_datetime_range_regex(
        const std::string& start_datetime,
        const std::string& end_datetime,
        const std::string& prefix_pattern = "",
        const std::string& suffix_pattern = "");

    struct splited_date {
        int year;
        int month;
        int day;
    };
    struct splited_time {
        int hour;
        int minute;
        int second;
    };
    struct splited_datetime {
        splited_date date;
        splited_time time;
    };
    std::string build_datetime_range_regex(splited_datetime start, splited_datetime end, const std::string& prefix_pattern = "", const std::string& suffix_pattern = "");

    // 파일/디렉터리 필터링 클래스
    class file_filter {
    public:
        explicit file_filter(const search_options& options);

        bool should_skip_directory(const std::string& dir_name_utf8) const;
        bool is_target_file(const std::filesystem::directory_entry& entry, const std::string& filename_utf8) const;

    private:
        static std::wregex wildcard_to_regex(const std::string& pattern);
        static bool matches_any(const std::string& name, const std::vector<std::wregex>& regexes);

        std::vector<std::wregex> compiled_includes_;
        std::vector<std::wregex> compiled_excludes_;
        std::optional<uintmax_t> max_file_size_bytes_;
        bool has_includes_ = false;
    };

    // 파일 검색 및 본문 매칭 엔진 클래스
    class file_searcher {
    public:
        explicit file_searcher(search_options options);

        // root_dir과 query는 options.file_path_encoding/UTF-8에 맞게 전달
        std::vector<search_match> search(const std::string& root_dir, const std::string& query) const;

    private:
        static std::string to_lower_ascii(std::string_view str);

        void search_plain(
            const std::filesystem::path& path,
            const std::string& filename_utf8,
            const std::string& query_utf8,
            const std::string& lower_query_utf8,
            std::vector<search_match>& results) const;

        void search_regex(
            const std::filesystem::path& path,
            const std::string& filename_utf8,
            const std::wregex& query_regex,
            std::vector<search_match>& results) const;

        search_options options_;
        file_filter filter_;
    };

} // namespace mino::core::findfile
