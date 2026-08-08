#include <iostream>
#include <thread>
#include <chrono>
#include <limits>

#include "mino/core/daemon/daemon.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== Termination Handler 테스트 시작 ===")
        << std::endl;

    // 1. 싱글톤 인스턴스 참조 획득
    auto& handler = mino::core::daemon::termination_handler::get_instance();

    // 2. 종료 시 수행할 콜백 함수 등록
    handler.set_callback([to_console_encoding]() {
        std::cout
            << to_console_encoding("\n[Callback Executed] 종료 신호(SIGINT/SIGTERM 또는 Console Ctrl Event)를 감지했습니다.")
            << std::endl;
        std::cout
            << to_console_encoding("[Callback Executed] 안전하게 자원을 정리합니다...")
            << std::endl;
        std::exit(0); 
    });

    // 3. OS 신호/콘솔 핸들러 초기화
    handler.initialize();

    std::cout
        << to_console_encoding("핸들러가 성공적으로 초기화되었습니다.")
        << std::endl;
    std::cout
        << to_console_encoding("테스트를 위해 터미널에서 Ctrl+C 를 누르거나 프로세스를 종료하세요.\n")
        << std::endl;

    // 4. 신호 수신 대기를 위한 메인 루프
    int sec = 0;
    while (true) {
        if (sec == std::numeric_limits<int>::max()) {
            sec = 0; // 초 카운터를 초기화
        }
        std::cout
            << to_console_encoding("프로그램 동작 중... (")
            << ++sec << to_console_encoding("초)")
            << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
