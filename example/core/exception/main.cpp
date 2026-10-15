#include <iostream>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/exception/exception.hpp"

bool test() {
    namespace fs = ::std::filesystem;
    bool result = fs::create_directory("invalid/not/exist/path");
    return result;
}

int main(int argc, char* argv[]) {
    namespace mce = mino::core::exception;
    namespace mcs = mino::core::string;
    namespace fs = ::std::filesystem;

    auto tce = mcs::to_console_encoding;

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

        std::cout << tce("[1][traceable_exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << what << std::endl;
        std::cout << tce(" 파일: ") << file_name << std::endl;
        std::cout << tce(" 라인: ") << line_number << std::endl;
    }
    catch (const std::exception& e) {
        // 이곳으로 예외 발생됨
        std::cout << tce("[1][exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << e.what() << std::endl;
    }
    catch (...) {
        std::cout << tce("[1][예외 발생]\n");
        std::cout << tce(" 알 수 없는 예외가 발생했습니다.") << std::endl;
    }

    std::cout << std::endl;

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

        std::cout << tce("[2][traceable_exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << what << std::endl;
        std::cout << tce(" 파일: ") << file_name << std::endl;
        std::cout << tce(" 라인: ") << line_number << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << tce("[2][exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << e.what() << std::endl;
    }
    catch (...) {
        std::cout << tce("[2][예외 발생]\n");
        std::cout << tce(" 알 수 없는 예외가 발생했습니다.") << std::endl;
    }

    std::cout << std::endl;

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

        std::cout << tce("[3][traceable_exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << what << std::endl;
        std::cout << tce(" 파일: ") << file_name << std::endl;
        std::cout << tce(" 라인: ") << line_number << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << tce("[2][exception][예외 발생]\n");
        std::cout << tce(" 메시지: ") << e.what() << std::endl;
    }
    catch (...) {
        std::cout << tce("[2][예외 발생]\n");
        std::cout << tce(" 알 수 없는 예외가 발생했습니다.") << std::endl;
    }

    return 0;
}
