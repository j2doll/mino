#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>

#include "mino/core/expected/expected.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 네임스페이스 별칭
namespace cexp = mino::core::expected;

// 커스텀 에러 열거형
enum class MathError {
    DivisionByZero,
    NegativeSquareRoot,
    OutOfBounds
};

// 테스트용 클래스 정의
class MathCalculator {
public:
    explicit MathCalculator(int max_limit = 1000) : max_limit_(max_limit) {}

    void set_max_limit(int limit) { max_limit_ = limit; }

    // 1. const 멤버 함수: 나눗셈 계산
    //    기대값 타입: int
    //    에러값 타입: std::string 
    cexp::expected<int, std::string> divide(int numerator, int denominator) const {
        if (denominator == 0) {
            return cexp::unexpected_value<std::string>("0으로 나눌 수 없습니다.");
        }

        int result = numerator / denominator;
        if (std::abs(result) > max_limit_) {
            return cexp::unexpected_value<std::string>("계산 결과가 한계치를 초과했습니다.");
        }

        return result; // int 타입의 정상 결과 반환
    }

    // 2. Enum을 에러 타입으로 사용하는 멤버 함수: 제곱근 계산
    //    기대값 타입: double
    //    에러값 타입: MathError(열거값)
    cexp::expected<double, MathError> safe_sqrt(double val) const {
        if (val < 0.0) {
            return cexp::unexpected_value<MathError>(MathError::NegativeSquareRoot);
        }
        return std::sqrt(val); // double 타입의 정상 결과 반환
    }

    // 3. 상태 변경(non-const) 멤버 함수: 계산 결과를 히스토리에 추가
    //    기대값 타입: int
    //    에러값 타입: std::string
    cexp::expected<int, std::string> add_to_history(int a, int b) {
        auto res = divide(a, b); // 내부에서 다른 멤버 함수 호출
        if (res) {
            history_.push_back(res.value());
        }
        return res; 
    }

    // 4. static 멤버 함수: 문자열을 정수로 파싱
    //    기대값 타입: int
    //    에러값 타입: std::string
    static cexp::expected<int, std::string> parse_int(const std::string& str) {
        if (str.empty()) {
            return cexp::unexpected_value<std::string>("빈 문자열입니다.");
        }
        try {
            return std::stoi(str); // int 타입의 정상 결과 반환
        }
        catch (...) {
            return cexp::unexpected_value<std::string>("정수로 변환할 수 없습니다.");
        }
    }

    // Getter
    const std::vector<int>& history() const { return history_; }

private:
    int max_limit_;
    std::vector<int> history_;
};

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== 클래스 멤버 함수 exp::expected 테스트 시작 ===")
        << std::endl;

    MathCalculator calc(100); // 최대 한계치 100 지정

    // -------------------------------------------------------------
    // [테스트 1] const 멤버 함수 테스트 (divide)
    // -------------------------------------------------------------
    {
        // 1-1. 정상 성공 케이스
        auto res_ok = calc.divide(50, 2); // 50 나누기 2 = 25
        assert(res_ok.has_value()); // 값 있음
        assert(res_ok.value() == 25); // 값
        std::cout
            << to_console_encoding("[PASS] 1-1. const 멤버 함수 성공: " +
                std::to_string(res_ok.value()))
            << std::endl;

        // 1-2. 0으로 나누기 에러 케이스
        auto res_zero = calc.divide(50, 0); // 50 나누기 0 = 에러
        assert(!res_zero.has_value()); // 값 없음
        assert(res_zero.error() == "0으로 나눌 수 없습니다.");
        assert(res_zero.value_or(-1) == -1); // res_zero에 값이 있으면 값을 반환하지만, 현재 값이 없고, 정의된 기본값(-1)으로 반환됨.
        std::cout
            << to_console_encoding("[PASS] 1-2. const 멤버 함수 에러(0 분모): " +
                res_zero.error())
            << std::endl;

        // 1-3. 한계치 초과 에러 케이스
        auto res_limit = calc.divide(500, 2); // 500 나누기 2 = 250 (결과 값이 사전에 정의한 한계치인 100을 초과함.)
        assert(!res_limit.has_value()); // 값 없음
        assert(res_limit.error() == "계산 결과가 한계치를 초과했습니다.");
        std::cout
            << to_console_encoding("[PASS] 1-3. const 멤버 함수 에러(한계 초과): " +
                res_limit.error())
            << std::endl;
    }

    // -------------------------------------------------------------
    // [테스트 2] Enum 에러 타입 멤버 함수 테스트 (safe_sqrt)
    // -------------------------------------------------------------
    {
        // 2-1. 정상 제곱근
        auto res_ok = calc.safe_sqrt(16.0); // 16의 제곱근 = 4
        assert(res_ok.has_value()); // 값 있음
        assert(res_ok.value() == 4.0); // 값
        std::cout
            << to_console_encoding("[PASS] 2-1. Enum 에러 타입 성공: " +
                std::to_string(res_ok.value()))
            << std::endl;

        // 2-2. 음수 제곱근 에러
        auto res_err = calc.safe_sqrt(-9.0); // -9의 제곱근 = 에러
        assert(!res_err.has_value()); // 값 없음
        assert(res_err.error() == MathError::NegativeSquareRoot); // Enum 타입 에러
        std::cout
            << to_console_encoding("[PASS] 2-2. Enum 에러 타입 실패 검증 완료")
            << std::endl;
    }

    // -------------------------------------------------------------
    // [테스트 3] 객체 상태 변경 멤버 함수 테스트 (add_to_history)
    // -------------------------------------------------------------
    {
        assert(calc.history().empty()); // 초기 히스토리 비어있음

        // 히스토리 추가 성공
        auto res1 = calc.add_to_history(20, 2); // 20 나누기 2 = 10
        assert(res1.has_value() && res1.value() == 10); // 값 있음 (값: 10)
        assert(calc.history().size() == 1); // 히스토리 크기 1
        assert(calc.history()[0] == 10); // 첫 번째 히스토리 값 10

        // 히스토리 추가 실패 
        auto res2 = calc.add_to_history(20, 0); // 20 나누기 0 = 에러 (실패 시, 히스토리에 기록되지 않음.)
        assert(!res2.has_value()); // 값 없음 (에러)
        assert(calc.history().size() == 1); // 히스토리 크기가 변하지 않음. 

        std::cout
            << to_console_encoding("[PASS] 3. 상태 변경 멤버 함수 및 히스토리 누적 검증 완료")
            << std::endl;
    }

    // -------------------------------------------------------------
    // [테스트 4] static 멤버 함수 테스트 (parse_int)
    // -------------------------------------------------------------
    {
        // 4-1. 파싱 성공
        auto res_ok = MathCalculator::parse_int("123"); // 문자열 "123"을 정수 123으로 파싱
        assert(res_ok.has_value()); // 값 있음
        assert(res_ok.value() == 123); // 값

        // 4-2. 빈 문자열 실패
        auto res_empty = MathCalculator::parse_int(""); // 빈 문자열을 정수로 파싱 시도
        assert(!res_empty.has_value()); // 값 없음
        assert(res_empty.error() == "빈 문자열입니다."); // 에러 메시지 확인

        // 4-3. 잘못된 문자로 인한 실패
        auto res_invalid = MathCalculator::parse_int("abc");
        assert(!res_invalid.has_value()); // 값 없음
        assert(res_invalid.error() == "정수로 변환할 수 없습니다."); // 에러 메시지 확인

        std::cout
            << to_console_encoding("[PASS] 4. static 멤버 함수 검증 완료")
            << std::endl;
    }

    std::cout
        << to_console_encoding("=== 모든 클래스 멤버 함수 검증(assert)을 통과했습니다 ===")
        << std::endl;

    return 0;
}
