#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "mino/core/process_util/process_finder.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 프로세스 목록 출력을 돕는 헬퍼 함수
void print_processes(
    const std::string& title, 
    const std::vector<mino::core::process_util::process_info>& processes)
{
    namespace mcp = mino::core::process_util;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== ")
        << to_console_encoding(title)
        << to_console_encoding(" (총 ")
        << processes.size()
        << to_console_encoding("개) ===\n");

    std::cout
        << std::left
        << std::setw(10)
        << to_console_encoding("PID")
        << " | "
        << to_console_encoding("Process Name\n");

    std::cout << "----------------------------------------\n";

    // 결과가 너무 많을 수 있으므로 상위 10개만 출력합니다.
    size_t count = 0;
    for (const auto& proc : processes) {
        std::cout
            << std::left
            << std::setw(10)
            << proc.pid
            << " | "
            << to_console_encoding(proc.process_name)
            << "\n";

        if (++count >= 10 && processes.size() > 10) {
            std::cout
                << to_console_encoding("... (외 ")
                << (processes.size() - 10)
                << to_console_encoding("개 생략)\n");
            break;
        }
    }

    std::cout << "\n";
}

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;
    namespace mcp = mino::core::process_util;

    std::cout
        << to_console_encoding("[ Process Finder 테스트 시작 ]\n\n");

    // 1. 전체 활성 프로세스 목록 가져오기 테스트
    std::vector<mcp::process_info> active_procs = mcp::get_active_processes();
    print_processes("1. 활성화된 전체 프로세스 목록", active_procs);

    // OS별 테스트 키워드 설정
#if defined(_WIN32) || defined(_WIN64)
    std::string target = "svchost";
    std::string target_upper = "SVCHOST";
#else
    std::string target = "systemd";
    std::string target_upper = "SYSTEMD";
#endif

    // 2. [방법 A] 이미 확보한 프로세스 목록(active_procs) 내에서 검색
    std::cout
        << to_console_encoding("--- [방법 A] 전달받은 프로세스 목록 기반 검색 ---\n");

    // 2-1) 대소문자 구분 검색
    auto found_a_sensitive = mcp::get_processes_by_name(active_procs, target, true);
    print_processes("방법 A - 대소문자 구분 O (" + target + ")", found_a_sensitive);

    // 2-2) 대소문자 미구분 검색 (대문자로 검색해도 소문자 프로세스가 찾아지는지 테스트)
    auto found_a_insensitive = mcp::get_processes_by_name(active_procs, target_upper, false);
    print_processes("방법 A - 대소문자 구분 X (" + target_upper + ")", found_a_insensitive);


    // 3. [방법 B] 내부에서 get_active_processes()를 직접 호출하여 검색
    std::cout << to_console_encoding("--- [방법 B] 내부 직접 호출 검색 ---\n");
    auto found_b = mcp::get_processes_by_name(target, true);
    print_processes("방법 B - 직접 검색 (" + target + ")", found_b);

    // 4. 경계 조건 및 예외 케이스 테스트
    std::cout << to_console_encoding("--- [엣지 케이스 테스트] ---\n");

    // 4-1) 빈 문자열 검색 (구현부 로직상 전체 목록이 반환되어야 함)
    auto empty_search = mcp::get_processes_by_name(active_procs, "", true);
    std::cout
        << to_console_encoding("[결과] 빈 문자열 검색 시 반환 수: ")
        << empty_search.size()
        << to_console_encoding(" / 전체 수: ")
        << active_procs.size()
        << to_console_encoding(" (")
        << to_console_encoding((empty_search.size() == active_procs.size() ? "성공" : "실패"))
        << to_console_encoding(")\n");

    // 4-2) 존재하지 않는 프로세스명 검색
    std::string fake_process = "NonExistent_Process_123456";
    auto nonexistent = mcp::get_processes_by_name(active_procs, fake_process, false);
    std::cout
        << to_console_encoding("[결과] 존재하지 않는 프로세스 검색 결과 수: ")
        << nonexistent.size()
        << " ("
        << to_console_encoding((nonexistent.empty() ? "성공" : "실패"))
        << ")\n";

    std::cout
        << to_console_encoding("\n[ Process Finder 테스트 완료 ]\n");
    return 0;
}
