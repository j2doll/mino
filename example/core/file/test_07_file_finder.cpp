#include <filesystem>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_file_finder(std::filesystem::path root_dir)
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    namespace mcs = mino::core::string;

    auto to_console_encoding = mcs::to_console_encoding;

    using file_finder = mcfile::file_finder;
    using finder_options = mcfile::file_finder::finder_options;
    using sort_key = mcfile::file_finder::sort_key;

    //-----------------------------------------------
    // 7-2. file_finder::finder_options 구조체의 모든 멤버 변수 설정

    finder_options fopt; // 파일 찾기 옵션 구조체

    fopt.recursive = true; // 하위 디렉터리까지 재귀적으로 탐색 (디폴트 true)   
    fopt.follow_symlinks = false; // 심볼릭 링크를 따라가지 않음 (디폴트 false)
    fopt.max_depth = -1; // -1은 무제한, 0은 현재 디렉터리만, 1은 현재 + 1단계 하위 디렉터리, ... (디폴트 -1)
    fopt.skip_permission_denied = true; // 권한 거부된 디렉터리 무시 (디폴트 true)
    fopt.stop_on_error = false; // 에러 발생 시 탐색 중단하지 않음 (디폴트 false)

    fopt.include_files = true; // 파일 항목 포함 (디폴트 true)
    fopt.include_dirs = true; // 디렉터리 항목 포함 (디폴트 false)
    fopt.include_symlinks_as_items = false; // 심볼릭 링크를 항목으로 포함하지 않음 (디폴트 false)

    fopt.extensions = { ".cpp", ".txt" }; // 확장자 필터링. 미설정시 모든 확장자 포함.
    fopt.include_globs = { "*" }; // 포함할 파일 이름 패턴. 미설정시 모든 파일 포함.   
    fopt.exclude_globs = { "*.tmp" }; // 제외할 파일 이름 패턴. 미설정시 모든 파일 포함.
    fopt.exclude_dir_globs = { ".git", "build*" }; // 제외할 디렉터리 이름 패턴. 미설정시 모든 디렉터리 포함.
    fopt.ext_case_sensitive = false; // 확장자 대소문자 구분하지 않음 (디폴트 false)
    fopt.include_hidden = true; // 숨김 파일/디렉터리 포함 (디폴트 true)
    fopt.include_system = true; // 시스템 파일/디렉터리 포함 (디폴트 true)
    fopt.name_regex = std::regex(".*"); // 이름 정규식 필터링. 미설정시 모든 이름 포함.
    fopt.mtime_since = std::chrono::system_clock::now() - std::chrono::hours(24); // -24H 이전 파일만 포함. 미설정시 모든 파일 포함
    fopt.mtime_until = std::chrono::system_clock::now() + std::chrono::hours(24); // +24H 이후 파일만 포함. 미설정시 모든 파일 포함
    fopt.min_size = 0; // 최소 파일 크기 0바이트. 미설정시 모든 크기 포함.
    fopt.max_size = 1024 * 1024 * 10; // 최대 파일 크기 10MB. 미설정시 모든 크기 포함.  
    fopt.compute_size = true; // 파일 크기 수집 여부(size 멤버에 파일 크기 저장). (디폴트 false)

    fopt.limit_results = 100; // 최대 결과 수 제한. (0이면 제한 없음)
    fopt.sort_by = sort_key::name; // 이름 기준 정렬. 미설정시 발견된 순서대로 결과 유지
    fopt.sort_ascending = true; // 오름차순 정렬. 미설정시 오름차순 정렬

    // 파일이 찾아 질때마다 호출되는 콜백. (true 반환 시 탐색 계속, false 반환 시 탐색 중단.)
    fopt.on_visit = [](const mino::core::file::file_finder::entry& entr) -> bool {
        using namespace std::chrono;
        auto to_sec = [](auto t) { return duration_cast<seconds>(t.time_since_epoch()).count(); };
        auto mtime = entr.mtime ? std::to_string(to_sec(*entr.mtime)) : "null";
        auto msize = entr.size.has_value() ? std::to_string(*entr.size) : "null";
        auto found_time = duration_cast<seconds>(entr.found_time.time_since_epoch()).count();
        mcsp::println(
            "방문 콜백:\n 경로={},\n 파일 여부={},\n 디렉터리 여부={},\n 심볼릭 링크 여부={},\n 메타데이터 시간={},\n 메타데이터 크기={},\n 발견 시각={}\n\n",
            entr.path.string(), // 발견한 경로
            entr.is_file, // 발견한 경로가 파일인지 여부
            entr.is_dir, // 발견한 경로가 디렉터리인지 여부
            entr.is_symlink, // 발견한 경로가 심볼릭 링크인지 여부
            mtime, // 메타데이터 시간 (없을 수도 있음)
            msize, // 메타데이터 크기 (없을 수도 있음)
            found_time // 발견 시각 (UTC 기준 Unix Epoch(1970-01-01 00:00:00 UTC) 초 단위) (타임포인트를 그대로 사용해도 됨.)
        );
        return true; // true 반환 시 탐색 계속
    };

    //-----------------------------------------------
    // 7-3. 정적 메서드 file_finder::find() 호출

    auto static_results = file_finder::find(root_dir, fopt); // 파일 찾기 수행

    auto static_results_size = static_results.size();
    mcsp::println("정적 find() 로 찾은 항목 수: {}", static_results_size);

    //-----------------------------------------------
    // 7-4. 인스턴스 생성 및 메서드 테스트 (생성자, set_options, options, find, last_errors, clear_errors)

    file_finder finder_inst; // 기본 생성자
    finder_inst.set_options(fopt); // 옵션 설정
    const finder_options& current_opts = finder_inst.options(); // 현재 옵션 확인

    file_finder finder_inst_opt(fopt); // 옵션 생성자

    // on_visit 을 null로 비워서 결과 벡터를 수집
    fopt.on_visit = nullptr;
    finder_inst.set_options(fopt);

    auto inst_results = finder_inst.find(root_dir); // 파일 찾기 수행

    auto inst_results_size = inst_results.size();
    mcsp::println("인스턴스 find() 로 찾은 항목 수: {}", inst_results_size);

    // 에러 관리 메서드
    auto errs = finder_inst.last_errors();
    auto errs_size = errs.size();
    mcsp::println("마지막 에러 수: {}", errs_size);

    finder_inst.clear_errors(); // 에러 기록 초기화

    //-----------------------------------------------
    // 7-5. file_finder::entry 구조체의 모든 멤버 변수 접근 및 유틸리티 메서드 테스트

    for (auto e : inst_results) { // entry 멤버 직접 접근
        std::filesystem::path p = e.path; // 발견한 경로
        bool is_f = e.is_file; // 발견한 경로가 파일인지 여부
        bool is_d = e.is_dir; // 발견한 경로가 디렉터리인지 여부
        bool is_sym = e.is_symlink; // 발견한 경로가 심볼릭 링크인지 여부
        std::optional<std::chrono::system_clock::time_point> mt = e.mtime; // 메타데이터 시간 (없을 수도 있음)
        std::optional<std::uintmax_t> sz = e.size; // 메타데이터 크기 (없을 수도 있음)
        std::chrono::system_clock::time_point ft = e.found_time; // 발견 시각 

        // 유틸리티 스태틱 메서드 테스트
        bool exists_flag = mcfile::file_finder::exists_now(e.path); // 현재 시점에 존재하는지 e.path 파일이 존재하는지 여부
        if (e.mtime.has_value()) {
            std::time_t tt = mcfile::file_finder::to_time_t(*e.mtime); // 메타 데이터 시간 -> time_t 변환
        }
        mcfile::file_finder::print_entry_safe(e, std::cout); // cout으로 파일 찾은 결과 출력
    }

    //-----------------------------------------------
    // 7-6. file_finder.hpp 의 글로벌 exists() 오버로딩 4가지 모두 테스트

    // mcfile::exists 인자:
    // (1) 찾으려는 경로, (2) 찾으려는 파일 이름, (3) 재귀 탐색 여부, (4) 대소문자 구분 여부

    // (1) string, (2) string
    bool ex1 = mcfile::exists(root_dir.string(), std::string("test_a.cpp"), true, true);
    mcsp::println("[1] exists(test_a.cpp) [string, string] 테스트 결과: {}", ex1);

    // (1) path, (2) string
    bool ex2 = mcfile::exists(root_dir, std::string("test_a.cpp"), true, true);
    mcsp::println("[2] exists(test_a.cpp) [path, string] 테스트 결과: {}", ex2);

    // (1) string, (2) path
    bool ex3 = mcfile::exists(root_dir.string(), std::filesystem::path("test_a.cpp"), true, true);
    mcsp::println("[3] exists(test_a.cpp) [string, path] 테스트 결과: {}", ex3);

    // (1) path, (2) path
    bool ex4 = mcfile::exists(root_dir, std::filesystem::path("test_a.cpp"), true, true);
    mcsp::println("[4] exists(test_a.cpp) [path, path] 테스트 결과: {}", ex4);

    return true; 
}
