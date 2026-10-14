#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_executable_path()
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    using exec_path_options = mcfile::exec_path_options;
    using name_type = mcfile::name_type;

    // 1-1. exec_path_options 구조체 멤버 전체 활용

    exec_path_options opt{}; // 실행 파일 경로 조회 동작을 제어하는 옵션

    opt.resolve_symlink = true; // 심볼릭 링크를 실제 경로로 변환

    opt.prefer_realpath_on_linux_deleted = true;
    // Linux에서 삭제된 실행 파일의 경로를 realpath()로 우선 시도

    opt.windows_keep_device_prefix = false;
    // Windows에서 \\?\ 접두사 제거

    std::error_code ec;

    // 1-2. executable_path() 모든 오버로딩 테스트
    std::filesystem::path p1 = mcfile::executable_path(ec, opt);
    std::filesystem::path p2 = mcfile::executable_path(opt);

    mcsp::println("실행 파일 경로 (path) p1: " + p1.string());
    mcsp::println("실행 파일 경로 (path) p2: " + p2.string());

    // 1-3. executable_dir() 모든 오버로딩 테스트
    std::filesystem::path d1 = mcfile::executable_dir(ec, opt);
    std::filesystem::path d2 = mcfile::executable_dir(opt);

    mcsp::println("실행 디렉터리 (path) d1: " + d1.string());
    mcsp::println("실행 디렉터리 (path) d2: " + d2.string());

    // 1-4. executable_path_string() 및 executable_path_wstring() 모든 오버로딩 테스트
    std::string ps1 = mcfile::executable_path_string(ec, opt);
    std::string ps2 = mcfile::executable_path_string(opt);

    mcsp::println("실행 파일 경로 (string) ps1: " + ps1);
    mcsp::println("실행 파일 경로 (string) ps2: " + ps2);

    std::wstring pws1 = mcfile::executable_path_wstring(ec, opt);
    std::wstring pws2 = mcfile::executable_path_wstring(opt);

    // 1-5. executable_dir_string() 및 executable_dir_wstring() 모든 오버로딩 테스트
    std::string ds1 = mcfile::executable_dir_string(ec, opt);
    std::string ds2 = mcfile::executable_dir_string(opt);

    mcsp::println("실행 디렉터리 (string) ds1: " + ds1);
    mcsp::println("실행 디렉터리 (string) ds2: " + ds2);

    std::wstring dws1 = mcfile::executable_dir_wstring(ec, opt);
    std::wstring dws2 = mcfile::executable_dir_wstring(opt);

    // 1-6. executable_name.hpp 의 name_type enum 및 모든 executable_name/executable_name_w 오버로딩 테스트
    std::string name_fn_ec = mcfile::executable_name(ec, name_type::filename, opt);
    std::string name_fn = mcfile::executable_name(name_type::filename, opt);

    mcsp::println("실행 파일 이름 (filename) name_fn_ec: " + name_fn_ec);
    mcsp::println("실행 파일 이름 (filename) name_fn: " + name_fn);

    std::string name_stem_ec = mcfile::executable_name(ec, name_type::stem, opt);
    std::string name_stem = mcfile::executable_name(name_type::stem, opt);

    mcsp::println("실행 파일 이름 (stem) name_stem_ec: " + name_stem_ec);
    mcsp::println("실행 파일 이름 (stem) name_stem: " + name_stem);

    std::wstring wname_fn_ec = mcfile::executable_name_w(ec, name_type::filename, opt);
    std::wstring wname_fn = mcfile::executable_name_w(name_type::filename, opt);

    std::wstring wname_stem_ec = mcfile::executable_name_w(ec, name_type::stem, opt);
    std::wstring wname_stem = mcfile::executable_name_w(name_type::stem, opt);

    return true; 
}
