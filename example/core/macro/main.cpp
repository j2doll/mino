#include <iostream>
#include <string>
#include <optional>

#include "mino/core/macro/setter_getter.hpp"
#include "mino/core/macro/try_opt.hpp"

#include "mino/core/string/to_console_encoding.hpp"

// -----------------------------------------------------------------------------
// 1. DEFINE_SETTER_GETTER 테스트용 구조체
// -----------------------------------------------------------------------------
struct Config {
    DEFINE_SETTER_GETTER(std::string, name)
    DEFINE_SETTER_GETTER(int, width)
    DEFINE_SETTER_GETTER(int, height)
    DEFINE_SETTER_GETTER(bool, fullscreen)
};

// -----------------------------------------------------------------------------
// 2. TRY_OPT 계열 매크로 테스트 함수들
// -----------------------------------------------------------------------------

// optional<타입>을 반환하는 테스트용 함수
std::optional<int> get_value(bool success) {
    if (success)
        return 42;
    return std::nullopt;
}

// TRY_OPT(값(VAR), 표현식(EXPR))
//      표현식(EXPR)이 optional<T> 타입을 반환하는 함수일 때,
//      값(VAR)이 없으면 즉시 return {}; 하고,
//      값(VAR)이 있으면 VAR에 바인딩합니다.
std::optional<int> test_try_opt(bool success) {
    TRY_OPT(val, get_value(success)); // success가 false인 경우, get_value()는 return {} 이 되고, TRY_OPT는 현재 라인에서 바로 return {}; 됩니다.
    return val + 10;
}

// TRY_OPT_NO_BIND(표현식(EXPR))
//      표현식(EXPR)이 optional<T> 타입을 반환하는 함수일 때,
//      값(VAR)이 없으면 즉시 return {}; 하고,
//      값(VAR)이 있으면 그냥 통과합니다. (값은 바인딩하지 않음)
std::optional<int> test_try_opt_no_bind(bool success) {
    TRY_OPT_NO_BIND(get_value(success)); // success가 false인 경우, get_value()는 return {} 이 되고, TRY_OPT_NO_BIND는 현재 라인에서 바로 return {}; 됩니다.
    return 100;
}

// TRY_OPT_VOID(표현식(EXPR))
//     void 반환 함수에서 optional<T> 타입을 반환하는 표현식(EXPR)을 검사할 때,
//     값이 없으면 그냥 return; 하고,
//     값이 있으면 그냥 통과합니다. (값은 바인딩하지 않음)
void test_try_opt_void(bool success, int& output) {
    TRY_OPT_VOID(get_value(success)); // success가 false인 경우, get_value()는 return {} 이 되고, TRY_OPT_VOID는 현재 라인에서 바로 return;(void return) 됩니다. 
    output = 999;
}

// TRY_OPT_BIND_VOID(값(VAR), 표현식(EXPR))
//     void 반환 함수에서 optional<T> 타입을 반환하는 표현식(EXPR)을 검사할 때,
//     값(VAR)이 없으면 그냥 return; 하고,
//     값(VAR)이 있으면 VAR에 바인딩합니다.
void test_try_opt_bind_void(bool success, int& output) {
    TRY_OPT_BIND_VOID(val, get_value(success)); // success가 false인 경우, get_value()는 return {} 이 되고, TRY_OPT_BIND_VOID는 현재 라인에서 바로 return;(void return) 됩니다. 
    output = val;
}
 
// -----------------------------------------------------------------------------
// 2-1. TRY_OPT 복합 활용 예제 (한 함수 안에서 여러 번 사용)
// -----------------------------------------------------------------------------
// MINO_UNIQUE_NAME 이 __LINE__ 을 이용해 임시 변수명을 만들어주기 때문에,
// 같은 함수 안에서 TRY_OPT 를 여러 줄에 걸쳐 여러 번 써도 이름이 충돌하지 않습니다.

// optional<타입>을 반환하는 테스트용 함수 (보너스 점수)
std::optional<int> get_bonus(bool success) {
    if (success)
        return 8;
    return std::nullopt;
}

// 두 개의 optional 을 순차적으로 검사/바인딩 후 합산
// - base 실패 시: 즉시 return {}; (bonus 는 호출조차 되지 않음)
// - base 성공, bonus 실패 시: bonus 단계에서 return {};
std::optional<int> test_try_opt_multiple(bool base_ok, bool bonus_ok) {
    TRY_OPT(base, get_value(base_ok));    // 1차 검사/바인딩: get_value(base_ok)가 int를 반환하면 base에 값을 넣고, 이 라인을 넘어간다. 단, 실패하면 현재 라인에서 return {}; 된다.
    TRY_OPT(bonus, get_bonus(bonus_ok));  // 2차 검사/바인딩: get_bonus(bonus_ok)가 int를 반환하면 bonus에 값을 넣고, 이 라인을 넘어간다. 단, 실패하면 현재 라인에서 return {}; 된다.
    return base + bonus;
}
// NOTE: 복수 개의 TRY_OPT()에 적용할 함수는 std::optional<T> 타입을 반환하고, 각 함수들의 T는 동일한 타입이어야 합니다. 

// optional<타입>을 반환하는 테스트용 함수 (이름)
std::optional<std::string> get_first_name(bool ok) {
    if (ok)
        return std::string("Mino");
    return std::nullopt;
}

// optional<타입>을 반환하는 테스트용 함수 (성)
std::optional<std::string> get_last_name(bool ok) {
    if (ok)
        return std::string("Engine");
    return std::nullopt;
}

// 3단계 이상 체인도 동일한 패턴으로 확장 가능
std::optional<std::string> test_try_opt_chain(bool has_first, bool has_last) {
    TRY_OPT(first, get_first_name(has_first));
    TRY_OPT(last, get_last_name(has_last));
    return first + " " + last;
}

// TRY_OPT_NO_BIND(사전 검증) + TRY_OPT(실제 값 바인딩)을 섞어 쓰는 예제
std::optional<int> test_try_opt_mixed(bool guard_ok, bool value_ok) {
    TRY_OPT_NO_BIND(get_value(guard_ok));  // 값은 필요 없고 통과 여부만 확인
    TRY_OPT(val, get_value(value_ok));     // 이후 실제로 사용할 값을 바인딩
    return val * 2;
}

// -----------------------------------------------------------------------------
// 3. TRY_PTR 계열 매크로 테스트 함수들
// -----------------------------------------------------------------------------

// TRY_PTR(값(VAR), 표현식(EXPR))
//      표현식(EXPR)이 포인터 타입을 반환하는 함수일 때,
//      값(VAR)이 nullptr이면 즉시 return {}; 하고,
//      값(VAR)이 nullptr이 아니면 VAR에 바인딩합니다.
std::optional<int> test_try_ptr(int* ptr) {
    TRY_PTR(p, ptr); // ptr이 nullptr이면 현재 라인에서 return {}; 된다. ptr이 nullptr이 아니면 p에 바인딩되고, 이 라인을 넘어간다.
    return *p + 5;
}

// TRY_PTR_NO_BIND(표현식(EXPR))
//      표현식(EXPR)이 포인터 타입을 반환하는 함수일 때,
//      값(VAR)이 nullptr이면 즉시 return {}; 하고,
//      값(VAR)이 nullptr이 아니면 그냥 통과합니다. (값은 바인
std::optional<int> test_try_ptr_no_bind(int* ptr) {
    TRY_PTR_NO_BIND(ptr); // ptr이 nullptr이면 현재 라인에서 return {}; 된다. ptr이 nullptr이 아니면 그냥 통과한다.
    return 200;
}

// TRY_PTR_VOID(표현식(EXPR))
//     void 반환 함수에서 포인터 타입을 반환하는 표현식(EXPR)을 검사할 때
//     값이 nullptr이면 그냥 return; 하고,
//     값이 nullptr이 아니면 그냥 통과합니다. (값은 바인딩하지 않음)
void test_try_ptr_void(int* ptr, int& output) {
    TRY_PTR_VOID(ptr); // ptr이 nullptr이면 현재 라인에서 return; 된다. ptr이 nullptr이 아니면 그냥 통과한다. 
    output = 888;
}

// TRY_PTR_BIND_VOID(값(VAR), 표현식(EXPR))
//     void 반환 함수에서 포인터 타입을 반환하는 표현식(EXPR)을 검사할 때
//     값이 nullptr이면 그냥 return; 하고,
//     값이 nullptr이 아니면 VAR에 바인딩합니다.
void test_try_ptr_bind_void(int* ptr, int& output) {
    TRY_PTR_BIND_VOID(p, ptr); // ptr이 nullptr이면 현재 라인에서 return; 된다. ptr이 nullptr이 아니면 p에 바인딩되고, 이 라인을 넘어간다.
    output = *p;
}

// -----------------------------------------------------------------------------
// 3-1. TRY_PTR 복합 활용 예제 (한 함수 안에서 여러 번 사용 / TRY_OPT와 혼용)
// -----------------------------------------------------------------------------

// 두 개의 포인터를 순차적으로 검사/바인딩
// - a 가 nullptr 이면 즉시 return {}; (b 는 검사되지 않음)
std::optional<int> test_try_ptr_multiple(int* a, int* b) {
    TRY_PTR(pa, a);
    TRY_PTR(pb, b);
    return *pa + *pb;
}

// TRY_PTR 과 TRY_OPT 를 한 함수 안에서 함께 사용하는 예제
std::optional<int> test_try_ptr_and_opt(int* ptr, bool bonus_ok) {
    TRY_PTR(p, ptr);                      // 포인터 nullptr 검사
    TRY_OPT(bonus, get_bonus(bonus_ok));  // optional 값 검사/바인딩
    return *p + bonus;
}

// -----------------------------------------------------------------------------
// Main 실행 함수
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== 1. DEFINE_SETTER_GETTER 테스트 ===")
        << std::endl;

    Config cfg;

    // 기본 초기화 값 확인
    std::cout
        << to_console_encoding("[기본값] name: '") << to_console_encoding(cfg.name())
        << to_console_encoding("', width: ") << cfg.width()
        << to_console_encoding(", fullscreen: ") << std::boolalpha << cfg.fullscreen()
        << std::endl;
    // 출력 값
    // [기본값] name: '', width : 0, fullscreen : false

    // Setter 및 체이닝 테스트
    cfg.name("Mino Engine")
        .width(1920)
        .height(1080)
        .fullscreen(true);

    std::cout
        << to_console_encoding("[설정후] name: ") << to_console_encoding(cfg.name())
        << to_console_encoding(", width: ") << cfg.width()
        << to_console_encoding(", height: ") << cfg.height()
        << to_console_encoding(", fullscreen: ") << std::boolalpha << cfg.fullscreen()
        << std::endl << std::endl;
    // 출력 값
    // [설정후] name : Mino Engine, width : 1920, height : 1080, fullscreen : true

    // 1-1. 추가 활용: getter가 참조를 반환하므로 대입 연산으로 직접 수정 가능
    cfg.width() += 100; // non-const getter 참조를 이용한 직접 수정

    // 1-2. 추가 활용: rvalue 세터(std::move 오버로드) 사용
    std::string moved_name = "Moved Engine";
    cfg.name(std::move(moved_name)); // moved_name은 이후 사용 불가(이동됨)

    std::cout
        << to_console_encoding("[참조수정+이동세터] name: ") << to_console_encoding(cfg.name())
        << to_console_encoding(", width: ") << cfg.width()
        << std::endl << std::endl;
    // 출력 값
    // [참조수정+이동세터] name: Moved Engine, width: 2020

    std::cout
        << to_console_encoding("=== 2. TRY_OPT 계열 테스트 ===") << std::endl;

    std::cout
        << to_console_encoding("TRY_OPT (성공 시 +10): ")
        << test_try_opt(true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT (성공 시 +10): 52

    std::cout
        << to_console_encoding("TRY_OPT (실패 시 nullopt): ")
        << test_try_opt(false).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT (실패 시 nullopt): -1

    std::cout
        << to_console_encoding("TRY_OPT_NO_BIND (성공): ")
        << test_try_opt_no_bind(true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT_NO_BIND (성공): 100

    std::cout
        << to_console_encoding("TRY_OPT_NO_BIND (실패): ")
        << test_try_opt_no_bind(false).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT_NO_BIND (실패): -1

    int void_out = 0;

    test_try_opt_void(true, void_out);
    std::cout
        << to_console_encoding("TRY_OPT_VOID (성공 시 값 변경): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_OPT_VOID (성공 시 값 변경): 999

    void_out = 0;
    test_try_opt_void(false, void_out);
    std::cout
        << to_console_encoding("TRY_OPT_VOID (실패 시 값 유지): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_OPT_VOID (실패 시 값 유지): 0

    test_try_opt_bind_void(true, void_out);
    std::cout
        << to_console_encoding("TRY_OPT_BIND_VOID (성공 시 바인딩값 저장): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_OPT_BIND_VOID (성공 시 바인딩값 저장): 42

    void_out = 0;
    test_try_opt_bind_void(false, void_out);
    std::cout
        << to_console_encoding("TRY_OPT_BIND_VOID (실패 시 값 유지): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_OPT_BIND_VOID (실패 시 값 유지): 0

    std::cout
        << to_console_encoding("=== 2-1. TRY_OPT 복합 활용 테스트 ===") << std::endl;

    std::cout
        << to_console_encoding("TRY_OPT 여러 번 (둘 다 성공): ")
        << test_try_opt_multiple(true, true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT 여러 번 (둘 다 성공): 50  (42 + 8)

    std::cout
        << to_console_encoding("TRY_OPT 여러 번 (base 실패): ")
        << test_try_opt_multiple(false, true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT 여러 번 (base 실패): -1

    std::cout
        << to_console_encoding("TRY_OPT 여러 번 (bonus 실패): ")
        << test_try_opt_multiple(true, false).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT 여러 번 (bonus 실패): -1

    std::cout
        << to_console_encoding("TRY_OPT 체인 (둘 다 성공): ")
        << to_console_encoding(test_try_opt_chain(true, true).value_or("실패")) << std::endl;
    // 출력 값
    // TRY_OPT 체인 (둘 다 성공): Mino Engine

    std::cout
        << to_console_encoding("TRY_OPT 체인 (last 실패): ")
        << to_console_encoding(test_try_opt_chain(true, false).value_or("실패")) << std::endl;
    // 출력 값
    // TRY_OPT 체인 (last 실패): 실패

    std::cout
        << to_console_encoding("TRY_OPT_NO_BIND + TRY_OPT 혼용 (둘 다 성공): ")
        << test_try_opt_mixed(true, true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_OPT_NO_BIND + TRY_OPT 혼용 (둘 다 성공): 84

    std::cout
        << to_console_encoding("TRY_OPT_NO_BIND + TRY_OPT 혼용 (guard 실패): ")
        << test_try_opt_mixed(false, true).value_or(-1) << std::endl << std::endl;
    // 출력 값
    // TRY_OPT_NO_BIND + TRY_OPT 혼용 (guard 실패): -1

    std::cout
        << to_console_encoding("=== 3. TRY_PTR 계열 테스트 ===")
        << std::endl;

    int raw_val = 100;
    int* valid_ptr = &raw_val;
    int* null_ptr = nullptr;

    std::cout
        << to_console_encoding("TRY_PTR (유효한 포인터): ")
        << test_try_ptr(valid_ptr).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR (유효한 포인터): 105

    std::cout
        << to_console_encoding("TRY_PTR (nullptr): ")
        << test_try_ptr(null_ptr).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR (nullptr): -1

    std::cout
        << to_console_encoding("TRY_PTR_NO_BIND (유효한 포인터): ")
        << test_try_ptr_no_bind(valid_ptr).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR_NO_BIND (유효한 포인터): 200

    std::cout
        << to_console_encoding("TRY_PTR_NO_BIND (nullptr): ")
        << test_try_ptr_no_bind(null_ptr).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR_NO_BIND (nullptr): -1

    void_out = 0;
    test_try_ptr_void(valid_ptr, void_out);
    std::cout
        << to_console_encoding("TRY_PTR_VOID (유효한 포인터): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_PTR_VOID (유효한 포인터): 888

    void_out = 0;
    test_try_ptr_void(null_ptr, void_out);
    std::cout
        << to_console_encoding("TRY_PTR_VOID (nullptr): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_PTR_VOID (nullptr): 0

    void_out = 0;
    test_try_ptr_bind_void(valid_ptr, void_out);
    std::cout
        << to_console_encoding("TRY_PTR_BIND_VOID (유효한 포인터): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_PTR_BIND_VOID (유효한 포인터): 100

    void_out = 0;
    test_try_ptr_bind_void(null_ptr, void_out);
    std::cout
        << to_console_encoding("TRY_PTR_BIND_VOID (nullptr): ")
        << void_out << std::endl;
    // 출력 값
    // TRY_PTR_BIND_VOID (nullptr): 0

    std::cout
        << to_console_encoding("=== 3-1. TRY_PTR 복합 활용 테스트 ===") << std::endl;

    int raw_val_b = 50;
    int* valid_ptr_b = &raw_val_b;

    std::cout
        << to_console_encoding("TRY_PTR 여러 번 (둘 다 유효): ")
        << test_try_ptr_multiple(valid_ptr, valid_ptr_b).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR 여러 번 (둘 다 유효): 150

    std::cout
        << to_console_encoding("TRY_PTR 여러 번 (a가 nullptr): ")
        << test_try_ptr_multiple(null_ptr, valid_ptr_b).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR 여러 번 (a가 nullptr): -1

    std::cout
        << to_console_encoding("TRY_PTR + TRY_OPT 혼용 (둘 다 성공): ")
        << test_try_ptr_and_opt(valid_ptr, true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR + TRY_OPT 혼용 (둘 다 성공): 108

    std::cout
        << to_console_encoding("TRY_PTR + TRY_OPT 혼용 (포인터가 nullptr): ")
        << test_try_ptr_and_opt(null_ptr, true).value_or(-1) << std::endl;
    // 출력 값
    // TRY_PTR + TRY_OPT 혼용 (포인터가 nullptr): -1

    return 0;
}
