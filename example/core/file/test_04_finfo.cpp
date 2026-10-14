#include <filesystem>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_finfo(std::filesystem::path sub_file1)
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    namespace mcs = mino::core::string;

    auto to_console_encoding = mcs::to_console_encoding;

    using file_info = mcfile::file_info;

    // 경로 문자열 (윈도우일 경우 CP949, 리눅스일 경우 UTF-8)
    auto sub_file1_path_string = sub_file1.string();

    file_info finfo(sub_file1_path_string); // 파일 정보

    mcsp::println("exists() : {}", finfo.exists()); // 존재 여부

    mcsp::println("is_file() : {}", finfo.is_file()); // 파일인지 여부
    mcsp::println("is_dir() : {}", finfo.is_dir()); // 디렉터리인지 여부

    mcsp::println("file_name() : {}", finfo.file_name()); // 파일 이름
    mcsp::println("extension() : {}", finfo.extension()); // 파일 확장자

    mcsp::println("parent_path() : {}", finfo.parent_path()); // 부모 디렉터리 경로
    mcsp::println("absolute_path() : {}", finfo.absolute_path()); // 절대 경로
    mcsp::println("file_size() : {} bytes", finfo.file_size()); // 파일 크기

    return true; 
}
