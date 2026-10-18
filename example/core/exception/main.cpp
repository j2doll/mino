#include <iostream>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/exception/exception.hpp"

extern void test_traceable_exception(); 

int main(int argc, char* argv[]) {
    namespace mce = mino::core::exception;
    namespace mcs = mino::core::string;
    namespace fs = ::std::filesystem;

    auto tce = mcs::to_console_encoding;

    // traceable_exception 테스트
    test_traceable_exception();

    return 0;
}
