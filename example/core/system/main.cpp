#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// ==========================================
// 1. command_line 테스트
// ==========================================
void test_command_line() {
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
        auto short_name = std::string( 1, (d.short_name ? d.short_name : '-') );
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

// ==========================================
// 2. device_id_generator 테스트
// ==========================================
void test_device_id_generator() {
    namespace mcsys = mino::core::system;
    using device_id_generator = mcsys::device_id_generator;

    print(tce("[TEST] 2. device_id_generator 테스트 시작"));

    std::string id1 = device_id_generator::get_unique_id(); // 장비 고유 ID 생성
    std::string id2 = device_id_generator::get_unique_id();

    print(tce("  -> 생성된 Device ID: "), id1,
        tce(" (길이: "), std::to_string(id1.length()), tce(")"));

    // 64자리 16진수 문자열인지 검증
    assert(id1.length() == 64 && "Device ID must be 64 characters long");
    assert(id1 == id2 && "Consecutive get_unique_id calls on the same machine must match");

    print(tce("[PASS] device_id_generator 테스트 완료\n"));
}

// ==========================================
// 3. resource_monitor 테스트
// ==========================================
void test_resource_monitor() {
    namespace mcsys = mino::core::system;
    using resource_monitor = mcsys::resource_monitor;

    print(tce("[TEST] 3. resource_monitor 테스트 시작"));

    // CPU 사용률 테스트 (이전 호출 대비 차이를 계산하므로 약간의 딜레이 후 2회 호출)
    double cpu_usage1 = resource_monitor::get_cpu_usage(); // 전체 CPU 사용률 첫 번째 샘플
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 잠시 대기

    double cpu_usage2 = resource_monitor::get_cpu_usage(); // 전체 CPU 사용률 두 번째 샘플
    print(tce("  -> 전체 CPU 사용률: "), cpu_usage2,
        tce("% (이전 샘플: "), cpu_usage1, tce("%)"));
    assert(cpu_usage2 >= 0.0 && cpu_usage2 <= 100.0);

    // 코어별 CPU 사용률 테스트
    auto core_usages = resource_monitor::get_cpu_usage_per_core(); // 코어별 CPU 사용률 첫 번째 샘플
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 잠시 대기
    core_usages = resource_monitor::get_cpu_usage_per_core(); // 코어별 CPU 사용률 두 번째 샘플

    print(tce("  -> 코어 개수: "), std::to_string(core_usages.size())); // 코어 개수 출력
    for (size_t i = 0; i < core_usages.size(); ++i) {
        print(tce("     - Core "), std::to_string(i), tce(": "), core_usages[i], tce("%"));
        assert(core_usages[i] >= 0.0 && core_usages[i] <= 100.0);
    }

    // 메모리 정보 테스트
    mcsys::memory_info mem = resource_monitor::get_memory_info(); // 메모리 정보 수집
    print(tce("  -> 메모리 정보:"));
    print(tce("     - 총 물리 메모리: "), (mem.total_phys_kb / 1024), tce(" MB"));
    print(tce("     - 사용 가능 메모리: "), (mem.available_phys_kb / 1024), tce(" MB"));
    print(tce("     - 사용률: "), mem.usage_percent, tce("%"));
    assert(mem.total_phys_kb > 0);
    assert(mem.usage_percent >= 0.0 && mem.usage_percent <= 100.0);

    // 디스크 정보 테스트 (기본 루트 경로)
    mcsys::disk_info disk = resource_monitor::get_disk_info(); // 디스크 정보 수집
    print(tce("  -> 디스크 정보 ("), disk.mount_path, tce("):"));
    print(tce("     - 총 용량: "), disk.total_gb, tce(" GB"));
    print(tce("     - 여유 공간: "), disk.free_gb, tce(" GB"));
    print(tce("     - 사용률: "), disk.usage_percent, tce("%"));
    assert(disk.total_gb > 0);
    assert(disk.usage_percent >= 0.0 && disk.usage_percent <= 100.0);

    print(tce("[PASS] resource_monitor 테스트 완료\n"));
}

// ==========================================
// 4. process_runner 테스트
// ==========================================
void test_process_runner() {
    namespace mcsys = mino::core::system;
    using process_runner = mcsys::process_runner;
    using process_result = mcsys::process_result;
    using process_status = mcsys::process_status;

    print(tce("[TEST] 4. process_runner 테스트 시작"));

    process_runner runner; // 프로세스 실행기 인스턴스 생성

#ifdef _WIN32
    std::string success_cmd = "cmd.exe /c exit 0";
    std::string fail_cmd = "cmd.exe /c exit 42";
    std::string timeout_cmd = "powershell -Command Start-Sleep -Seconds 5";
#else
    std::string success_cmd = "true";
    std::string fail_cmd = "sh -c 'exit 42'";
    std::string timeout_cmd = "sleep 5";
#endif

    // 1) 정상 종료 테스트 (exit code 0)
    process_result res_ok = runner.run_process(success_cmd); // 정상 종료 명령 실행
    print(tce("  -> 정상 실행 테스트: status="), static_cast<int>(res_ok.status),
        tce(", exit_code="), res_ok.exit_code);
    assert(res_ok.status == process_status::success); // 상태 확인
    assert(res_ok.exit_code == 0);

    // 2) 비정상 종료 테스트 (exit code != 0)
    process_result res_fail = runner.run_process(fail_cmd); // 비정상 종료 명령 실행
    print(tce("  -> 비정상 종료 테스트: status="), static_cast<int>(res_fail.status),
        tce(", exit_code="), res_fail.exit_code);
    assert(res_fail.status == process_status::abnormal_exit); // 상태 확인
    assert(res_fail.exit_code == 42);

    // 3) 타임아웃 테스트 (100ms 제한)
    auto timeout_ms = std::chrono::milliseconds(100); // 100ms 제한
    process_result res_timeout = runner.run_process(timeout_cmd, timeout_ms); // 타임아웃 명령 실행
    print(tce("  -> 타임아웃 테스트: status="), static_cast<int>(res_timeout.status),
        tce(", exit_code="), res_timeout.exit_code);
    assert(res_timeout.status == process_status::timeout); // 상태 확인

    print(tce("[PASS] process_runner 테스트 완료\n"));
}

// ==========================================
// 5. crash_handler 테스트 (초기화 및 스택 트레이스)
// ==========================================
void test_crash_handler() {
    namespace mcsys = mino::core::system;
    using crash_handler = mcsys::crash_handler;

    print(tce("[TEST] 5. crash_handler 테스트 시작"));

    // 초기화 함수 호출 검증 (커스텀 콜백 등록)
    crash_handler::initialize([](const std::string& crash_log) {
        eprint(tce("[CALLBACK] Crash occurred with log:\n"), tce(crash_log));
    });

    // 현재 위치에서의 콜 스택 트레이스 수집 검증
    std::vector<std::string> stack_trace = crash_handler::get_stack_trace(); // 스택 트레이스 수집
    print(tce("  -> 현재 콜 스택 프레임 수: "), std::to_string(stack_trace.size()));
    for (size_t i = 0; i < stack_trace.size(); ++i) {
        print(tce("     "), tce(stack_trace[i]));
    }
    assert(!stack_trace.empty() && "Stack trace should not be empty");

    print(tce("[PASS] crash_handler 테스트 완료\n"));
}

// ==========================================
// main 함수
// ==========================================
int main() {
    print(tce("========================================"));
    print(tce(" mino::core::system 단위 테스트 시작"));
    print(tce("========================================\n"));

    test_command_line();
    test_device_id_generator();
    test_resource_monitor();
    test_process_runner();
    test_crash_handler();

    print(tce("========================================"));
    print(tce(" 모든 테스트가 성공적으로 통과되었습니다."));
    print(tce("========================================"));

    return 0;
}
