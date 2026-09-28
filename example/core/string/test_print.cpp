#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>

#include "mino/core/string/string.hpp"

namespace mcsp = mino::core::string::print;
using namespace std::string_view_literals;

// =============================================================================
// std::cout / std::cerr 캡처 헬퍼 (자동 검증용 스트림 버퍼 리디렉션)
// =============================================================================
struct CoutCapture {
    std::stringstream buffer;
    std::streambuf* old = nullptr;

    CoutCapture()
        : old(std::cout.rdbuf(buffer.rdbuf())) // std::cout의 버퍼를 stringstream buffer로 리디렉션
    {
    }

    ~CoutCapture() {
        std::cout.rdbuf(old); // std::cout의 버퍼를 원래대로 복원
    }

    std::string str() const { // 캡처된 cout 내용을 문자열로 반환
        return buffer.str();
    }

    void clear() {
        buffer.str("");
        buffer.clear();
    }
};

struct CerrCapture {
    std::ostringstream buffer;
    std::streambuf* old_buf;

    CerrCapture() : old_buf(std::cerr.rdbuf(buffer.rdbuf())) {}
    ~CerrCapture() { std::cerr.rdbuf(old_buf); }

    // ANSI 이스케이프 코드를 제거한 순수 텍스트 반환
    std::string str() const {
        std::string s = buffer.str();
        const std::string red = "\033[31m";
        const std::string reset = "\033[0m";

        size_t pos = 0;
        while ((pos = s.find(red)) != std::string::npos) s.erase(pos, red.length());
        while ((pos = s.find(reset)) != std::string::npos) s.erase(pos, reset.length());
        return s;
    }

    void clear() {
        buffer.str("");
        buffer.clear();
    }
};

// =============================================================================
// 1. 순차적 위치 기반 포맷팅 ({}) 테스트 (print, println)
// =============================================================================
void test_sequential_formatting() {
    mcsp::println("[Test 1] 순차적 위치 기반 포맷팅 ({}) 검증 중...");
    CoutCapture cap;

    // (1) 기본 문자열 및 정수 대입
    mcsp::print("Hello, {}! You have {} messages.", "Alice", 3);
    assert(cap.str() == "Hello, Alice! You have 3 messages.");
    cap.clear();

    // (2) println 개행 검증
    mcsp::println("Value: {}", 42);
    assert(cap.str() == "Value: 42\n");
    cap.clear();

    // (3) 다양한 타입(bool, double, std::string, string_view) 검증
    bool flag = true;
    double pi = 3.14;
    std::string s = "std_string";
    std::string_view sv = "string_view"sv;

    mcsp::print("bool: {}, double: {}, s: {}, sv: {}", flag, pi, s, sv);
    assert(cap.str() == "bool: true, double: 3.14, s: std_string, sv: string_view");
    cap.clear();
}

// =============================================================================
// 2. 이름 기반 포맷팅 ({key}) - 다양한 타입 지원 검증 (printss, printlnss)
// =============================================================================
void test_named_various_types() {
    mcsp::println("[Test 2] 이름 기반 포맷팅 ({key}) 다양한 타입 검증 중...");
    CoutCapture cap;

    int age = 25;
    double score = 98.5;
    bool is_student = false;
    char grade = 'A';
    std::string city = "Seoul";
    std::string_view view_country = "Korea"sv;

    // primitive, const char*, string, string_view 혼용 검증
    mcsp::printlnss(
        "{name} ({age}, {grade}) - Score: {score}, Student: {student}, Loc: {city}, {country}",
        {
            {"name", "John"},
            {"age", age},
            {"grade", grade},
            {"score", score},
            {"student", is_student},
            {"city", city},
            {"country", view_country}
        }
    );

    std::string expected = "John (25, A) - Score: 98.5, Student: false, Loc: Seoul, Korea\n";
    assert(cap.str() == expected);
    cap.clear();
}

// =============================================================================
// 3. 키-값 컨테이너 지원 검증 (std::map, std::unordered_map)
// =============================================================================
void test_container_support() {
    mcsp::println("[Test 3] 키-값 컨테이너 지원 검증 중...");
    CoutCapture cap;

    // (1) std::map<std::string, std::string>
    std::map<std::string, std::string> env_map = {
        {"HOST", "localhost"},
        {"PORT", "8080"}
    };
    mcsp::printss("Connecting to {HOST}:{PORT}", env_map);
    assert(cap.str() == "Connecting to localhost:8080");
    cap.clear();

    // (2) std::unordered_map<std::string, int>
    std::unordered_map<std::string, int> count_map = {
        {"apple", 5},
        {"banana", 12}
    };
    mcsp::printlnss("Inventory - Apple: {apple}, Banana: {banana}", count_map);
    assert(cap.str() == "Inventory - Apple: 5, Banana: 12\n");
    cap.clear();
}

// =============================================================================
// 4. 예외 및 엣지 케이스 처리 검증
// =============================================================================
void test_edge_cases() {
    mcsp::println("[Test 4] 이스케이프 및 엣지 케이스 검증 중...");
    CoutCapture cap;

    // (1) 중괄호 이스케이프 "{{ -> {"
    mcsp::printss("Escaped: {{name}} is {name}", { {"name", "John"} });
    assert(cap.str() == "Escaped: {name} is John");
    cap.clear();

    // (2) 매칭되는 키가 없을 때 원본 {key} 유지
    mcsp::printss("Found: {name}, Missing: {unknown_key}", { {"name", "John"} });
    assert(cap.str() == "Found: John, Missing: {unknown_key}");
    cap.clear();

    // (3) 동일한 키가 여러 번 등장할 때
    mcsp::printss("{v} + {v} = {result}", { {"v", 10}, {"result", 20} });
    assert(cap.str() == "10 + 10 = 20");
    cap.clear();

    // (4) 닫는 괄호가 없는 불완전한 포맷
    mcsp::printss("Incomplete: {name, and {name}", { {"name", "John"} });
    assert(cap.str() == "Incomplete: {name, and John");
    cap.clear();
}

// =============================================================================
// 5. 표준 에러 출력 (eprint, eprintln, eprintss, eprintlnss) 검증
// =============================================================================
void test_stderr_output() {
    mcsp::println("[Test 5] 표준 에러(std::cerr) 출력 검증 중...");
    CerrCapture cap;

    mcsp::eprint("[ERROR] Code: {}", 500);
    assert(cap.str() == "[ERROR] Code: 500");
    cap.clear();

    mcsp::eprintln("[WARN] {}", "Disk almost full");
    assert(cap.str() == "[WARN] Disk almost full\n");
    cap.clear();

    mcsp::eprintss("[{level}] {msg} at {file}:{line}", {
        {"level", "FATAL"},
        {"msg", "Null pointer"},
        {"file", "main.cpp"},
        {"line", 42}
        });
    assert(cap.str() == "[FATAL] Null pointer at main.cpp:42");
    cap.clear();

    mcsp::eprintlnss("[{level}] Done", { {"level", "INFO"} });
    assert(cap.str() == "[INFO] Done\n");
    cap.clear();
}

// =============================================================================
// 6. 실제 콘솔 출력 시각 검증 (한글 및 인코딩 포함)
// =============================================================================
void test_visual_output() {
    mcsp::println("\n=======================================================");
    mcsp::println("                 실제 콘솔 출력 결과                   ");
    mcsp::println("=======================================================");

    std::string name = "홍길동";
    int age = 30;
    bool is_active = true;
    double balance = 12500.75;

    // (1) 순차 출력 (한글 포함)
    mcsp::println("[순차 출력] 안녕하세요, {}님! 나이: {}", name, age);

    // (2) 이름 기반 출력 (한글 포함)
    mcsp::printlnss(
        "[이름 기반 출력] {whose} 이름은 {name}이고, 나이는 {age}세입니다. (활성 상태: {status}, 잔고: {balance}원)",
        {
            {"whose", "저의"},
            {"name", name},
            {"age", age},
            {"status", is_active},
            {"balance", balance}
        }
    );

    // (3) 에러 출력
    mcsp::eprintlnss("[에러 스트림 테스트] 레벨: {level}, 메시지: {msg}", {
        {"level", "CRITICAL"},
        {"msg", "서버 응답 없음"}
        });

    mcsp::println("=======================================================");
}

void test_print() {
    mcsp::println("=== print.hpp 기능 테스트 시작 ===");

    test_sequential_formatting();
    test_named_various_types();
    test_container_support();
    test_edge_cases();
    test_stderr_output();
    test_visual_output();

    mcsp::println("\n[모든 테스트 완료] print.hpp 관련 테스트가 성공적으로 완료되었습니다.");
}
