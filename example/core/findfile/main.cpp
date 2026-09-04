#include <iostream>
#include <string>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/findfile/findfile.hpp"

namespace mcs = mino::core::string;
auto tce = mcs::to_console_encoding;

namespace mcsp = mino::core::string::print;
auto print = [](std::string_view fmt, auto&&... args) {
    mcsp::print(fmt, std::forward<decltype(args)>(args)...);
    };
auto println = [](std::string_view fmt, auto&&... args) {
    mcsp::println(fmt, std::forward<decltype(args)>(args)...);
    };

void print_matches(const std::string& title, const std::string& search_string, const std::vector<mino::core::findfile::search_match>& matches) {
    println("====================================================");
    println(" [시나리오] {}", title);
    println(" 검색 문자열: {}", search_string);
    println(" 검색된 일치 건수: {}", matches.size());
    println("====================================================");
    for (size_t i = 0; i < matches.size() && i < 3; ++i) {
        const auto& m = matches[i];
        println("파일명: {}", m.file_name);
        println("경로  : {}", m.file_path);
        println("위치  : {}번째 줄, {}열", m.line_number, m.column_number);
        println("내용  : {}", m.matched_line);
        println("----------------------------------------------------");
    }
    println("\n");
}

int main(int argc, char* argv[]) {
    namespace mcs = mino::core::string;
    auto tce = mcs::to_console_encoding;

    namespace mcdt = mino::core::datetime;
    namespace mcdtu = mcdt::util;

    namespace mcf = mino::core::findfile;
    using search_options = mcf::search_options;
    using file_searcher = mcf::file_searcher;
    using text_encoding = mcf::text_encoding;

    {
        std::string source_dir = SOURCE_DIR;
        std::filesystem::path source_path(source_dir);
#ifdef _WIN32
        std::filesystem::path search_path = (source_path / "windows");
#else
        std::filesystem::path search_path = (source_path / "linux");
#endif 

        println(">> 파일 검색 경로: {}", search_path.string());
    }

    // ------------------------------------------------------------------------
    // 시나리오 1. 한글 경로 + 한글 본문 검색
    // (Windows : 경로 CP949, 본문 CP949) (Linux : 경로 UTF-8, 본문 UTF-8)
    // ------------------------------------------------------------------------
    {
        search_options opt;
#ifdef _WIN32
        opt.file_path_encoding    = text_encoding::cp949; 
        opt.file_content_encoding = text_encoding::cp949; 
#else
        opt.file_path_encoding    = text_encoding::utf8;
        opt.file_content_encoding = text_encoding::utf8; 
#endif
        opt.include_wildcards = { "로그_*.txt", "사용자_*.log" }; // UTF-8 문자열 
        opt.exclude_wildcards = { "임시_폴더" }; // UTF-8 문자열
        opt.case_sensitive = false; // 대소문자 구분 없이 검색

        file_searcher searcher(opt);

        std::string source_dir = SOURCE_DIR; // 현재 소스 코드 경로
        std::filesystem::path source_path(source_dir);
#ifdef _WIN32
        std::filesystem::path search_path = (source_path / "windows");
#else
        std::filesystem::path search_path = (source_path / "linux");
#endif 
        std::string han_dir = tce("한글_디렉터리"); // 윈도:CP949, 리눅:UTF8
        std::filesystem::path hangul_path = search_path / han_dir;  // 검색 대상인 경로

        std::string search_string_utf8 = "심각한_오류"; // UTF-8 문자열
        std::string search_string = tce(search_string_utf8); // 윈도:CP949, 리눅:UTF8

        auto results = searcher.search(hangul_path.string(), search_string); // 검색 수행

#ifdef _WIN32
        print_matches("1. Windows 레거시 환경 (경로: CP949 + 본문: CP949)", search_string_utf8, results);
#else
        print_matches("1. Linux 레거시 환경 (경로: UTF8 + 본문: UTF8)", search_string_utf8, results);
#endif 
    }

    // ------------------------------------------------------------------------
    // 시나리오 2. Linux / 최신 크로스 플랫폼 환경 (경로: UTF-8, 본문: UTF-8)
    // 예: /var/log/한글경로/ 아래의 모든 파일에서 한글 정규식 패턴 검색
    // ------------------------------------------------------------------------
    {
        search_options opt;
#ifdef _WIN32
        opt.file_path_encoding    = text_encoding::cp949;
        opt.file_content_encoding = text_encoding::cp949;
#else
        opt.file_path_encoding    = text_encoding::utf8;
        opt.file_content_encoding = text_encoding::utf8;
#endif
        opt.include_wildcards = { "*.json", "*.log" }; // UTF-8 문자열
        opt.case_sensitive = false; // 대소문자 구분 없이 검색
        opt.use_regex = true; // 정규식 검색 활성화

        file_searcher searcher(opt);

        std::string source_dir = SOURCE_DIR; // 현재 소스 코드 경로
        std::filesystem::path source_path(source_dir);
#ifdef _WIN32
        std::filesystem::path search_path = (source_path / "windows");
#else
        std::filesystem::path search_path = (source_path / "linux");
#endif 
        std::string han_dir = tce("한글_디렉터리"); // 윈도:CP949, 리눅:UTF8  
        std::filesystem::path hangul_path = search_path / han_dir; // 검색 대상인 경로

        // 본문 내 한글이 포함된 형태(예: "에러코드_숫자") 정규식 검색
        std::string korean_regex_utf8 = R"(에러코드_\d{4})"; // UTF-8 문자열
        std::string korean_regex = tce(korean_regex_utf8); // 윈도:CP949, 리눅:UTF8

        auto results = searcher.search(hangul_path.string(), korean_regex); // 정규식 검색 수행

#ifdef _WIN32
        print_matches("2. Windows / 표준 환경 (경로: CP949 + 본문: CP949 정규식)", korean_regex_utf8, results);
#else
        print_matches("2. Linux / 표준 환경 (경로: UTF-8 + 본문: UTF-8 정규식)", korean_regex_utf8, results);
#endif 
    }

    // ------------------------------------------------------------------------
    // 시나리오 3. 혼합 환경 (파일명/경로는 UTF-8, 본문 파일은 과거 CP949 ANSI 문서)
    // 용도: Git 저장소(UTF-8 경로) 내에 저장된 과거 한국어 CP949 소스/문서 파일 검색
    // ------------------------------------------------------------------------
    {
        search_options opt;
#ifdef _WIN32
        opt.file_path_encoding = text_encoding::cp949;
#else
        opt.file_path_encoding = text_encoding::utf8;
#endif
        opt.file_content_encoding = text_encoding::utf8; // 파일 안의 내용은 UTF-8로 파일 가정

        opt.include_wildcards = { "*.cpp", "*.h", "*.txt" };
        opt.max_file_size_bytes = 10 * 1024 * 1024; // 10MB 이하

        file_searcher searcher(opt);

        std::string source_dir = SOURCE_DIR; // 현재 소스 코드 경로
        std::filesystem::path source_path(source_dir);
#ifdef _WIN32
        std::filesystem::path search_path = (source_path / "windows");
#else
        std::filesystem::path search_path = (source_path / "linux");
#endif 
        std::string han_dir = tce("한글_디렉터리"); // 윈도:CP949, 리눅:UTF8 
        std::filesystem::path hangul_path = search_path / han_dir; // 검색 대상인 경로

        // 검색하려는 문자열
        auto search_string_utf8 = "접속성공"; // UTF-8 문자열

        auto results = searcher.search(hangul_path.string(), search_string_utf8);

        print_matches("3. 혼합 환경 (경로: 네이티브 + 본문: UTF-8)", search_string_utf8, results);
    }

    // ------------------------------------------------------------------------
        // 시나리오 4. 한글 접두어 + 날짜/시간 구간 정규식 결합 검색
        // ------------------------------------------------------------------------
    {
        search_options opt;

#ifdef _WIN32
        opt.file_path_encoding = text_encoding::cp949;
        opt.file_content_encoding = text_encoding::cp949;
#else
        opt.file_path_encoding = text_encoding::utf8;
        opt.file_content_encoding = text_encoding::utf8;
#endif

        // 파일명 정규식 생성 (UTF-8 문자열)
        // => "서버로그_" 로 시작하고, 년월일시분초 정보가 오고, ".log" 로 끝나는 파일.

        // 문자열 타입
        //std::string date_regex = mcf::build_datetime_range_regex(
        //    "20260904100000", "20260904100159", "^서버로그_", R"(\.log$)"
        // );

        // 시간정보 구조체 타입
        std::string date_regex = mcf::build_datetime_range_regex(
            { {2026, 9, 4}, {10, 0, 0} }, { {2026, 9, 4}, {10, 1, 59} }, "^서버로그_", R"(\.log$)"
        ); 

        opt.include_regex_patterns.push_back(date_regex); // 정규식 검색 패턴 추가
        opt.use_regex = true; // 정규식 검색 활성화

        auto combined_regex = opt.get_combined_include_regex();
        println("정규식 정보: {}", combined_regex); // 정규식 정보 출력

        file_searcher searcher(opt);

        std::string source_dir = SOURCE_DIR;
        std::filesystem::path source_path(source_dir);
#ifdef _WIN32
        std::filesystem::path search_path = (source_path / "windows");
#else
        std::filesystem::path search_path = (source_path / "linux");
#endif 
        std::string han_dir = tce("한글_디렉터리"); // 윈도:CP949, 리눅:UTF8
        std::filesystem::path hangul_path = search_path / han_dir; // 검색 대상인 경로

        std::string search_string_utf8 = "FATAL긴급"; // UTF-8 문자열
        std::string search_string = tce(search_string_utf8); // 윈도:CP949, 리눅:UTF8

        auto results = searcher.search(hangul_path.string(), search_string);

        print_matches("4. 한글 파일명 + 날짜 범위 정규식 검색", search_string_utf8, results);
    }

    return 0;
}
