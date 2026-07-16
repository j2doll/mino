#pragma once

#include <filesystem>
#include <system_error>
#include <string>
#include <vector>
#include <cstring>

namespace mino::core::file {

    // 실행 파일 경로 조회 동작을 제어하는 옵션
    struct  exec_path_options {
        bool resolve_symlink = true; // 심볼릭 링크/정션 해제 여부
        bool prefer_realpath_on_linux_deleted = true; // Linux에서 (deleted) 파일 우선 realpath 사용 여부
        bool windows_keep_device_prefix = false; // Windows에서 \\?\ 접두사 유지 여부
    };

    // executable_path()
    //    - 반환 타입: std::filesystem::path
    //    - 실행 프로그램의 전체 경로(디렉터리 + 파일명)를 path 객체로 반환한다.
    //    - path 객체를 사용하면 parent_path(), filename(), stem(), extension() 등
    //      파일시스템 조작 메서드를 활용할 수 있다.
    // 
     std::filesystem::path executable_path(std::error_code& ec, const exec_path_options& opt = {}); // 실행 파일 전체 경로 (디렉터리 + 파일명)
     std::filesystem::path executable_path(const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 path 반환

    // executable_dir()
    //    - 반환 타입: std::filesystem::path
    //    - 실행 프로그램의 디렉터리 경로만 path 객체로 반환한다
    //    - path 객체를 사용하면 parent_path(), filename(), stem(), extension() 등
    //      파일시스템 조작 메서드를 활용할 수 있다.
     std::filesystem::path executable_dir(std::error_code& ec, const exec_path_options& opt = {}); // 실행 파일 디렉터리 (디렉터리만)
     std::filesystem::path executable_dir(const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 path 반환

    // executable_path_string()
    //    - 반환 타입: std::string (Windows에서는 UTF-8 변환 u8string())
    //    - 실행 프로그램의 전체 경로를 문자열로 반환한다.
    //    - 출력, 로그 기록, 직렬화 등 단순 문자열 처리가 필요한 경우에 편리하다.
     std::string  executable_path_string(std::error_code& ec, const exec_path_options& opt = {}); // 문자열 래퍼 
     std::string  executable_path_string(const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 문자열 반환

    // executable_path_wstring()
    //    - 반환 타입: std::wstring
    //    - 실행 프로그램의 전체 경로를 와이드 문자열로 반환한다.
    //    - Windows에서 유니코드 경로 처리가 필요할 때 유용
     std::wstring executable_path_wstring(std::error_code& ec, const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 문자열 반환
     std::wstring executable_path_wstring(const exec_path_options& opt = {});

    // executable_dir_string()
    //    - 반환 타입: std::string (Windows에서는 UTF-8 변환 u8string())
    //    - 실행 프로그램의 디렉터리 경로만 문자열로 반환한다.
    //    - 출력, 로그 기록, 직렬화 등 단순 문자열 처리가 필요한 경우에 편리하다.
     std::string  executable_dir_string(std::error_code& ec, const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 문자열 반환
     std::string  executable_dir_string(const exec_path_options& opt = {});

    // executable_dir_wstring()
    //    - 반환 타입: std::wstring
    //    - 실행 프로그램의 디렉터리 경로만 와이드 문자열로 반환
    //    - Windows에서 유니코드 경로 처리가 필요할 때 유용
     std::wstring executable_dir_wstring(std::error_code& ec, const exec_path_options& opt = {}); // 예외를 던지지 않고, 실패 시 빈 문자열 반환
     std::wstring executable_dir_wstring(const exec_path_options& opt = {});

} // namespace mino::core::file
