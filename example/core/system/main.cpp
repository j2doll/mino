#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

extern void test_command_line();
extern void test_device_id_generator();
extern void test_resource_monitor();
extern void test_process_runner();
extern void test_crash_handler();

// ==========================================
// main 함수
// ==========================================
int main() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce(" mino::core::system 단위 테스트 시작"));
    print(tce("========================================\n"));

    test_command_line();
    test_device_id_generator();
    test_resource_monitor();
    test_process_runner();
    test_crash_handler();

    print(tce("========================================"));
    print(tce(" 모든 테스트가 성공적으로 통과되었습니다."));
    print(tce("========================================"));

    return 0;
}
