#include <iostream>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/exception/exception.hpp"

namespace {
    bool test() {
        namespace fs = ::std::filesystem;
        bool result = fs::create_directory("invalid/not/exist/path"); // 예외 발생
        return result;
    }
} // anonymous namespace

void test_traceable_exception() {
    namespace fs = ::std::filesystem;

    namespace mce = mino::core::exception;
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    auto tce = mcs::to_console_encoding;

    mcsp::println("================================");
    mcsp::println("   traceable_exception 테스트  ");
    mcsp::println("================================");
    mcsp::println("\n");

    //--------------------------------------------
    // [1] 일반적인 try-catch 블록 사용

    try {
        bool result = false;
        result = fs::create_directory("invalid/not/exist/path"); // 예외 발생  
    }
    catch (const mce::traceable_exception& e) {
        auto what = std::string(e.what()); // 예외 메시지
        auto file_name = std::string(e.file_name()); // 예외 발생 파일 이름
        auto line_number = e.line_number(); // 예외 발생 파일의 라인 번호

        mcsp::eprintln("[1][traceable_exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", what);
        mcsp::eprintln(" 파일: {}", file_name);
        mcsp::eprintln(" 라인: {}", line_number);
    }
    catch (const std::exception& e) {
        // 이곳으로 예외 발생됨
        mcsp::eprintln("[1][exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", e.what());
    }
    catch (...) {
        mcsp::eprintln("[1][예외 발생]");
        mcsp::eprintln(" 알 수 없는 예외가 발생했습니다.");
    }

    mcsp::println("\n");

    //--------------------------------------------
    // [2] MINO_X() 매크로 사용

    try {
        bool result = false;
        MINO_X(result = fs::create_directory("invalid/not/exist/path")); // 예외 발생  
    }
    catch (const mce::traceable_exception& e) {
        // 이곳으로 예외 발생됨
        auto what = std::string(e.what()); // 예외 메시지
        auto file_name = std::string(e.file_name()); // 예외 발생 파일 이름
        auto line_number = e.line_number(); // 예외 발생 파일의 라인 번호

        mcsp::eprintln("[2][traceable_exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", what);
        mcsp::eprintln(" 파일: {}", file_name);
        mcsp::eprintln(" 라인: {}", line_number);
    }
    catch (const std::exception& e) {
        mcsp::eprintln("[2][exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", e.what());
    }
    catch (...) {
        mcsp::eprintln("[2][예외 발생]");
        mcsp::eprintln(" 알 수 없는 예외가 발생했습니다.");
    }

    mcsp::println("\n");

    //--------------------------------------------
    // [3] 함수 호출 시 MINO_X() 매크로 사용
    //   - 함수 내부까지는 추적 안됨.
    //   - 함수 내부까지 추적을 위해서는 boost.stacktrace 또는 C++23 std::stacktrace 사용 필요.

    try {
        MINO_X(test());
    }
    catch (const mce::traceable_exception& e) {
        auto what = std::string(e.what()); // 예외 메시지
        auto file_name = std::string(e.file_name()); // 예외 발생 파일 이름
        auto line_number = e.line_number(); // 예외 발생 파일의 라인 번호

        mcsp::eprintln("[3][traceable_exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", what);
        mcsp::eprintln(" 파일: {}", file_name);
        mcsp::eprintln(" 라인: {}", line_number);
    }
    catch (const std::exception& e) {
        mcsp::eprintln("[3][exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", e.what());
    }
    catch (...) {
        mcsp::eprintln("[3][예외 발생]");
        mcsp::eprintln(" 알 수 없는 예외가 발생했습니다.");
    }

    mcsp::println("\n");

    //--------------------------------------------
    // [4] 멀티 라인 명령 매크로 사용
    //  - C++17 버전 제한 때문에 번거로운 매크로 사용방법이 제시됨.
    //  - C++23 이상인 경우 std::stacktrace를 활용할 것.
    try {
        bool result = false;
        MINO_X_BEGIN
            MINO_X_LINE; int a = 10;
        MINO_X_LINE; std::string b = std::to_string(a);
        MINO_X_LINE; result = fs::create_directory("invalid/not/exist/path"); // 예외 발생
        MINO_X_LINE; std::cout << tce("[4][MINO_X2] 디렉토리 생성 시도") << std::endl;
        MINO_X_END
    }
    catch (const mce::traceable_exception& e) {
        auto what = std::string(e.what()); // 예외 메시지
        auto file_name = std::string(e.file_name()); // 예외 발생 파일 이름
        auto line_number = e.line_number(); // 예외 발생 파일의 라인 번호

        mcsp::eprintln("[4][traceable_exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", what);
        mcsp::eprintln(" 파일: {}", file_name);
        mcsp::eprintln(" 라인: {}", line_number);
    }
    catch (const std::exception& e) {
        mcsp::eprintln("[4][exception][예외 발생]");
        mcsp::eprintln(" 메시지: {}", e.what());
    }
    catch (...) {
        mcsp::eprintln("[4][예외 발생]");
        mcsp::eprintln(" 알 수 없는 예외가 발생했습니다.");
    }

    mcsp::println("\n");
    mcsp::println("================================");
    mcsp::println(" traceable_exception 테스트 종료 ");
    mcsp::println("================================");
    mcsp::println("\n");

} // void test_traceable_exception()
