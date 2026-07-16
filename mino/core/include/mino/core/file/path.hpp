#pragma once

#include <string>
#include <filesystem>

namespace mino::core::file {
    // UTF-8 인코딩된 std::string 을 std::filesystem::path 로 변환
    // Windows: CP_UTF8 -> wide 문자로 변환 후 path(wstring) 생성
    // POSIX : path(std::string) 으로 생성
     std::filesystem::path path_from_utf8(const std::string& utf8);
}
