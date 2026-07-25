#include <cctype>
#include <string>
#include <vector>
#include <regex>

#include "mino/core/validation/fluent_validation_wrapper.hpp"

namespace mino::core::validation::fluent_validation_wrapper {

    // Email: 하이픈/대괄호 이슈를 피하기 위해 안전한 패턴으로 교체
    bool is_email(const std::string& value) {
        if (value.empty()) return false;
        const std::regex pattern(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
        return std::regex_match(value, pattern);
    }

    bool is_phone_number(const std::string& value) {
        if (value.empty()) return false;
        // 한국 전화번호 형식 (010-1234-5678 또는 02-123-4567 등)
        const std::regex pattern(R"(^(?:01[016789]-\d{3,4}-\d{4}|(?:02|0[3-6][1-9])-\d{3,4}-\d{4})$)");
        return std::regex_match(value, pattern);
    }

    bool is_url(const std::string& value) {
        if (value.empty()) return false;
        const std::regex pattern(R"(^https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)$)");
        return std::regex_match(value, pattern);
    }

    bool is_ip_address(const std::string& value) {
        if (value.empty()) return false;
        // IPv4 엄격 검사: 0.0.0.0 ~ 255.255.255.255
        const std::regex pattern(R"(^((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.){3}(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)$)");
        return std::regex_match(value, pattern);
    }

    bool is_alpha(const std::string& value) {
        if (value.empty()) return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isalpha(c);
        });
    }

    bool is_alphanumeric(const std::string& value) {
        if (value.empty()) return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isalnum(c);
        });
    }

    bool is_numeric(const std::string& value) {
        if (value.empty()) return false;
        // 테스트 기대대로 정수(소수 불허)만 허용하도록 변경
        return std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isdigit(c);
        });
    }

    bool is_base64(const std::string& value) {
        if (value.empty() || value.length() % 4 != 0) return false;
        const std::regex pattern(R"(^[A-Za-z0-9+/]*={0,2}$)");
        return std::regex_match(value, pattern);
    }

    bool is_hex_color(const std::string& value) {
        const std::regex pattern(R"(^#?([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$)");
        return std::regex_match(value, pattern);
    }

    bool is_json(const std::string& value) {
        if (value.empty()) return false;
        // 간단한 구조적 확인 (엄격 파싱은 외부 라이브러리 권장)
        return ((value.front() == '{' && value.back() == '}') ||
                (value.front() == '[' && value.back() == ']'));
    }

    bool is_resident_registration_number(const std::string& value)
    {
        // 숫자만 추출 (하이픈 허용)
        std::string digits;
        digits.reserve(13);
        for (unsigned char c : value) {
            if (std::isdigit(c)) {
                digits.push_back(static_cast<char>(c));
            }
        }

        // 길이 검사
        if (digits.size() != 13) {
            return false;
        }

        // 기본 숫자 확인(안전성)
        for (char c : digits) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }

        // 생년월일 부분(YYMMDD) 파싱
        int yy = std::stoi(digits.substr(0, 2));
        int mm = std::stoi(digits.substr(2, 2));
        int dd = std::stoi(digits.substr(4, 2));

        // 7번째 자리로 세기 및 세기(성별/세기) 판별
        char gender = digits[6];
        int full_year = 0;
        if (gender == '1' || gender == '2' || gender == '5' || gender == '6') {
            full_year = 1900 + yy;
        } else if (gender == '3' || gender == '4' || gender == '7' || gender == '8') {
            full_year = 2000 + yy;
        } else {
            // 0,9 등 허용되지 않는 값
            return false;
        }

        // 월/일 기본 유효성 검사 (윤년 포함)
        if (mm < 1 || mm > 12) {
            return false;
        }

        int mdays[13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
        bool leap = ( (full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0) );
        if (leap) {
            mdays[2] = 29;
        }

        if (dd < 1 || dd > mdays[mm]) {
            return false;
        }

        // 체크섬 계산
        const int weights[12] = { 2,3,4,5,6,7,8,9,2,3,4,5 };
        int sum = 0;
        for (int i = 0; i < 12; ++i) {
            sum += (digits[i] - '0') * weights[i];
        }
        int check = (11 - (sum % 11)) % 10;
        int last = digits[12] - '0';
        return check == last;
    }

} 