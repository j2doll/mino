#pragma once

#include <iostream>
#include <string>

namespace mino::core::validation::fluent_validation_wrapper {

    // PII 검증
    bool is_email(const std::string& value);
    bool is_phone_number(const std::string& value); // 한국 기준
    bool is_url(const std::string& value);
    bool is_ip_address(const std::string& value);

    // 주민등록번호(주민번) 검증: 13자리(하이픈 허용), 날짜 유효성 및 체크섬 검사
    bool is_resident_registration_number(const std::string& value);

    // 문자열 속성 검증
    bool is_alpha(const std::string& value);
    bool is_alphanumeric(const std::string& value);
    bool is_numeric(const std::string& value);

    // 데이터 형식 검증
    bool is_base64(const std::string& value);
    bool is_hex_color(const std::string& value);
    bool is_json(const std::string& value);

} 