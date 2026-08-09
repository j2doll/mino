#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>
#include <chrono>
#include <optional>
#include <regex>

// 프로젝트 헤더 포함
#include "mino/core/file/file.hpp"
#include "mino/core/string/to_console_encoding.hpp"

namespace {
    // 콘솔 인코딩 변환 출력 헬퍼 함수
    void print_out(const std::string& msg) {
        std::cout << mino::core::string::to_console_encoding(msg) << std::endl;
    }

    void print_err(const std::string& msg) {
        std::cerr << mino::core::string::to_console_encoding(msg) << std::endl;
    }
}

int main() {
    namespace mcfile = mino::core::file;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    print_out("==================================================");
    print_out("[1] executable_path.hpp & executable_name.hpp 테스트");
    print_out("==================================================");

    // 1-1. exec_path_options 구조체 멤버 전체 활용
    mcfile::exec_path_options opt{};
    opt.resolve_symlink = true;
    opt.prefer_realpath_on_linux_deleted = true;
    opt.windows_keep_device_prefix = false;

    std::error_code ec;

    // 1-2. executable_path() 모든 오버로딩 테스트
    std::filesystem::path p1 = mcfile::executable_path(ec, opt);
    std::filesystem::path p2 = mcfile::executable_path(opt);

    // 1-3. executable_dir() 모든 오버로딩 테스트
    std::filesystem::path d1 = mcfile::executable_dir(ec, opt);
    std::filesystem::path d2 = mcfile::executable_dir(opt);

    // 1-4. executable_path_string() 및 executable_path_wstring() 모든 오버로딩 테스트
    std::string ps1 = mcfile::executable_path_string(ec, opt);
    std::string ps2 = mcfile::executable_path_string(opt);
    std::wstring pws1 = mcfile::executable_path_wstring(ec, opt);
    std::wstring pws2 = mcfile::executable_path_wstring(opt);

    // 1-5. executable_dir_string() 및 executable_dir_wstring() 모든 오버로딩 테스트
    std::string ds1 = mcfile::executable_dir_string(ec, opt);
    std::string ds2 = mcfile::executable_dir_string(opt);
    std::wstring dws1 = mcfile::executable_dir_wstring(ec, opt);
    std::wstring dws2 = mcfile::executable_dir_wstring(opt);

    // 1-6. executable_name.hpp 의 name_type enum 및 모든 executable_name/executable_name_w 오버로딩 테스트
    std::string name_fn_ec = mcfile::executable_name(ec, mcfile::name_type::filename, opt);
    std::string name_fn = mcfile::executable_name(mcfile::name_type::filename, opt);
    std::string name_stem_ec = mcfile::executable_name(ec, mcfile::name_type::stem, opt);
    std::string name_stem = mcfile::executable_name(mcfile::name_type::stem, opt);

    std::wstring wname_fn_ec = mcfile::executable_name_w(ec, mcfile::name_type::filename, opt);
    std::wstring wname_fn = mcfile::executable_name_w(mcfile::name_type::filename, opt);
    std::wstring wname_stem_ec = mcfile::executable_name_w(ec, mcfile::name_type::stem, opt);
    std::wstring wname_stem = mcfile::executable_name_w(mcfile::name_type::stem, opt);

    print_out("실행 파일 경로 (path): " + p1.string());
    print_out("실행 디렉터리 (path): " + d1.string());
    print_out("실행 파일 이름 (filename): " + name_fn);
    print_out("실행 파일 이름 (stem): " + name_stem);


    print_out("\n==================================================");
    print_out("[2] path.hpp (path_from_utf8) 테스트");
    print_out("==================================================");

    std::string utf8_path_str = "test_dir_한글경로_유니코드/file.txt";
    std::filesystem::path converted_path = mcfile::path_from_utf8(utf8_path_str);
    auto converted_path_str = converted_path.string(); // OS 캐릭터셋 문자열
    print_out("path_from_utf8 변환 결과: ");
    std::cout << converted_path_str << std::endl;


    print_out("\n==================================================");
    print_out("[3] 테스트용 임시 디렉터리 및 파일 생성");
    print_out("==================================================");

    std::filesystem::path root_dir = std::filesystem::current_path() / "mino_test_workspace";
    std::filesystem::create_directories(root_dir, ec);
    if (ec) {
        print_err("임시 작업 디렉터리 생성 실패: " + ec.message());
        return 1;
    }
    else {
        print_out("임시 작업 디렉터리 생성 성공: ");
        std::cout << root_dir.string() << std::endl;
    }

    std::filesystem::path sub_file1 = root_dir / "test_a.cpp";
    std::filesystem::path sub_file2 = root_dir / "test_b.txt";

    {
        std::ofstream ofs(sub_file1);
        ofs << "// C++ Source code sample\nint main() { return 0; }";
    }
    {
        std::ofstream ofs(sub_file2);
        ofs << "Text file sample data";
    }


    print_out("\n==================================================");
    print_out("[4] file_info 클래스 전체 멤버 메서드 테스트");
    print_out("==================================================");

    mcfile::file_info finfo(sub_file1.string());

    print_out("exists()        : " + std::string(finfo.exists() ? "true" : "false"));
    print_out("is_file()       : " + std::string(finfo.is_file() ? "true" : "false"));
    print_out("is_dir()        : " + std::string(finfo.is_dir() ? "true" : "false"));
    print_out("file_name()     : " + finfo.file_name());
    print_out("extension()     : " + finfo.extension());
    print_out("parent_path()   : " + finfo.parent_path());
    print_out("absolute_path() : " + finfo.absolute_path());
    print_out("file_size()     : " + std::to_string(finfo.file_size()) + " bytes");


    print_out("\n==================================================");
    print_out("[5] permissions_info 클래스 및 perms 구조체 멤버 테스트");
    print_out("==================================================");

    mcfile::permissions_info pinfo(sub_file1.string());

    print_out("pinfo.exists()  : " + std::string(pinfo.exists() ? "true" : "false"));
    print_out("pinfo.to_string(): " + pinfo.to_string());

    // perms 구조체 멤버 (read, write, execute) 사용
    mcfile::perms owner_p = pinfo.for_owner();
    mcfile::perms group_p = pinfo.for_group();
    mcfile::perms others_p = pinfo.for_others();

    print_out("Owner  - R:" + std::to_string(owner_p.read) + " W:" + std::to_string(owner_p.write) + " X:" + std::to_string(owner_p.execute));
    print_out("Group  - R:" + std::to_string(group_p.read) + " W:" + std::to_string(group_p.write) + " X:" + std::to_string(group_p.execute));
    print_out("Others - R:" + std::to_string(others_p.read) + " W:" + std::to_string(others_p.write) + " X:" + std::to_string(others_p.execute));


    print_out("\n==================================================");
    print_out("[6] file_size_formatter 클래스 및 metric_type enum 테스트");
    print_out("==================================================");

    // 6-1. byte_to_string() 테스트 (metric_type::iec 및 metric_type::si)
    std::string sz_iec = mcfile::file_size_formatter::byte_to_string(2048, mcfile::metric_type::iec, 2);
    std::string sz_si = mcfile::file_size_formatter::byte_to_string(2048, mcfile::metric_type::si, 2);
    print_out("2048 B (IEC): " + sz_iec);
    print_out("2048 B (SI) : " + sz_si);

    // 6-2. string_to_bytes() 테스트
    std::optional<uint64_t> bytes_opt = mcfile::file_size_formatter::string_to_bytes("1.5 MB", mcfile::metric_type::iec);
    if (bytes_opt.has_value()) {
        print_out("'1.5 MB' -> " + std::to_string(*bytes_opt) + " bytes");
    }

    // 6-3. get_filesize_string() 테스트
    std::optional<std::string> fs_str_opt = mcfile::file_size_formatter::get_filesize_string(sub_file1, mcfile::metric_type::iec, 2);
    if (fs_str_opt.has_value()) {
        print_out("sub_file1 용량: " + *fs_str_opt);
    }


    print_out("\n==================================================");
    print_out("[7] file_finder 클래스, 구조체, 스태틱/글로벌 함수 전체 테스트");
    print_out("==================================================");

    // 7-1. file_finder::sort_key enum 멤버 사용
    mcfile::file_finder::sort_key sk_none = mcfile::file_finder::sort_key::none;
    mcfile::file_finder::sort_key sk_path = mcfile::file_finder::sort_key::path;
    mcfile::file_finder::sort_key sk_name = mcfile::file_finder::sort_key::name;
    mcfile::file_finder::sort_key sk_mtime = mcfile::file_finder::sort_key::mtime;
    mcfile::file_finder::sort_key sk_size = mcfile::file_finder::sort_key::size;

    // 7-2. file_finder::finder_options 구조체의 모든 멤버 변수 설정
    mcfile::file_finder::finder_options fopt;
    fopt.recursive = true;
    fopt.follow_symlinks = false;
    fopt.max_depth = -1;
    fopt.skip_permission_denied = true;
    fopt.stop_on_error = false;

    fopt.include_files = true;
    fopt.include_dirs = true;
    fopt.include_symlinks_as_items = false;

    fopt.extensions = { ".cpp", ".txt" };
    fopt.include_globs = { "*" };
    fopt.exclude_globs = { "*.tmp" };
    fopt.exclude_dir_globs = { ".git", "build*" };
    fopt.ext_case_sensitive = false;
    fopt.include_hidden = true;
    fopt.include_system = true;
    fopt.name_regex = std::regex(".*");

    fopt.mtime_since = std::chrono::system_clock::now() - std::chrono::hours(24);
    fopt.mtime_until = std::chrono::system_clock::now() + std::chrono::hours(24);
    fopt.min_size = 0;
    fopt.max_size = 1024 * 1024 * 10; // 10MB
    fopt.compute_size = true;

    fopt.limit_results = 100;
    fopt.sort_by = sk_name;
    fopt.sort_ascending = true;

    // 방문자(on_visit) 콜백 테스트
    fopt.on_visit = [](const mcfile::file_finder::entry& e) -> bool {
        // true 반환 시 탐색 계속
        return true;
        };

    // 7-3. 정적 메서드 file_finder::find() 호출
    std::vector<mcfile::file_finder::entry> static_results = mcfile::file_finder::find(root_dir, fopt);
    print_out("정적 find() 로 찾은 항목 수: " + std::to_string(static_results.size()));

    // 7-4. 인스턴스 생성 및 메서드 테스트 (생성자, set_options, options, find, last_errors, clear_errors)
    mcfile::file_finder finder_inst; // 기본 생성자
    mcfile::file_finder finder_inst_opt(fopt); // 옵션 생성자

    finder_inst.set_options(fopt);
    const mcfile::file_finder::finder_options& current_opts = finder_inst.options();

    // on_visit 을 null로 비워서 결과 벡터를 수집
    fopt.on_visit = nullptr;
    finder_inst.set_options(fopt);

    std::vector<mcfile::file_finder::entry> inst_results = finder_inst.find(root_dir);
    print_out("인스턴스 find() 로 찾은 항목 수: " + std::to_string(inst_results.size()));

    // 에러 관리 메서드
    const std::vector<std::string>& errs = finder_inst.last_errors();
    print_out("마지막 에러 수: " + std::to_string(errs.size()));
    finder_inst.clear_errors();

    // 7-5. file_finder::entry 구조체의 모든 멤버 변수 접근 및 유틸리티 메서드 테스트
    for (const mcfile::file_finder::entry& e : inst_results) {
        // entry 멤버 직접 접근
        std::filesystem::path p = e.path;
        bool is_f = e.is_file;
        bool is_d = e.is_dir;
        bool is_sym = e.is_symlink;
        std::optional<std::chrono::system_clock::time_point> mt = e.mtime;
        std::optional<std::uintmax_t> sz = e.size;
        std::chrono::system_clock::time_point ft = e.found_time;

        // 유틸리티 스태틱 메서드 테스트
        bool exists_flag = mcfile::file_finder::exists_now(e.path);
        if (e.mtime.has_value()) {
            std::time_t tt = mcfile::file_finder::to_time_t(*e.mtime);
        }
        mcfile::file_finder::print_entry_safe(e, std::cout);
    }

    // 7-6. file_finder.hpp 의 글로벌 exists() 오버로딩 4가지 모두 테스트
    bool ex1 = mcfile::exists(root_dir.string(), std::string("test_a.cpp"), true, true);
    bool ex2 = mcfile::exists(root_dir, std::string("test_a.cpp"), true, true);
    bool ex3 = mcfile::exists(root_dir.string(), std::filesystem::path("test_a.cpp"), true, true);
    bool ex4 = mcfile::exists(root_dir, std::filesystem::path("test_a.cpp"), true, true);

    print_out("exists() 테스트 결과 (1~4): " +
        std::to_string(ex1) + " " + std::to_string(ex2) + " " +
        std::to_string(ex3) + " " + std::to_string(ex4));


    print_out("\n==================================================");
    print_out("[8] 임시 파일 cleanup");
    print_out("==================================================");

    std::filesystem::remove_all(root_dir, ec);
    if (!ec) {
        print_out("임시 작업 디렉터리가 정상적으로 삭제되었습니다.");
    }

    print_out("==================================================");
    print_out("모든 public 멤버 테스트 완료!");
    print_out("==================================================");

    return 0;
}
