#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <ostream>
#include <cstdint>
#include <functional>
#include <regex>
#include <utility>

namespace mino::core::file {

    namespace fs = std::filesystem;

    class  file_finder {
    public:
        // 정렬 키
        enum class sort_key { none = 0, path, name, mtime, size };

        // 결과 항목
        struct entry {
            fs::path path;   // 발견한 경로
            bool is_file = false; // 파일 여부
            bool is_dir = false; // 디렉토리 여부
            bool is_symlink = false; // 심볼릭 링크 여부

            // 메타데이터(없을 수도 있음)
            std::optional<std::chrono::system_clock::time_point> mtime;
            std::optional<std::uintmax_t> size;

            // "발견 시각"(탐색기가 해당 항목을 집계한 순간)
            std::chrono::system_clock::time_point found_time{};
        }; // ← 세미콜론 누락 주의

        // 탐색 옵션(기존 options → finder_options 로 명칭 변경)
        struct finder_options {
            // 탐색 동작
            bool recursive = true;               // 재귀 탐색
            bool follow_symlinks = false;        // 심볼릭 링크 추적
            int  max_depth = -1;                 // -1: 제한 없음, 0: 자식만
            bool skip_permission_denied = true;  // 권한 오류 건너뛰기
            bool stop_on_error = false;          // 오류 시 즉시 중단

            // 결과 포함 대상
            bool include_files = true;           // 파일 결과 포함
            bool include_dirs = false;          // 디렉토리 결과 포함
            bool include_symlinks_as_items = false; // 심볼릭 링크 자체를 결과로 포함

            // 이름/패턴 필터
            std::vector<std::string> extensions;     // ".cpp" 등
            std::vector<std::string> include_globs;  // "*.cpp" 등
            std::vector<std::string> exclude_globs;  // "*.gen.*" 등
            std::vector<std::string> exclude_dir_globs; // "build*", ".git" 등
            bool ext_case_sensitive = false;         // 확장자 대소문자 구분 여부
            bool include_hidden = true;              // 숨김 포함(유닉스 '.' / 윈도우 Hidden)
            bool include_system = true;              // 시스템 포함(윈도우 전용, 유닉스/맥: 무시)
            std::optional<std::regex> name_regex;    // 파일/디렉토리 "이름" 정규식

            // 시간/크기 필터
            std::optional<std::chrono::system_clock::time_point> mtime_since; // 시각 이후
            std::optional<std::chrono::system_clock::time_point> mtime_until; // 시각 이전
            std::optional<std::uintmax_t> min_size;  // 바이트
            std::optional<std::uintmax_t> max_size;  // 바이트
            bool compute_size = false;               // 파일 크기 수집 여부

            // 결과 제어
            std::size_t limit_results = 0;     // 0 = 제한 없음, 양수 = 최대 결과 수
            sort_key sort_by = sort_key::none; // 정렬하지 않으면 먼저 발견된 순서대로 결과 유지
            bool sort_ascending = true;        // 정렬시 오름차순 정렬 여부

            // 방문자(스트리밍) 콜백: true=계속, false=중단
            std::function<bool(const entry&)> on_visit;
        }; // ← 세미콜론 누락 주의

    public:
        // 정적 편의 함수(예외 미사용)
        static std::vector<entry> find(const fs::path& root,
            const finder_options& opt) noexcept;

    public:
        file_finder() = default;                                  // 기본 생성자
        explicit file_finder(finder_options opt)                  // 옵션 지정 생성자
            : opts_(std::move(opt)) {
        }

        void set_options(const finder_options& opt) { opts_ = opt; }
        const finder_options& options() const { return opts_; }

        // 인스턴스 메서드(예외 미사용)
        std::vector<entry> find(const fs::path& root) noexcept;

        // 최근 에러 문자열들(권한 거부/상태 조회 실패 등)
        const std::vector<std::string>& last_errors() const noexcept { return last_errors_; }
        void clear_errors() noexcept { last_errors_.clear(); }

        // 유틸
        static bool exists_now(const fs::path& p) noexcept;
        static std::time_t to_time_t(const std::chrono::system_clock::time_point& tp) noexcept;
        static void print_entry_safe(const entry& e, std::ostream& os) noexcept;

    private:
        finder_options opts_{};                // ← options_ → opts_
        std::vector<std::string> last_errors_;
    };

    // ===== 통합 헬퍼 exists() (UTF-8/유니코드 안전 버전) =====
    // 기본값: recursive=false(비재귀), case_insensitive=true(대소문자 무시)
    //
    // 문자열 버전(호환용)

     bool exists(
        const std::string& utf8_root, // 찾기 시작하는 루트 경로 (UTF-8 문자열)
        const std::string& utf8_filename, // 찾을 파일명 (예: "report.txt") (UTF-8 문자열)
        bool recursive = false, // 재귀 탐색 여부 (기본: false)
        bool case_insensitive = true // 대소문자 무시 여부 (기본: true)
    ) noexcept;

     bool exists(
        const std::filesystem::path& root, // 찾기 시작하는 루트 경로
        const std::string& utf8_filename, // 찾을 파일명 (예: "report.txt") (UTF-8 문자열)
        bool recursive = false, // 재귀 탐색 여부 (기본: false)
        bool case_insensitive = true // 대소문자 무시 여부 (기본: true)
    ) noexcept;

     bool exists(
        const std::string& utf8_root, // 찾기 시작하는 루트 경로 (UTF-8 문자열)
        const std::filesystem::path& filename, // 찾을 파일명 (예: "report.txt") (fs::path로 유니코드 안전)
        bool recursive = false, // 재귀 탐색 여부 (기본: false)
        bool case_insensitive = true // 대소문자 무시 여부 (기본: true)
    ) noexcept;

    // 경로 버전(권장: 유니코드 안전)
     bool exists(
        const std::filesystem::path& root, // 찾기 시작하는 루트 경로
        const std::filesystem::path& filename, // 찾을 파일명 (예: "report.txt") (fs::path로 유니코드 안전)
        bool recursive = false, // 재귀 탐색 여부 (기본: false)
        bool case_insensitive = true // 대소문자 무시 여부 (기본: true)
    ) noexcept;

}  
