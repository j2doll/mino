#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <spdlog/spdlog.h>

namespace mino::external::server {

    class server_application {

    protected:
        std::vector<std::string> arguments_;
        std::atomic<bool> is_requested_to_stop_;
        std::atomic<bool> is_paused_;
        std::mutex pause_mutex_;
        std::condition_variable pause_cv_;
        std::shared_ptr<spdlog::logger> logger_;
        static inline server_application* instance_ = nullptr;

    public:
        server_application();
        virtual ~server_application();

        // 외부에서 로거를 주입할 수 있는 세터 (nullptr 허용)
        void set_logger(std::shared_ptr<spdlog::logger> logger);

        // 애플리케이션 프레임워크의 핵심 진입점
        int start(int argc, char** argv);

        // 외부/내부에서 안전한 종료(Stop)를 요청할 때 호출
        void terminate();

        // 애플리케이션 일시정지(Pause) 요청
        void pause();

        // 일시정지 상태 해제(Resume) 요청
        void resume();

        bool is_cancelled() const;
        bool is_paused() const;

    protected:
        // 메인 비즈니스 로직 구동 전, 사전 준비를 수행하는 가상 함수
        virtual void pre_run();

        // 사용자가 메인 무한 루프를 구현할 순수 가상 함수
        virtual int run(const std::vector<std::string>& args) = 0;

        // 상태 변화에 대응하는 이벤트 콜백 훅(Hooks)
        virtual void on_pause();
        virtual void on_resume();
        virtual void on_terminate();

        // 메인 스레드 루프 내에서 일시정지 여부를 체크하고 대기하는 헬퍼 함수
        void check_pause_status();

        // 자식 클래스에서 주입된 로거를 안전하게 꺼내 쓸 수 있도록 포인터 반환 (nullptr 가능)
        std::shared_ptr<spdlog::logger> logger();

        // 플랫폼별 명령줄 인자 파싱 함수
        void parse_arguments(int argc, char** argv);

        // 플랫폼별 신호 처리기 설정 함수
        static void setup_signal_handlers();

#if defined(_WIN32) || defined(_WIN64)
        // Windows에서 Ctrl+C, Ctrl+Break, 콘솔 창 닫기 등의 이벤트를 처리하는 핸들러
        static int win32_ctrl_handler(unsigned long ctrl_type);
        std::string wstring_to_utf8(const std::wstring& wstr);
#else
        // POSIX 시스템에서 SIGINT, SIGTERM 등의 신호를 처리하는 핸들러
        static void posix_signal_handler(int signal);
#endif

    };

} 