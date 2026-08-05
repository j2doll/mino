#include <iostream>
#include <iomanip>
#include <vector>

#include "mino/core/string/to_console_encoding.hpp"

int main(int argc, char** argv)
{
    namespace mstring = mino::core::string;
    auto to_console_encoding = mstring::to_console_encoding;

    std::string utf8string = "한글";
    auto result = to_console_encoding(utf8string);
    std::cout << "to_console_encoding() : " << result << "\n";

    return 0;
}
