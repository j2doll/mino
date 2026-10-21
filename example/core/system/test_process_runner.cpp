#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 4. process_runner 테스트
// ==========================================
void test_process_runner() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

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

