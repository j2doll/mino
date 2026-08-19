#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <limits>

#include "mino/core/string/to_console_encoding.hpp" // mino core lib class

#include "mino/external/json/json.hpp" // mino external json lib class

// ----------------------------------------------------------------------------
// 1. 노드 탐색 및 수정 함수 테스트 (get_node, get_node_mutable)
// ----------------------------------------------------------------------------
void test_get_node() {
    namespace mej = mino::external::json;

    nlohmann::json data = {
        {"key", "value"},
        {"nested",
            {
                {"val", 10}
            }
        }
    };

    nlohmann::json::json_pointer valid_ptr("/nested/val"); // 존재하는 경로
    nlohmann::json::json_pointer invalid_ptr("/nested/missing"); // 존재하지 않는 경로

    // [const 버전 get_node 검증]
    const nlohmann::json* node = mej::get_node(data, valid_ptr); // 존재하는 경로 얻기
    assert(node != nullptr && node->get<int>() == 10); // 값 검증
    assert(mej::get_node(data, invalid_ptr) == nullptr); // 존재하지 않는 경로는 nullptr 반환

    // [수정 가능 버전 get_node_mutable 검증]
    nlohmann::json* m_node = mej::get_node_mutable(data, valid_ptr); // 존재하는 경로 얻기. mutable은 수정 가능하다는 의미.
    assert(m_node != nullptr && node->get<int>() == 10); // 현재 /nested/val 값은 10
    *m_node = 20; // /nested/val 값을 20으로 수정
    assert(data["nested"]["val"] == 20);
    assert(mej::get_node_mutable(data, invalid_ptr) == nullptr);
}

// ----------------------------------------------------------------------------
// 2. 안전한 숫자 타입 변환 함수 테스트 (try_number_cast<T>)
// ----------------------------------------------------------------------------
void test_try_number_cast() {
    namespace mej = mino::external::json;

    nlohmann::json j_int = 42; // 정수 노드
    nlohmann::json j_float_int = 42.0; // 정수형 값이지만 float 타입으로 표현된 노드
    nlohmann::json j_float_frac = 42.5; // 소수점 포함 float 노드
    nlohmann::json j_str = "not_a_number"; // 문자열 노드
    nlohmann::json j_overflow = 1e20; // int 범위를 초과하는 큰 수 노드

    // NaN 및 Inf 노드 준비
    nlohmann::json j_nan = std::numeric_limits<double>::quiet_NaN(); // NaN 노드 (숫자 아님)
    nlohmann::json j_inf = std::numeric_limits<double>::infinity(); // Inf 노드 (무한대)

    int out_i = 0;
    long long out_ll = 0;
    double out_d = 0.0;

    // [int 타입 캐스팅 검증]
    assert(mej::try_number_cast<int>(j_int, out_i) && out_i == 42); // 정수 노드 j_int에서 int인 out_i로 변환 성공
    assert(mej::try_number_cast<int>(j_float_int, out_i) && out_i == 42); // 정수형 값이지만 float 타입인 j_float_int에서 int로 변환 성공
    assert(!mej::try_number_cast<int>(j_float_frac, out_i)); // 소수점 포함 시는 float 타입으로 변환 실패
    assert(!mej::try_number_cast<int>(j_str, out_i)); // 문자열 노드에서 int로 변환 실패
    assert(!mej::try_number_cast<int>(j_overflow, out_i));   // int 범위 초과 시 변환 실패
    assert(!mej::try_number_cast<int>(j_nan, out_i));        // NaN 거부 (숫자 아님)
    assert(!mej::try_number_cast<int>(j_inf, out_i));        // Inf 거부 (무한대)

    // [long long 타입 캐스팅 검증]
    assert(mej::try_number_cast<long long>(j_int, out_ll) && out_ll == 42LL); // 정수 노드 j_int에서 long long인 out_ll로 변환 성공

    // [double 타입 캐스팅 검증]
    assert(mej::try_number_cast<double>(j_int, out_d) && out_d == 42.0); // 정수 노드 j_int에서 double인 out_d로 변환 성공
    assert(mej::try_number_cast<double>(j_float_frac, out_d) && std::abs(out_d - 42.5) < 1e-6); // 소수점 포함 float 노드 j_float_frac에서 double로 변환 성공
    assert(!mej::try_number_cast<double>(j_nan, out_d));     // NaN 거부 (숫자 아님)
    assert(!mej::try_number_cast<double>(j_inf, out_d));     // Inf 거부 (무한대)
}

// ----------------------------------------------------------------------------
// 3. JSON Pointer 기반 기본값 fallback 함수 테스트 (value_or)
// ----------------------------------------------------------------------------
void test_value_or_pointer() {
    namespace mej = mino::external::json;

    nlohmann::json data = {
        {"str", "hello"},
        {"flag", true},
        {"num_i", 100},
        {"num_ll", 9007199254740991LL}, 
        {"num_d", 3.1415},
        {"null_val", nullptr}
    };
    // NOTE: 숫자는 double로 표현 가능한 값. 예: 9007199254740991 (2^53 - 1) 또는 그 이하 값.

    nlohmann::json::json_pointer p_str("/str"); // 존재하는 문자열 경로. "hello"
    nlohmann::json::json_pointer p_flag("/flag"); // 존재하는 bool 경로. true
    nlohmann::json::json_pointer p_num_i("/num_i"); // 존재하는 int 경로. 100
    nlohmann::json::json_pointer p_num_ll("/num_ll"); // 존재하는 long long 경로. 9007199254740991
    nlohmann::json::json_pointer p_num_d("/num_d"); // 존재하는 double 경로. 3.1415
    nlohmann::json::json_pointer p_null("/null_val"); // 존재하는 null 경로. nullptr
    nlohmann::json::json_pointer p_missing("/missing"); // 존재하지 않는 경로. nullptr 반환

    // [std::string 오버로드]
    assert(mej::value_or(data, p_str, std::string("def")) == "hello"); // 존재하는 문자열 경로. "hello"
    assert(mej::value_or(data, p_missing, std::string("def")) == "def"); // 존재하지 않는 경로. 기본값 "def" 반환
    assert(mej::value_or(data, p_num_i, std::string("def")) == "def"); // 타입 불일치 (int를 string으로 변환 불가)
    assert(mej::value_or(data, p_null, std::string("def")) == "def"); // null 처리. 기본값 "def" 반환

    // [bool 오버로드]
    assert(mej::value_or(data, p_flag, false) == true); // 존재하는 bool 경로. true 반환
    assert(mej::value_or(data, p_missing, false) == false); // 존재하지 않는 경로. 기본값 false 반환
    assert(mej::value_or(data, p_str, true) == true); // 타입 불일치 (string을 bool로 변환 불가). 기본값 true 반환
    assert(mej::value_or(data, p_null, true) == true); // null 처리. 기본값 true 반환

    // [template <class T> T value_or 숫자 오버로드]
    assert(mej::value_or<int>(data, p_num_i, -1) == 100); // 존재하는 int 경로. 100 반환
    assert(mej::value_or<int>(data, p_missing, -1) == -1); // 존재하지 않는 경로. 기본값 -1 반환
    assert(mej::value_or<int>(data, p_null, -1) == -1); // null 처리. 기본값 -1 반환

    assert(mej::value_or<long long>(data, p_num_ll, -1LL) == 9007199254740991LL); // 존재하는 long long 경로. 9007199254740991 반환
    assert(mej::value_or<long long>(data, p_missing, -1LL) == -1LL); // 존재하지 않는 경로. 기본값 -1LL 반환

    assert(std::abs(mej::value_or<double>(data, p_num_d, 0.0) - 3.1415) < 1e-6); // 존재하는 double 경로. 3.1415 반환
    assert(std::abs(mej::value_or<double>(data, p_missing, 1.0) - 1.0) < 1e-6); // 존재하지 않는 경로. 기본값 1.0 반환
}

// ----------------------------------------------------------------------------
// 4. 문자열 경로 기반 기본값 fallback 함수 테스트 (value_or_path)
// ----------------------------------------------------------------------------
void test_value_or_path() {
    namespace mej = mino::external::json;

    nlohmann::json data = {
        {"str", "world"},
        {"flag", false},
        {"num_i", 200},
        {"num_ll", 5000000000LL},
        {"num_d", 2.718}
    };

    // string path
    assert(mej::value_or_path(data, "/str", std::string("def")) == "world"); // 존재하는 문자열 경로. "world"
    assert(mej::value_or_path(data, "invalid_ptr_syntax", std::string("def")) == "def"); // 존재하지 않는 경로. 기본값 "def" 반환

    // bool path
    assert(mej::value_or_path(data, "/flag", true) == false); // 존재하는 bool 경로. false 반환
    assert(mej::value_or_path(data, "/none", true) == true); // 존재하지 않는 경로. 기본값 true 반환

    // template <class T> T value_or_path
    assert(mej::value_or_path<int>(data, "/num_i", 0) == 200); // 존재하는 int 경로. 200 반환
    assert(mej::value_or_path<long long>(data, "/num_ll", 0LL) == 5000000000LL); // 존재하는 long long 경로. 5000000000 반환
    assert(std::abs(mej::value_or_path<double>(data, "/num_d", 0.0) - 2.718) < 1e-6); // 존재하는 double 경로. 2.718 반환
    assert(mej::value_or_path<int>(data, "invalid_syntax", 999) == 999); // 존재하지 않는 경로. 기본값 999 반환
}

// ----------------------------------------------------------------------------
// 5. 외부 공개 API 및 확장 시나리오 테스트 (이스케이프, 배열, null, 범위 초과 등)
// ----------------------------------------------------------------------------
void test_external_api() {
    namespace mej = mino::external::json;

    // [1. 경로 존재 여부 및 특수문자 이스케이프 (~0, ~1) 테스트][cite: 4]
    nlohmann::json config_json = {
        {"config", {
            {"database",
                {
                    {"host", "db.local"},
                    {"port", 5432},
                    {"ssl", true}
                }
            },
            {"weird",
                {
                    {"a/b", 1}, // '/' 포함 키
                    {"c~d", 2}  // '~' 포함 키
                }
            }
        }}
    };

    // 경로 존재 여부 검증
    assert(mej::exists(config_json, "/config/database/host") == true);
    assert(mej::exists(config_json, "/config/database/port") == true);
    assert(mej::exists(config_json, "/config/database/ssl") == true);
    assert(mej::exists(config_json, "/config/database/user") == false);
    assert(mej::exists(config_json, "/config/does_not_exist") == false);
    assert(mej::exists(config_json, "/config/database/port/x") == false);

    // JSON Pointer 이스케이프 검증: ~1은 '/', ~0은 '~'
    assert(mej::exists(config_json, "/config/weird/a~1b") == true);
    assert(mej::exists(config_json, "/config/weird/c~0d") == true);
    assert(mej::exists(config_json, "/config/weird/a~0b") == false); // 잘못된 이스케이프

    // [2. 배열 인덱싱 테스트]
    const std::string arr_json_src = R"json({
        "arr": [10, 20, 30]
    })json";
    nlohmann::json arr_json;
    try {
        arr_json = nlohmann::json::parse(arr_json_src);
    }
    catch (const nlohmann::json::parse_error& e) {
        assert(false && "JSON parsing failed");
    }

    assert(mej::exists(arr_json, "/arr/0") == true);
    assert(mej::exists(arr_json, "/arr/2") == true);
    assert(mej::exists(arr_json, "/arr/3") == false); // 인덱스 범위 초과
    assert(mej::get_int(arr_json, "/arr/1", -1) == 20);

    // [3. 깊은 중첩 구조 및 null 노드 처리 테스트]
    nlohmann::json nested_json = {
        {"a", {
            {"b", {
                {"c", {
                    {"d", "leaf"},
                    {"n", nullptr}
                }}
            }}
        }}
    };
    assert(mej::get_string(nested_json, "/a/b/c/d", "x") == "leaf"); // 존재하는 문자열 노드
    assert(mej::get_string(nested_json, "/a/b/c/n", "def") == "def"); // null 노드는 기본값 반환
    assert(mej::get_string(nested_json, "/a/b/x", "missing") == "missing"); // 존재하지 않는 경로는 기본값 반환

    // [4. get_int / get_double 세부 타입 변환 및 경계값 검증]
    nlohmann::json num_json;
    num_json["port_int"] = 5432;
    num_json["port_float_int"] = 6000.0;
    num_json["port_float_frac"] = 6000.5;
    num_json["text"] = "7000";
    num_json["flag"] = true;
    num_json["beyond_int_max"] = 2147483648ull; // int 최댓값 초과
    num_json["val_nan"] = std::numeric_limits<double>::quiet_NaN();
    num_json["val_inf"] = std::numeric_limits<double>::infinity();

    // get_int
    assert(mej::get_int(num_json, "/port_int", 0) == 5432); // 정수 노드에서 int 변환 성공
    assert(mej::get_int(num_json, "/port_float_int", 0) == 6000); // 소수부 없는 float 노드에서 int 변환 성공
    assert(mej::get_int(num_json, "/port_float_frac", 1234) == 1234); // 소수부 있는 float 노드에서 int 변환 실패 후 기본값 반환
    assert(mej::get_int(num_json, "/text", 4321) == 4321); // 문자열 노드에서 int 변환 실패 후 기본값 반환
    assert(mej::get_int(num_json, "/flag", 1) == 1); // bool 노드에서 int 변환 실패 후 기본값 반환
    assert(mej::get_int(num_json, "/beyond_int_max", -1) == -1); // int 범위 초과 노드에서 int 변환 실패 후 기본값 반환

    // get_double
    assert(std::abs(mej::get_double(num_json, "/port_int", -1.0) - 5432.0) < 1e-6); // 정수 노드에서 double 변환 성공
    assert(mej::get_double(num_json, "/val_nan", 1.5) == 1.5); // NaN 노드에서 double 변환 실패 후 기본값 반환  
    assert(mej::get_double(num_json, "/val_inf", 2.5) == 2.5); // Inf 노드에서 double 변환 실패 후 기본값 반환 
    assert(mej::get_double(num_json, "/text", 9.9) == 9.9); // 문자열 노드에서 double 변환 실패 후 기본값 반환   

    // get_bool
    nlohmann::json bool_json = {
        {"enabled", true},
        {"flag_as_num", 1},
        {"flag_as_str", "true"}
    };

    assert(mej::get_bool(bool_json, "/enabled", false) == true); // 존재하는 bool 노드에서 true 반환
    assert(mej::get_bool(bool_json, "/flag_as_num", false) == false); // 숫자 노드에서 bool 변환 실패 후 기본값 false 반환    
    assert(mej::get_bool(bool_json, "/flag_as_str", false) == false); // 문자열 노드에서 bool 변환 실패 후 기본값 false 반환  
    assert(mej::get_bool(bool_json, "/missing", true) == true); // 존재하지 않는 경로에서 기본값 true 반환
}

// ----------------------------------------------------------------------------
// 메인 함수
// ----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding; // mino core 라이브러리 클래스
    print(tce("mino core 라이브러리 클래스 호출"));

    test_get_node();
    test_try_number_cast();
    test_value_or_pointer();
    test_value_or_path();
    test_external_api();

    std::cout << "All extended test cases passed successfully!" << std::endl;
    return 0;
}
