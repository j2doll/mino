#include <iostream>
#include <cassert>

#include "mino/core/enum/enum.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 1. 네임스페이스 포함 열거형 정의 테스트 (자동 할당 및 값 명시 mixed)
DEFINE_ENUM_NAMESPACE(game, \
    Status, \
    Idle = 0, Running, Paused = 10, Stopped)
    // 네임스페이스 game 인 Status 열거형(game::Status) 정의

// 2. 전역 열거형 정의 테스트 (음수 및 명시적 정수 값)
DEFINE_ENUM_GLOBAL(Color, \
    Red = -1, Green, Blue = 5)
    // 전역 Color 열거형 정의

int main() { 
    namespace enums = mino::core::enums;

    // --------------------------------------------------
    // Test 1: game::Status (네임스페이스 및 파싱 동작)
    // --------------------------------------------------
    std::cout << "=== Test 1: game::Status ===" << std::endl;

    // enum_name 테스트 (열거형 값 -> 문자열)
    std::cout << "game::Status::Idle   -> " << enums::enum_name(game::Status::Idle) << std::endl;
    std::cout << "game::Status::Running -> " << enums::enum_name(game::Status::Running) << std::endl;
    std::cout << "game::Status::Paused  -> " << enums::enum_name(game::Status::Paused) << std::endl;
    std::cout << "game::Status::Stopped -> " << enums::enum_name(game::Status::Stopped) << std::endl;
    // 출력 결과
    // game::Status::Idle   -> Idle
    // game::Status::Running -> Running
    // game::Status::Paused  -> Paused
    // game::Status::Stopped -> Stopped

    // enum_cast (문자열 -> enum 값) 성공 테스트
    auto status_opt = enums::enum_cast<game::Status>("Paused");
    if (status_opt.has_value()) {
        std::cout
            << "cast('Paused') -> Match game::Status::Paused: "
            << (status_opt.value() == game::Status::Paused ? "true" : "false")
            << std::endl;
    }
    // 출력 결과
    // cast('Paused') -> Match game::Status::Paused: true

    // enum_cast 실패 테스트 (존재하지 않는 이름)
    auto invalid_opt = enums::enum_cast<game::Status>("Unknown"); // Unknown은 정의되지 않은 이름
    std::cout
        << "cast('Unknown') has_value: "
        << (invalid_opt.has_value() ? "true" : "false")
        << std::endl;
    // 출력 결과
    // cast('Unknown') has_value: false

    // enum_entries 전체 순회 테스트
    std::cout << "\n[game::Status Entries List]" << std::endl;
    for (const auto& entry : enums::enum_entries<game::Status>()) {
        std::cout
            << "  Name: " << entry.name
            << " | Value: " << static_cast<int>(entry.value)
            << std::endl;
    }
    // 출력 결과
    // [game::Status Entries List]
    //   Name: Idle | Value: 0
    //   Name: Running | Value: 1
    //   Name: Paused | Value: 10
    //   Name: Stopped | Value: 11

    // --------------------------------------------------
    // Test 2: Color (전역 열거형 문자열 및 숫자 테스트)
    // --------------------------------------------------
    std::cout << "\n=== Test 2: Color ===" << std::endl;

    std::cout << "Color::Red   -> name: " << enums::enum_name(Color::Red)
        << " | value: " << static_cast<int>(Color::Red) << std::endl;
    std::cout << "Color::Green -> name: " << enums::enum_name(Color::Green)
        << " | value: " << static_cast<int>(Color::Green) << std::endl;
    std::cout << "Color::Blue  -> name: " << enums::enum_name(Color::Blue)
        << " | value: " << static_cast<int>(Color::Blue) << std::endl;
    // 출력 결과
    // Color::Red   -> name: Red | value: -1
    // Color::Green -> name: Green | value: 0
    // Color::Blue  -> name: Blue | value: 5

    // 단증(assert) 검증
    assert(enums::enum_name(Color::Red) == "Red"); // 문자열 검증
    assert(enums::enum_cast<Color>("Blue") == Color::Blue); // 문자열 -> enum 검증
    assert(static_cast<int>(Color::Green) == 0); // Red가 -1이므로 Green은 0으로 자동 증가

    std::cout << "\nAll assertion tests passed successfully!" << std::endl;

    return 0;
}
