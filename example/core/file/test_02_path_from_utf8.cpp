#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_path_from_utf8()
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::string utf8_path_str = "test_dir_한글경로_유니코드/file.txt";
    std::filesystem::path converted_path = mcfile::path_from_utf8(utf8_path_str);

    auto converted_path_str = converted_path.string(); // OS 캐릭터셋 문자열

    mcsp::println("path_from_utf8 변환 결과: ");
    std::cout << converted_path_str << std::endl;

    return true;
}
