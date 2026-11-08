#include <fstream>
#include <filesystem>
#include <cassert>
 
#include "mino/core/directory/directory.hpp"
#include "mino/core/string/string.hpp"

namespace fs = std::filesystem;
using namespace mino::core::directory;
namespace prn = mino::core::string::print;

void print_result(const std::string& test_name, const create_dir_result& res) {
    prn::println("=== [{}] ===", test_name);
    prn::println("Target    : {}", res.requested.string());

    if (res.success) {
        prn::println("Success   : true");
    }
    else {
        prn::eprintln("Success   : false");
    }

    prn::println("Code      : {}", directory_maker::to_string(res.code));

    // res.message는 이미 to_console_encoding()이 적용된 콘솔 바이트열이므로,
    // UTF-8을 기대하는 format_to에 넘길 때는 from_console_encoding으로 복원하여 넘깁니다.
    prn::println("Message   : {}", mino::core::string::from_console_encoding(res.message));

    prn::println("Created # : {}", res.created.size());
    for (const auto& p : res.created) {
        prn::println("  - {}", p.string());
    }
    prn::println("");
}
 
int main() {
    const fs::path sandbox = fs::current_path() / "test_sandbox";

    // 0. 이전 테스트 잔여물 정리
    std::error_code ec;
    fs::remove_all(sandbox, ec);

    // 기본 생성자 (한국어 메시지, 기본 옵션: make_parents = true, succeed_if_exists = true)
    auto lang = language::Korean;
    directory_maker maker(lang);

    // -------------------------------------------------------------
    // Test 1: 빈 경로 입력 검증
    // -------------------------------------------------------------
    {
        auto res = maker.create_directory_tree("");
        print_result("Test 1: Empty Path", res);
        assert(!res.success);
        assert(res.code == create_path_code::invalid_path);
    }

    // -------------------------------------------------------------
    // Test 2: 다단계 디렉터리 신규 생성 (make_parents = true)
    // -------------------------------------------------------------
    const fs::path deep_path = sandbox / "a" / "b" / "c";
    {
        auto res = maker.create_directory_tree(deep_path);
        print_result("Test 2: Create Deep Tree", res);
        assert(res.success);
        assert(res.code == create_path_code::created);
        assert(fs::is_directory(deep_path));
    }

    // -------------------------------------------------------------
    // Test 3: 이미 존재하는 디렉터리 (succeed_if_exists = true)
    // -------------------------------------------------------------
    {
        auto res = maker.create_directory_tree(deep_path);
        print_result("Test 3: Already Exists (Allow)", res);
        assert(res.success);
        assert(res.code == create_path_code::already_exists);
    }

    // -------------------------------------------------------------
    // Test 4: 이미 존재하는 디렉터리 (succeed_if_exists = false)
    // -------------------------------------------------------------
    {
        create_dir_options opt = maker.options();
        opt.succeed_if_exists = false;
        maker.set_options(opt);

        auto res = maker.create_directory_tree(deep_path);
        print_result("Test 4: Already Exists (Strict Error)", res);
        assert(!res.success);
        assert(res.code == create_path_code::already_exists);

        // 옵션 복원
        opt.succeed_if_exists = true;
        maker.set_options(opt);
    }

    // -------------------------------------------------------------
    // Test 5: 부모 디렉터리 미생성 옵션 (make_parents = false)
    // -------------------------------------------------------------
    {
        create_dir_options opt = maker.options();
        opt.make_parents = false;
        maker.set_options(opt);

        // 부모(non_existent)가 없으므로 실패해야 함
        const fs::path invalid_parent_path = sandbox / "non_existent" / "leaf";
        auto res = maker.create_directory_tree(invalid_parent_path);
        print_result("Test 5: No Parents Option (Fail Expected)", res);
        assert(!res.success);
        assert(res.code == create_path_code::no_such_parent);

        // 옵션 복원
        opt.make_parents = true;
        maker.set_options(opt);
    }

    // -------------------------------------------------------------
    // Test 6: 경로 상에 디렉터리가 아닌 일반 파일이 위치한 경우
    // -------------------------------------------------------------
    {
        const fs::path file_path = sandbox / "regular_file.txt";
        {
            std::ofstream ofs(file_path);
            ofs << "dummy content";
        }

        // 일반 파일 하위에 디렉터리 생성을 시도
        const fs::path conflict_path = file_path / "sub_dir";
        auto res = maker.create_directory_tree(conflict_path);
        print_result("Test 6: Not a Directory Conflict", res);
        assert(!res.success);
        assert(res.code == create_path_code::not_directory);
    }

    // -------------------------------------------------------------
    // Test 7: 영어 메시지 출력 확인
    // -------------------------------------------------------------
    {
        maker.set_language(language::English);
        auto res = maker.create_directory_tree("");
        print_result("Test 7: English Message Check", res);
        assert(res.message.find("Invalid path") != std::string::npos);
    }

    // -------------------------------------------------------------
    // 테스트 종료 후 샌드박스 정리
    // -------------------------------------------------------------
    fs::remove_all(sandbox, ec);
    prn::println("모든 테스트가 성공적으로 통과되었습니다.");

    return 0;
}
