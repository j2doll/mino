#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 1. command_line 테스트
// ==========================================
void test_command_line() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    namespace mcsys = mino::core::system;
    using command_line = mcsys::command_line;

    print(tce("[TEST] 1. command_line 테스트 시작"));

    command_line cmd;
    cmd.set_version("1.0.0"); // 버전 설정

    // 옵션 등록: long_name, short_name, requires_value, description
    cmd.add_option("output", 'o', true, "출력 파일 경로 지정");
    cmd.add_option("verbose", 'v', false, "상세 로그 활성화");
    cmd.add_option("threads", 't', true, "스레드 개수");
    cmd.add_option("force", 'f', false, "강제 실행 플래그");

    // 가상의 커맨드라인 인자 설정
    // ./my_app -o result.txt --threads=4 -f input1.txt input2.txt
    std::vector<std::string> args = {
        "my_app",
        "-o", "result.txt",
        "--threads=4",
        "-f",
        "input1.txt",
        "input2.txt"
    };

    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.data()));
    }
    int argc = static_cast<int>(argv.size());

    bool parse_ok = cmd.parse(argc, argv.data()); // 파싱 수행
    assert(parse_ok && "command_line::parse failed!"); // 파싱 결과 확인

    // 옵션 정의 열거 및 출력
    std::cout << std::endl;
    for (const auto& d : cmd.options()) {
        auto long_name = d.long_name.empty() ? "(no long name)" : d.long_name;
        auto short_name = std::string(1, (d.short_name ? d.short_name : '-'));
        auto requires_value = d.requires_value ? "true" : "false";
        auto desc = d.description.empty() ? "(no description)" : d.description;
        std::cout
            << "\t"
            << tce(long_name)
            << ", short: " << tce(short_name)
            << ", requires_value: " << std::boolalpha << requires_value
            << ", desc: " << tce(desc)
            << std::endl;
    }
    std::cout << std::endl;

    // has 및 get 검증
    assert(cmd.has("output") && "output option should exist"); // 존재 여부 확인
    assert(cmd.get("output") == "result.txt"); // 값 확인

    assert(cmd.has("threads") && "threads option should exist"); // 존재 여부 확인
    assert(cmd.get("threads") == "4"); // 값 확인

    assert(cmd.has("force") && "force flag should exist"); // 존재 여부 확인
    assert(cmd.get("force") == "1"); // 값 확인

    // 기본값 동작 검증
    assert(!cmd.has("non_existent")); // 존재하지 않는 옵션 확인
    assert(cmd.get("non_existent", "default_val") == "default_val"); // 기본값 확인


    // 위치 인자(positionals) 검증
    auto positionals = cmd.positional(); // 위치 인자 반환
    for (auto& pos : positionals) {
        print(tce("  -> 위치 인자: "), tce(pos));
    }
    std::cout << std::endl;

    assert(positionals.size() == 2);
    assert(positionals[0] == "input1.txt");
    assert(positionals[1] == "input2.txt");

    // usage 문자열 생성 검증
    std::string usage_str = cmd.usage(); // Usage 문자열 생성
    assert(!usage_str.empty());
    print(tce("  -> 생성된 Usage 문자열 예시:\n"), tce(usage_str));
    // Usage: my_app [options] [args]
    // 
    // Options:
    //   -o, --output <value>  출력 파일 경로 지정
    //   -v, --verbose 상세 로그 활성화
    //   -t, --threads <value> 스레드 개수
    //   -f, --force   강제 실행 플래그
    //   -h, --help    Show this help message
    //   -v, --version Show version (1.0.0)

    print(tce("[PASS] command_line 테스트 완료\n"));
}
