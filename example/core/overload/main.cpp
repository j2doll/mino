#include <iostream>
#include <variant>
#include <string>

#include "mino/core/overload/overload.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    namespace mco = mino::core::overload;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    // 1. 테스트용 std::variant 타입 정의
    using VarType = std::variant<int, double, std::string>;

    // 2. overload 패턴을 활용한 방문자(Visitor) 객체 생성
    auto visitor = mco::overload{
        [](int i) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("[int 처리]: ") << i << '\n';
        },
        [](double d) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("[double 처리]: ") << d << '\n';
        },
        [](const std::string& s) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("[string 처리]: ") << to_console_encoding(s) << '\n';
        }
    };

    // 3. 다양한 타입의 variant 값 준비
    VarType v1 = 42; // int
    VarType v2 = 3.14159; // double
    VarType v3 = std::string("Hello, overload!"); // std::string

    std::cout << to_console_encoding("=== std::visit 테스트 ===") << std::endl;
    std::visit(visitor, v1); // int로 처리
    std::visit(visitor, v2); // double로 처리
    std::visit(visitor, v3); // std::string로 처리

    std::cout << to_console_encoding("\n=== 직접 호출 테스트 ===") << std::endl;
    visitor(100); // 직접 호출: int 처리
    visitor(2.718); // 직접 호출: double 처리
    visitor("Direct Call"); // 직접 호출: std::string 처리

    // 4. std::visit 안에서 즉석(In-place)으로 overload 생성하여 사용하기
    std::cout << to_console_encoding("\n=== Inline std::visit 테스트 ===") << std::endl;

    VarType v4 = 999; // int 타입의 variant 값
    std::visit(mco::overload{
        [](int i) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("Inline int: ") << i << '\n';
        },
        [](auto&& arg) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("기타 타입 처리\n");
        } // generic lambda
    }, v4);

    // =========================================================================
    // 5. 반환 값 처리 테스트 (동일한 타입 반환)
    // =========================================================================
    std::cout
        << to_console_encoding("\n=== 반환 값 처리 테스트 (std::string 반환) ===") << std::endl;

    auto stringifier = mco::overload{
        [](int i) -> std::string {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            return to_console_encoding("[int 반환값]: ") + std::to_string(i * 2);
        },
        [](double d) -> std::string {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            return to_console_encoding("[double 반환값]: ") + std::to_string(d + 1.0);
        },
        [](const std::string& s) -> std::string {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            return to_console_encoding("[string 반환값]: ") + to_console_encoding(s) + " (processed)";
        }
    };

    std::string res1 = std::visit(stringifier, v1); // int 처리 후 std::string 반환
    std::string res2 = std::visit(stringifier, v2); // double 처리 후 std::string 반환
    std::string res3 = std::visit(stringifier, v3); // std::string 처리 후 std::string 반환

    std::cout << res1 << '\n'; // [int 반환값]: 84
    std::cout << res2 << '\n'; // [double 반환값]: 4.14159
    std::cout << res3 << '\n'; // [string 반환값]: Hello, overload! (processed)

    // =========================================================================
    // 6. 반환 값 처리 테스트 (서로 다른 타입 반환 - std::variant 활용)
    // =========================================================================
    std::cout << to_console_encoding("\n=== 반환 값 처리 테스트 (variant 반환) ===") << std::endl;

    using ResultType = std::variant<int, std::string>;

    auto process_and_convert = mco::overload{
        [](int i) -> ResultType {
            return i * 10; // ResultType 내의 int로 저장됨
        },
        [](double d) -> ResultType {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            return to_console_encoding("double에서 변환됨: ") + std::to_string(d); // ResultType 내의 string으로 저장됨
        },
        [](const std::string& s) -> ResultType {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            return to_console_encoding(s) + " -> Result"; // ResultType 내의 string으로 저장됨
        }
    };

    ResultType r1 = std::visit(process_and_convert, v1); // v1이 VarType(int)이므로 int로 처리되고, r1은 ResultType(int)로 저장됨
    if (std::holds_alternative<int>(r1)) {
        int int_r1 = std::get<int>(r1); // r1에서 int 값 추출 가능
        std::cout
            << to_console_encoding("r1에서 추출된 int 값: ")
            << int_r1 << '\n';
    }

    ResultType r2 = std::visit(process_and_convert, v2); // v2이 VarType(double)이므로 double로 처리되고, r2는 ResultType(string)로 저장됨
    if (std::holds_alternative<std::string>(r2)) {
        std::string str_r2 = std::get<std::string>(r2); // r2에서 string 값 추출 가능
        std::cout
            << to_console_encoding("r2에서 추출된 string 값: ")
            << to_console_encoding(str_r2) << '\n';
    }

    // 반환받은 ResultType 결과 출력
    auto result_printer = mco::overload{
        [](int val) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("결과 (int): ") << val << '\n';
        },
        [](const std::string& val) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout << to_console_encoding("결과 (string): ") << val << '\n';
        }
    };

    std::visit(result_printer, r1); // r1은 int 타입이므로, 결과 (int): 420 출력
    std::visit(result_printer, r2); // r2는 string 타입이므로, 결과 (string): double에서 변환됨: 3.14159 출력

    return 0;
}
