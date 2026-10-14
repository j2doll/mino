#include <filesystem>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"

bool test_file_size_formatter(std::filesystem::path sub_file1)
{
    namespace mcsp = mino::core::string::print;
    namespace mcfile = mino::core::file;
    namespace mcs = mino::core::string;

    auto to_console_encoding = mcs::to_console_encoding;

    using fsf = mcfile::file_size_formatter;
    using metric_type = mcfile::metric_type;

    auto metric_si = metric_type::si;
    auto metric_iec = metric_type::iec;

    //---------------------------------------------------------
    // 6-1. byte_to_string() 테스트 (metric_type::iec 및 metric_type::si)

    int precision = 2; // 소수점 이하 자리수 정밀도
    uint64_t bytes_numbers = 2048; // 변환할 바이트 수

    std::string sz_iec = fsf::byte_to_string(bytes_numbers, metric_iec, precision);
    // 2048 를 IEC 단위의 문자열로 변환

    mcsp::println("2048 B (IEC): {}", sz_iec); // 2048 B (IEC): 2 KB
    // IEC 단위: 1024 B = 1 KiB

    std::string sz_si = fsf::byte_to_string(bytes_numbers, metric_si, precision);
    // 2048 를 SI 단위의 문자열로 변환

    mcsp::println("2048 B (SI): {}", sz_si); // 2048 B (SI) : 2.05 KB
    // SI 단위: 1000 B = 1 KB
    // 2048 B / 1000 = 2.048 KB => 소수점 이하 2자리까지 반올림하면 2.05 KB

    //---------------------------------------------------------
    // 6-2. string_to_bytes() 테스트

    std::string file_size_string = "1.5 MB"; // 변환할 파일 용량 문자열
    // MB 이외에 KB, GB, TB, PB, EB 등도 테스트 가능
    // "1.5MB" 나 "1.5 mb" 등 공백 유무 및 대소문자 구분 없이도 변환 가능

    auto bytes_opt = fsf::string_to_bytes(file_size_string, metric_iec);
    // "1.5 MB" 문자열을 IEC 단위의 바이트 수로 변환

    if (bytes_opt.has_value()) {
        auto bytes = *bytes_opt; // 변환된 바이트 수
        mcsp::println("'{}' -> {} bytes", file_size_string, bytes); // '1.5 MB' -> 1572864 bytes
    } else {
        mcsp::eprintln("문자열 변환 실패: {}", file_size_string);
        return false;
    }

    //---------------------------------------------------------
    // 6-3. get_filesize_string() 테스트

    auto fs_str_opt = fsf::get_filesize_string(sub_file1, metric_iec, precision);
    // sub_file1 (fs::path) 의 파일 용량을 IEC 단위 문자열로 변환

    if (fs_str_opt.has_value()) {
        auto fs_str = *fs_str_opt; // 변환된 파일 용량 문자열
        mcsp::println("sub_file1 용량: {}", fs_str);
    } else {
        mcsp::eprintln("문자열 변환 실패: {}", sub_file1);
        return false;
    }

    return true; 
}
