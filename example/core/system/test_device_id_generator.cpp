#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 2. device_id_generator 테스트
// ==========================================
void test_device_id_generator() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    namespace mcsys = mino::core::system;
    using device_id_generator = mcsys::device_id_generator;

    print(tce("[TEST] 2. device_id_generator 테스트 시작"));

    std::string id1 = device_id_generator::get_unique_id(); // 장비 고유 ID 생성
    std::string id2 = device_id_generator::get_unique_id();

    print(tce("  -> 생성된 Device ID: "), id1,
        tce(" (길이: "), std::to_string(id1.length()), tce(")"));

    // 64자리 16진수 문자열인지 검증
    assert(id1.length() == 64 && "Device ID must be 64 characters long");
    assert(id1 == id2 && "Consecutive get_unique_id calls on the same machine must match");

    print(tce("[PASS] device_id_generator 테스트 완료\n"));
}
