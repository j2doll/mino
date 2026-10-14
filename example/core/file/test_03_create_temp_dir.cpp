#include <filesystem>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_create_temp_dir(
    std::filesystem::path root_dir,
    std::filesystem::path sub_file1,
    std::filesystem::path sub_file2)
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    namespace mcs = mino::core::string;

    auto to_console_encoding = mcs::to_console_encoding;

    std::error_code ec;
    std::filesystem::create_directories(root_dir, ec);
    if (ec) {
        mcsp::eprintln("임시 작업 디렉터리 생성 실패: " + ec.message());
        return false;
    }
    else {
        mcsp::println("임시 작업 디렉터리 생성 성공: " + root_dir.string());
    }

    {
        std::ofstream ofs(sub_file1);
        ofs << "// C++ Source code sample\nint main() { return 0; }";
    }

    {
        std::ofstream ofs(sub_file2);
        ofs << "Text file sample data";
    }

    return true;
}
