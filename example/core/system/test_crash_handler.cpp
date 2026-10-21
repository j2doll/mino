#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 5. crash_handler 테스트 (초기화 및 스택 트레이스)
// ==========================================
void test_crash_handler() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    namespace mcsys = mino::core::system;
    using crash_handler = mcsys::crash_handler;

    print(tce("[TEST] 5. crash_handler 테스트 시작"));

    // 초기화 함수 호출 검증 (커스텀 콜백 등록)
    crash_handler::initialize([](const std::string& crash_log) {
        const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
        auto tce = mino::core::string::to_console_encoding;
        eprint(tce("[CALLBACK] Crash occurred with log:\n"), tce(crash_log));
    });

    // 현재 위치에서의 콜 스택 트레이스 수집 검증
    std::vector<std::string> stack_trace = crash_handler::get_stack_trace(); // 스택 트레이스 수집
    print(tce("  -> 현재 콜 스택 프레임 수: "), std::to_string(stack_trace.size()));
    for (size_t i = 0; i < stack_trace.size(); ++i) {
        print(tce("     "), tce(stack_trace[i]));
    }
    assert(!stack_trace.empty() && "Stack trace should not be empty");

    print(tce("[PASS] crash_handler 테스트 완료\n"));
}

