#include <string>
#include <vector>
#include <cmath>
#include <cassert>

#include "mino/core/expected/expected.hpp"
#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/string/print.hpp"  

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

    // 1. const 멤버 함수 divide() : 나눗셈 계산
    //    기대값 타입: int
    //    에러값 타입: std::string 
    mino::core::expected::expected<int, std::string> divide(int numerator, int denominator) const {
        if (denominator == 0) {
            return mino::core::expected::unexpected_value<std::string>("Cannot divide by 0.");
        }

        int result = numerator / denominator;
        if (std::abs(result) > max_limit_) {
            return mino::core::expected::unexpected_value<std::string>("The calculation result exceeded the limit.");
        }

        return result; // int 타입의 정상 결과 반환
    }

    // 2. Enum을 에러 타입으로 사용하는 멤버 함수 safe_sqrt() : 제곱근 계산
    //    기대값 타입: double
    //    에러값 타입: MathError(열거값)
    mino::core::expected::expected<double, MathError> safe_sqrt(double val) const {
        if (val < 0.0) {
            return mino::core::expected::unexpected_value<MathError>(MathError::NegativeSquareRoot);
        }
        return std::sqrt(val); // double 타입의 정상 결과 반환
    }

    // 3. 상태 변경(non-const) 멤버 함수 add_to_history() : 계산 결과를 히스토리에 추가
    //    기대값 타입: int
    //    에러값 타입: std::string
    mino::core::expected::expected<int, std::string> add_to_history(int a, int b) {
        auto res = divide(a, b); // 내부에서 다른 멤버 함수 호출
        if (res) {
            history_.push_back(res.value());
        }
        return res;
    }

    // 4. static 멤버 함수 parse_int() : 문자열을 정수로 파싱
    //    기대값 타입: int
    //    에러값 타입: std::string
    static mino::core::expected::expected<int, std::string> parse_int(const std::string& str) {
        if (str.empty()) {
            return mino::core::expected::unexpected_value<std::string>("This is an empty string.");
        }
        try {
            return std::stoi(str); // int 타입의 정상 결과 반환
        }
        catch (...) {
            return mino::core::expected::unexpected_value<std::string>("Cannot convert to integer.");
        }
    }

    // Getter
    const std::vector<int>& history() const { return history_; }

private:
    int max_limit_;
    std::vector<int> history_;
};

int main() {
    using namespace mino::core::string::print;

    println("=== 클래스 멤버 함수 exp::expected 테스트 시작 ===");

    MathCalculator calc(100); // 최대 한계치 100 지정

    // -------------------------------------------------------------
    // [테스트 1] const 멤버 함수 테스트 (divide)
    // -------------------------------------------------------------
    {
        // 1-1. 정상 성공 케이스
        auto res_ok = calc.divide(50, 2); // 반환값 <int, std::string>
        if (res_ok.has_value()) { // 성공

            auto result = res_ok.value(); // 성공인 경우 int 타입
            assert(result == 25); // 값

            println("[INFO] 1-1. const 멤버 함수 결과: {}", result);
        } else {
            eprintln("[ERROR] 1-1. const 멤버 함수 실패");
            return 11;
        }

        // 1-2. 0으로 나누기 에러 케이스
        auto res_zero = calc.divide(50, 0); // 반환값 <int, std::string>
        if (!res_zero.has_value()) { // 실패

            auto error_msg = res_zero.error(); // 실패인 경우 std::string 타입
            assert(error_msg == "Cannot divide by 0."); // 에러 메시지 확인

            auto value_or_result = res_zero.value_or(-1);
            assert(value_or_result == -1);
            // res_zero에 값이 있으면 값을 반환하지만,
            // 현재 값이 없어서 정의된 기본값(-1)으로 반환됨.

            println("[INFO] 1-2. const 멤버 함수 에러(0 분모): {}", error_msg);
        }
        else {
            eprintln("[ERROR] 1-2. const 멤버 함수 에러(0 분모) 실패");
            return 12;
        }

        // 1-3. 한계치 초과 에러 케이스
        auto res_limit = calc.divide(500, 2); // 반환값 <int, std::string> (500/2 = 250 > 100 한계치)
        if (!res_limit.has_value()) { // 실패

            auto error_msg = res_limit.error(); // 실패인 경우 std::string 타입
            assert(error_msg == "The calculation result exceeded the limit."); // 에러 메시지 확인

            auto value_or_result = res_limit.value_or(-1);
            assert(value_or_result == -1);
            // res_limit에 값이 있으면 값을 반환하지만,
            // 현재 값이 없고 정의된 기본값(-1)으로 반환됨.
            
            println("[INFO] 1-3. const 멤버 함수 에러(한계 초과): {}", error_msg);
        }
        else {
            eprintln("[ERROR] 1-3. const 멤버 함수 에러(한계 초과) 실패");
            return 13;
        }
    }

    // -------------------------------------------------------------
    // [테스트 2] Enum 에러 타입 멤버 함수 테스트 (safe_sqrt)
    // -------------------------------------------------------------
    {
        // 2-1. 정상 제곱근
        auto res_ok = calc.safe_sqrt(16.0); // 반환값 <double, MathError>   
        if (res_ok.has_value()) {

            auto result = res_ok.value(); // 성공이면 double 타입
            assert(result == 4.0); // 값 확인

            println("[INFO] 2-1. Enum 에러 타입 결과: {}", result);
        } else {
            eprintln("[ERROR] 2-1. Enum 에러 타입 실패");
            return 21;
        }

        // 2-2. 음수 제곱근 에러
        auto res_err = calc.safe_sqrt(-9.0);  // 반환값 <double, MathError> (-9.0은 음수이므로 에러 발생)
        if (!res_err.has_value()) { // 실패

            auto error_enum = res_err.error(); // 실패이면 MathError 타입
            assert(error_enum == MathError::NegativeSquareRoot); // Enum 타입 에러 확인

            println("[INFO] 2-2. Enum 에러 타입 에러: NegativeSquareRoot");
        } else {
            eprintln("[ERROR] 2-2. Enum 에러 타입 실패");
            return 22;
        }
    }

    // -------------------------------------------------------------
    // [테스트 3] 객체 상태 변경 멤버 함수 테스트 (add_to_history)
    // -------------------------------------------------------------
    {
        assert(calc.history().empty()); // 초기 히스토리 비어있음

        // 히스토리 추가 성공
        auto res1 = calc.add_to_history(20, 2); // 반환값 <int, std::string> (20 나누기 2 = 10)    
        if (res1.has_value()) { // 성공
            auto result = res1.value(); // 성공인 경우 int 타입
            assert(res1.has_value() && result == 10); // 값 있음 (값: 10)

            assert(calc.history().size() == 1); // 히스토리 크기 1
            assert(calc.history()[0] == 10); // 첫 번째 히스토리 값 10

            println("[INFO] 3-1. 상태 변경 멤버 함수 결과: {}", result);
        } else {
            eprintln("[ERROR] 3-1. 상태 변경 멤버 함수 실패");
            return 31;
        }
 
        // 히스토리 추가 실패 
        auto res2 = calc.add_to_history(20, 0); // 반환값 <int, std::string> (20 나누기 0 = 에러)
        if (!res2.has_value()) { // 실패
            auto error_msg = res2.error(); // 실패인 경우 std::string 타입
            assert(error_msg == "Cannot divide by 0."); // 에러 메시지 확인

            assert(calc.history().size() == 1); // 실패이므로 히스토리 크기는 변하지 않음.

            println("[INFO] 3-2. 상태 변경 멤버 함수 에러: {}", error_msg);
        } else {
            eprintln("[ERROR] 3-2. 상태 변경 멤버 함수 실패");
            return 32;
        }

        println("[PASS] 3. 상태 변경 멤버 함수 및 히스토리 누적 검증 완료");
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
        assert(res_empty.error() == "This is an empty string."); // 에러 메시지 확인

        // 4-3. 잘못된 문자로 인한 실패
        auto res_invalid = MathCalculator::parse_int("abc");
        assert(!res_invalid.has_value()); // 값 없음
        assert(res_invalid.error() == "Cannot convert to integer."); // 에러 메시지 확인

        println("[PASS] 4. static 멤버 함수 검증 완료");
    }

    println("=== 모든 클래스 멤버 함수 검증(assert)을 통과했습니다 ===");

    return 0;
}
