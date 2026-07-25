#pragma once

#include <string>
#include <chrono>

namespace mino::core::system {

    enum class process_status {
        success,            // 종료 코드 0으로 정상 종료
        timeout,            // 지정된 시간 초과로 강제 종료
        execution_failed,   // 프로세스 시작 실패
        abnormal_exit       // 0이 아닌 종료 코드 또는 비정상 종료
    };

    struct  process_result {
        process_status status;
        int exit_code;
    };

    class  process_runner {
    public:
        process_runner() = default;
        ~process_runner() = default;

        /**
         * @brief 프로세스를 실행하고 대기합니다.
         * @param command 실행할 명령어 문자열
         * @param timeout_ms 대기 시간 (0ms는 무한 대기)
         * @return 실행 결과와 종료 코드를 포함한 구조체
         */
        process_result run_process(const std::string& command, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));

    private:
#ifdef _WIN32
        process_result run_windows(const std::string& command, std::chrono::milliseconds timeout_ms);
#else
        process_result run_linux(const std::string& command, std::chrono::milliseconds timeout_ms);
#endif
    };

} 