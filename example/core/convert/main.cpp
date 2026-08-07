#include <iostream>
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <cstdint>

#include "mino/core/convert/convert.hpp" 

void test_safe_string_to_num_integral() {
    namespace convert = mino::core::convert;

    std::cout << "[Test] safe_string_to_num (Integral)\n";

    // 1. 정상 양수/음수 변환
    auto res1 = convert::safe_string_to_num<int>("12345");
    assert(res1.has_value() && res1.value() == 12345);

    auto res2 = convert::safe_string_to_num<int>("-6789");
    assert(res2.has_value() && res2.value() == -6789);

    // 2. 빈 문자열
    auto res_empty = convert::safe_string_to_num<int>("");
    assert(!res_empty.has_value());

    // 3. 잘못된 문자가 포함된 경우 (전체 파싱 실패 검증)
    auto res_invalid = convert::safe_string_to_num<int>("123a");
    assert(!res_invalid.has_value());

    // 4. 데이터 타입 범위 초과 (Overflow) (int16_t 범위: -32768 ~ 32767)
    auto res_overflow = convert::safe_string_to_num<int16_t>("999999");
    assert(!res_overflow.has_value());

    // 부호 있는 64비트 정수 (Signed 64-bit Integer) 최댓값: 9,223,372,036,854,775,807 (약 922경 3,372조)
    auto res_max = convert::safe_string_to_num<long long>("9223372036854775807");
    assert(res_max.has_value() && res_max.value() == std::numeric_limits<long long>::max());

    std::cout << " -> Integral conversion tests passed.\n\n";
}

void test_safe_string_to_num_floating() {
    namespace convert = mino::core::convert;

    std::cout << "[Test] safe_string_to_num (Floating Point)\n";

    // 1. float / double 정상 변환
    auto res_float = convert::safe_string_to_num<float>("3.14"); // float
    assert(res_float.has_value() && std::abs(res_float.value() - 3.14f) < 0.001f); // 소수점 3자리까지는 정확하게 비교

    auto res_float_negative = convert::safe_string_to_num<float>("-45.678f"); // 'f' 접미사가 붙은 경우에도 변환 가능하도록 처리되어야 함
    assert(res_float_negative.has_value() && std::abs(res_float_negative.value() - (-45.678f)) < 0.001f); // 소수점 3자리까지는 정확하게 비교

    auto res_double = convert::safe_string_to_num<double>("-123.456789"); // double
    assert(res_double.has_value() && std::abs(res_double.value() - (-123.456789)) < 0.000001); // 소수점 6자리까지는 정확하게 비교

    auto res_scientific = convert::safe_string_to_num<double>("1e1"); // 1e1 = 10.0
    assert(res_scientific.has_value() && std::abs(res_scientific.value() - 10.0) < 0.01); // 소수점 2자리까지는 정확하게 비교 

    // 2. 잘못된 문자열 예외 처리 (std::invalid_argument)
    auto res_invalid = convert::safe_string_to_num<double>("abc");
    assert(!res_invalid.has_value());

    // 3. 범위 초과 예외 처리 (std::out_of_range) (float 범위: 약 ±3.4e38)
    auto res_overflow = convert::safe_string_to_num<float>("1e39");
    assert(!res_overflow.has_value());

    std::cout << " -> Floating point conversion tests passed.\n\n";
}

void test_safe_num_to_string() {
    namespace convert = mino::core::convert;

    std::cout << "[Test] safe_num_to_string (Floating Point)\n";

    // 1. NaN 입력 검증
    constexpr double nan_val = std::numeric_limits<double>::quiet_NaN(); // not a number (NaN)
    auto res_nan = convert::safe_num_to_string(nan_val);
    assert(!res_nan.has_value());

    // 2. precision < 0 (기본 std::to_string 사용)
    auto res_default = convert::safe_num_to_string(3.141592, -1); // -1은 precision < 0을 의미 
    assert(res_default.has_value()); // 기본 std::to_string 사용 
    std::cout << "  - Default (precision < 0): " << res_default.value() << '\n'; // 3.141592 출력

    // 3. precision >= 0 및 use_fixed = true
    auto res_fixed = convert::safe_num_to_string(3.141592, 2, true); // 정밀도 2, 고정 소수점 사용
    assert(res_fixed.has_value() && res_fixed.value() == "3.14");
    std::cout << "  - Fixed (precision = 2): " << res_fixed.value() << '\n'; // 3.14 출력

    // 4. precision >= 0 및 use_fixed = false
    auto res_scientific = convert::safe_num_to_string(1234.5678, 2, false); // 정밀도 2, 과학적 표기법 사용
    assert(res_scientific.has_value());
    std::cout << "  - Non-fixed (precision = 2): " << res_scientific.value() << '\n'; // 1.2e+03 과학적 표기법으로 출력. 

    std::cout << " -> Number to string conversion tests passed.\n\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << " Starting Convert Library Unit Tests    \n";
    std::cout << "========================================\n\n";

    test_safe_string_to_num_integral();
    test_safe_string_to_num_floating();
    test_safe_num_to_string();

    std::cout << "All unit tests completed successfully!\n";
    return 0;
}
