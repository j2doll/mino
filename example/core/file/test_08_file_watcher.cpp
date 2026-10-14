#include "mino/core/file/file.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"

bool test_file_watcher(std::filesystem::path target) {
    namespace mcsp = mino::core::string::print;
    namespace mcs = mino::core::string;
    namespace mcf = mino::core::file;

    auto to_console_encoding = mcs::to_console_encoding;

    using file_watcher = mcf::file_watcher;
    using file_status = mcf::file_status;

    file_watcher watcher; // 파일/경로 감시자

    // 1. 감시 주기 설정
    if (!watcher.set_interval(std::chrono::milliseconds(500))) {
        mcsp::eprintln("유효하지 않은 감시 주기입니다.");
        return false;
    }

    // 2. 대상 경로 설정
    // std::filesystem::path target = "./mino_test_workspace";
    if (!watcher.set_target_path(target)) {
        mcsp::eprintln("경로가 존재하지 않거나 읽기 권한이 없습니다: {}", target.string());
        return false;
    }

    // 3. 감시 시작
    auto error_message = watcher.start([](const std::filesystem::path& path, file_status status) {
        switch (status) {
            case file_status::created: // 파일 생성 이벤트
                mcsp::println("[CREATED]  {}", path.string());
                break;
            case file_status::modified: // 파일 수정 이벤트
                mcsp::println("[MODIFIED] {}", path.string());
                break;
            case file_status::erased: // 파일 삭제 이벤트
                mcsp::println("[ERASED]   {}", path.string());
                break;
        }
    });

    if (error_message.has_value()) { // 감시 시작 실패 시 
        std::string error_msg = *error_message;
        mcsp::eprintln("Failed to start file watcher: {}", error_msg); // 에러 메시지 출력
        return false;
    }

    auto wtch_path = watcher.get_target_path(); // 감시 중인 경로 가져오기
    auto abs_path = std::filesystem::absolute(wtch_path); // 절대 경로 변환
    mcsp::println("감시 동작 중: {}", abs_path.string());

    // 10초간 대기 후 종료. 메인 루프 또는 쓰레드 루프로 대체하여 처리 가능.
    int count = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
        if (count >= 10)
            break;
        mcsp::println("감시 중... {}", count);
    }

    watcher.stop(); // 감시 종료
    mcsp::println("감시 종료 완료.");

    return true; 
}
