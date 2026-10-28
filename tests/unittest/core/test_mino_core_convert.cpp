#include "catch_amalgamated.hpp"
#include "mino/core/convert/convert.hpp"

#include <string>
#include <limits>
#include <cmath>
#include <cstdint>

using namespace mino::core::convert;

TEST_CASE("정수형(integral) 문자열 파싱 테스트", "[convert][number_string]") {
    SECTION("기본 양수 및 음수 정수 변환") {
        auto val_int = safe_string_to_num<int>("12345");
        REQUIRE(val_int.has_value());
        CHECK(*val_int == 12345);

        auto val_neg = safe_string_to_num<int>("-9876");
        REQUIRE(val_neg.has_value());
        CHECK(*val_neg == -9876);
    }

    SECTION("무부호 정수(unsigned int, uint64_t) 변환") {
        auto val_uint = safe_string_to_num<uint32_t>("4294967295");
        REQUIRE(val_uint.has_value());
        CHECK(*val_uint == 4294967295U);

        auto val_u64 = safe_string_to_num<uint64_t>("18446744073709551615");
        REQUIRE(val_u64.has_value());
        CHECK(*val_u64 == 18446744073709551615ULL);

        // unsigned 타입에 음수 전달 시 실패
        auto invalid_uint = safe_string_to_num<uint32_t>("-1");
        CHECK_FALSE(invalid_uint.has_value());
    }

    SECTION("정수 경계값(limits) 검증") {
        auto max_int = safe_string_to_num<int>(std::to_string(std::numeric_limits<int>::max()));
        REQUIRE(max_int.has_value());
        CHECK(*max_int == std::numeric_limits<int>::max());

        auto min_int = safe_string_to_num<int>(std::to_string(std::numeric_limits<int>::min()));
        REQUIRE(min_int.has_value());
        CHECK(*min_int == std::numeric_limits<int>::min());

        // 범위 초과(overflow) 시 nullopt 반환
        auto overflow_int = safe_string_to_num<int>("99999999999999999999");
        CHECK_FALSE(overflow_int.has_value());
    }

    SECTION("유효하지 않은 문자열 형식 검증") {
        // 빈 문자열
        CHECK_FALSE(safe_string_to_num<int>("").has_value());
        // 숫자가 아닌 문자 포함
        CHECK_FALSE(safe_string_to_num<int>("123a").has_value());
        CHECK_FALSE(safe_string_to_num<int>("abc").has_value());
        // 공백 포함 시 실패 (std::from_chars는 공백을 스킵하지 않음)
        CHECK_FALSE(safe_string_to_num<int>(" 123").has_value());
        CHECK_FALSE(safe_string_to_num<int>("123 ").has_value());
        // 소수점이 포함된 경우
        CHECK_FALSE(safe_string_to_num<int>("12.34").has_value());
    }
}

TEST_CASE("부동소수점(floating-point) 문자열 파싱 테스트", "[convert][number_string]") {
    SECTION("float 파싱") {
        auto val_f = safe_string_to_num<float>("3.1415");
        REQUIRE(val_f.has_value());
        CHECK(Catch::Approx(*val_f).epsilon(0.0001) == 3.1415f);

        auto val_neg_f = safe_string_to_num<float>("-0.005");
        REQUIRE(val_neg_f.has_value());
        CHECK(Catch::Approx(*val_neg_f).epsilon(0.0001) == -0.005f);
    }

    SECTION("double 파싱") {
        auto val_d = safe_string_to_num<double>("123456.789012");
        REQUIRE(val_d.has_value());
        CHECK(Catch::Approx(*val_d).epsilon(0.000001) == 123456.789012);

        // 지수 표기법 (e-notation) 파싱
        auto val_exp = safe_string_to_num<double>("1.23e4");
        REQUIRE(val_exp.has_value());
        CHECK(Catch::Approx(*val_exp).epsilon(0.001) == 12300.0);
    }

    SECTION("부동소수점 유효하지 않은 입력 및 범위 초과") {
        CHECK_FALSE(safe_string_to_num<double>("").has_value());
        CHECK_FALSE(safe_string_to_num<double>("not_a_number").has_value());

        // float 표현 범위를 크게 벗어난 값
        CHECK_FALSE(safe_string_to_num<float>("1e300").has_value());
    }
}

TEST_CASE("부동소수점 숫자 -> 문자열 변환(safe_num_to_string) 테스트", "[convert][number_string]") {
    SECTION("기본 정밀도 (precision < 0) - std::to_string 동작") {
        auto str_d = safe_num_to_string(3.14);
        REQUIRE(str_d.has_value());
        CHECK(*str_d == std::to_string(3.14));

        auto str_f = safe_num_to_string(1.5f);
        REQUIRE(str_f.has_value());
        CHECK(*str_f == std::to_string(1.5f));
    }

    SECTION("고정 소수점(use_fixed = true) 및 precision 지정") {
        // 소수점 2자리 반올림 포맷팅
        auto str_2 = safe_num_to_string(3.141592, 2, true);
        REQUIRE(str_2.has_value());
        CHECK(*str_2 == "3.14");

        // 소수점 4자리 포맷팅
        auto str_4 = safe_num_to_string(3.141592, 4, true);
        REQUIRE(str_4.has_value());
        CHECK(*str_4 == "3.1416");

        // 정수 부분만(소수점 0자리)
        auto str_0 = safe_num_to_string(12.7, 0, true);
        REQUIRE(str_0.has_value());
        CHECK(*str_0 == "13");
    }

    SECTION("일반 포맷(use_fixed = false) 및 precision 지정") {
        // use_fixed = false일 때 유효숫자(significant figures) 기준으로 동작
        auto str_sig = safe_num_to_string(123.456, 4, false);
        REQUIRE(str_sig.has_value());
        CHECK(*str_sig == "123.5");
    }

    SECTION("NaN 처리 확인") {
        double nan_val = std::numeric_limits<double>::quiet_NaN();
        auto str_nan = safe_num_to_string(nan_val);
        CHECK_FALSE(str_nan.has_value());

        float nan_val_f = std::numeric_limits<float>::quiet_NaN();
        auto str_nan_f = safe_num_to_string(nan_val_f, 2);
        CHECK_FALSE(str_nan_f.has_value());
    }
}
