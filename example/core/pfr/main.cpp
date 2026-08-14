#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <array>
#include <cassert>

#include "mino/core/pfr/pfr.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ----------------------------------------------------------------------------
// 테스트용 구조체 정의
// ----------------------------------------------------------------------------

// 1. 커스텀 필드 이름이 없는 기본 구조체
struct Student {
    int id;
    std::string name;
    double gpa;
};

// 2. mino_field_names()를 통해 커스텀 필드 이름을 제공하는 구조체
struct Point3D {
    float x;
    float y;
    float z;

    // 커스텀 필드 이름 설정
    static constexpr std::array<const char*, 3> mino_field_names() {
        return { "x", "y", "z" }; // 멤버 순서 대로 이름 지정
    }
};


// ----------------------------------------------------------------------------
// 메인 테스트 함수
// ----------------------------------------------------------------------------

int main() {
    namespace pfr = mino::core::pfr;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("========================================\n");
    std::cout << to_console_encoding(" 1. Member Count & get<I> 테스트\n");
    std::cout << to_console_encoding("========================================\n");

    Student s1{ 101, "Alice", 3.89 };

    // 1-1. 필드 개수 계산 검증
    constexpr size_t count = pfr::fields_count<Student>(); // 3
    std::cout
        << to_console_encoding("Student 필드 개수: ")
        << count
        << to_console_encoding(" (기댓값: 3)\n");
    assert(count == 3);

    // 1-2. get<I>를 통한 값 읽기
    std::cout
        << to_console_encoding("get<0>(s1): ")
        << pfr::get<0>(s1) // 첫번째 멤버 값: 101 
        << "\n";
    std::cout
        << to_console_encoding("get<1>(s1): ")
        << pfr::get<1>(s1) // 두번째 멤버 값: "Alice"
        << "\n";
    std::cout
        << to_console_encoding("get<2>(s1): ")
        << pfr::get<2>(s1) // 세번째 멤버 값: 3.89
        << "\n";

    // 1-3. get<I> 참조를 통한 값 수정
    pfr::get<1>(s1) = "Alice Smith";
    std::cout
        << to_console_encoding("get<1>을 통해 수정된 name: ")
        << to_console_encoding(s1.name) // "Alice Smith"
        << to_console_encoding("\n\n");

    std::cout << to_console_encoding("========================================\n");
    std::cout << to_console_encoding(" 2. structure_for_each 테스트\n");
    std::cout << to_console_encoding("========================================\n");

    std::cout << to_console_encoding("Student s1 멤버 순회 출력:\n");

    // 2-1. structure_for_each를 사용하여 s1의 각 멤버 값과 타입 출력
    pfr::structure_for_each(s1, [](auto&& val) {
        auto to_console_encoding = mino::core::string::to_console_encoding;
        std::cout
            << to_console_encoding("  - 값: ") 
            << val // 멤버 값
            << to_console_encoding(" (타입: ")
            << pfr::get_type_name<decltype(val)>() // 멤버 타입
            << to_console_encoding(")\n");
    });

    std::cout << "\n";

    std::cout << to_console_encoding("========================================\n");
    std::cout << to_console_encoding(" 3. get_type_name 테스트\n");
    std::cout << to_console_encoding("========================================\n");

    std::cout
        << to_console_encoding("int                            : ")
        << pfr::get_type_name<int>() // "int" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::string                    : ")
        << pfr::get_type_name<std::string>() // "string" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::vector<int>               : ")
        << pfr::get_type_name<std::vector<int>>() // "vector<int>" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::map<std::string, double>  : ")
        << pfr::get_type_name<std::map<std::string, double>>() // "map<string, double>" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::optional<float>           : ")
        << pfr::get_type_name<std::optional<float>>() // "optional<float>" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::variant<int, std::string> : ")
        << pfr::get_type_name<std::variant<int, std::string>>() // "variant<int, string>" 문자열
        << to_console_encoding("\n");
    std::cout
        << to_console_encoding("std::array<int, 5>             : ")
        << pfr::get_type_name<std::array<int, 5>>() // "array<int, 5>" 문자열
        << to_console_encoding("\n\n");


    std::cout << to_console_encoding("========================================\n");
    std::cout << to_console_encoding(" 4. structure_for_each_with_name 테스트\n");
    std::cout << to_console_encoding("========================================\n");

    // 4-1. 커스텀 필드 이름이 없는 경우 (자동 생성: field0, field1, ...)
    std::cout << to_console_encoding("[Student - 기본 인덱스 이름 적용]\n");

    pfr::structure_for_each_with_name(s1, [](const char* name, auto&& val) {
        std::cout << "  " << name << " = " << val << "\n";
    });
    // field0 = 101
    // field1 = Alice Smith
    // field2 = 3.89

    // 4-2. 커스텀 필드 이름이 지정된 경우 (x, y, z)
    std::cout << to_console_encoding("\n[Point3D - mino_field_names 적용]\n");

    Point3D pt{ 10.5f, 20.0f, -5.2f };
    pfr::structure_for_each_with_name(pt, [](const char* name, auto&& val) {
        std::cout << "  " << name << " = " << val << "\n";
    });
    // x = 10.5
    // y = 20
    // z = -5.2

    std::cout << to_console_encoding("\n모든 테스트가 성공적으로 완료되었습니다!\n");

    return 0;
}
