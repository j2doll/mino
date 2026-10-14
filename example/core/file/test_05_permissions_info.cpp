#include <filesystem>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_permissions_info(std::filesystem::path sub_file1)
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    namespace mcs = mino::core::string;

    auto to_console_encoding = mcs::to_console_encoding;

    using file_info = mcfile::file_info;

    auto sub_file1_path_string = sub_file1.string();
    // 경로 문자열 (윈도우일 경우 CP949, 리눅스일 경우 UTF-8)

    mcfile::permissions_info pinfo(sub_file1_path_string); // 파일 권한 정보

    mcsp::println("pinfo.exists() : {}", pinfo.exists() ); // 존재 여부
    mcsp::println("pinfo.to_string(): {}", pinfo.to_string()); // 파일 권한 정보 문자열

    // perms 구조체 멤버 (read, write, execute) 사용

    mcfile::perms owner_p = pinfo.for_owner(); // 소유자 권한
    mcsp::println("Owner - R:{} W:{} X:{}", owner_p.read, owner_p.write, owner_p.execute);

    mcfile::perms group_p = pinfo.for_group(); // 그룹 권한
    mcsp::println("Group - R:{} W:{} X:{}", group_p.read, group_p.write, group_p.execute);

    mcfile::perms others_p = pinfo.for_others(); // 기타 사용자 권한
    mcsp::println("Others - R:{} W:{} X:{}", others_p.read, others_p.write, others_p.execute);

    return true; 
}
