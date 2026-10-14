#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>
#include <chrono>
#include <optional>
#include <regex>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp" 

extern bool test_executable_path();
extern bool test_path_from_utf8();
extern bool test_create_temp_dir(std::filesystem::path root_dir,
    std::filesystem::path sub_file1, std::filesystem::path sub_file2);
extern bool test_file_watcher(std::filesystem::path target);
extern bool test_finfo(std::filesystem::path sub_file1);
extern bool test_permissions_info(std::filesystem::path sub_file1);
extern bool test_file_size_formatter(std::filesystem::path sub_file1);
extern bool test_file_finder(std::filesystem::path root_dir); 

int main() {
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    mcsp::println("==================================================");
    mcsp::println("[1] executable_path.hpp & executable_name.hpp 테스트");
    mcsp::println("==================================================");

    assert(test_executable_path());

    mcsp::println("\n==================================================");
    mcsp::println("[2] path.hpp (path_from_utf8) 테스트");
    mcsp::println("==================================================");

    assert(test_path_from_utf8());

    mcsp::println("\n==================================================");
    mcsp::println("[3] 테스트용 임시 디렉터리 및 파일 생성");
    mcsp::println("==================================================");

    std::filesystem::path root_dir = std::filesystem::current_path() / "mino_test_workspace";
    std::filesystem::path sub_file1 = root_dir / "test_a.cpp";
    std::filesystem::path sub_file2 = root_dir / "test_b.txt";

    assert(test_create_temp_dir(root_dir, sub_file1, sub_file2));

    mcsp::println("\n==================================================");
    mcsp::println("[4] file_info 클래스 전체 멤버 메서드 테스트");
    mcsp::println("==================================================");

    assert(test_finfo(sub_file1));

    mcsp::println("\n==================================================");
    mcsp::println("[5] permissions_info 클래스 및 perms 구조체 멤버 테스트");
    mcsp::println("==================================================");

    assert(test_permissions_info(sub_file1));

    mcsp::println("\n==================================================");
    mcsp::println("[6] file_size_formatter 클래스 및 metric_type enum 테스트");
    mcsp::println("==================================================");

    assert(test_file_size_formatter(sub_file1));

    mcsp::println("\n==================================================");
    mcsp::println("[7] file_finder 클래스, 구조체, 스태틱/글로벌 함수 전체 테스트");
    mcsp::println("==================================================");

    assert(test_file_finder(root_dir));

    // 8. file_watcher.hpp 테스트
    mcsp::println("\n==================================================");
    mcsp::println("[8] file_watcher.hpp 테스트");
    mcsp::println("==================================================");

    std::filesystem::path file_watcher_target = root_dir;
    assert(test_file_watcher(file_watcher_target));

    mcsp::println("\n==================================================");
    mcsp::println("[99] 임시 파일 cleanup");
    mcsp::println("==================================================");

    std::error_code ec;
    std::filesystem::remove_all(root_dir, ec);
    if (!ec) {
        mcsp::println("임시 작업 디렉터리가 정상적으로 삭제되었습니다.");
    } else {
        mcsp::eprintln("임시 작업 디렉터리 삭제 중 오류 발생: {}", ec.message());
    }

    mcsp::println("==================================================");
    mcsp::println("모든 public 멤버 테스트 완료!");
    mcsp::println("==================================================");

    return 0;
}
