#include <iostream>
#include <filesystem>
#include "mino/core/string/string.hpp"
#include "mino/core/exception/exception.hpp"

int main(int argc, char* argv[]) {
    namespace mce = ::mino::core::exception;
    namespace mcs = ::mino::core::string;
    auto tce = mcs::to_console_encoding;

    try {
        namespace fs = ::std::filesystem;
        MINO_X(fs::create_directory("invalid/not/exist/path"));
    }
    catch (const mce::traceable_exception& e) {
        auto what = std::string(e.what());
        auto file_name = std::string(e.file_name());
        auto line_number = e.line_number();

        std::cout << tce("[예외 발생]\n");
        std::cout << tce("메시지: ") << what << std::endl;
        std::cout << tce("파일: ") << file_name << std::endl;
        std::cout << tce("라인: ") << line_number << std::endl;
    }

    return 0;
}
