#pragma once

#include <string>

#include "mino/core/file/executable_path.hpp"

namespace mino::core::file {

    // 실행 파일 이름 반환 형식
    enum class name_type {
        filename, // 확장자 포함 (Hello.exe)
        stem      // 확장자 제외 (Hello)
    };

    // 실행 파일 이름 반환. 예: Hello.exe 또는 Hello
     std::string executable_name(
        std::error_code& ec, // 에러 코드 반환 버전
        name_type type = name_type::filename, // 반환 형식 지정 (기본값: filename)
        const exec_path_options& opt = {} // 실행 경로 조회 옵션 (기본값: 기본 옵션)
    );

    // 예외를 던지지 않고, 실패 시 빈 문자열 반환
     std::string executable_name(
        name_type type = name_type::filename, // 반환 형식 지정 (기본값: filename)
        const exec_path_options& opt = {} // 실행 경로 조회 옵션 (기본값: 기본 옵션)
    );

    // 실행 파일 이름 반환. 예: L"Hello.exe" 또는 L"Hello" 
     std::wstring executable_name_w(
        std::error_code& ec, // 에러 코드 반환 버전
        name_type type = name_type::filename, // 반환 형식 지정 (기본값: filename)
        const exec_path_options& opt = {} // 실행 경로 조회 옵션 (기본값: 기본 옵션)
    );

    // 예외를 던지지 않고, 실패 시 빈 문자열 반환
     std::wstring executable_name_w(
        name_type type = name_type::filename, // 반환 형식 지정 (기본값: filename)
        const exec_path_options& opt = {} // 실행 경로 조회 옵션 (기본값: 기본 옵션)
    );

} // namespace mino::core::file
