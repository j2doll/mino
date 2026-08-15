#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <limits>

#include "mino/core/server/server.hpp"

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 사용자가 지정한 출력 헬퍼
namespace {
    // 문자열 인코딩 변환 함수 (프로젝트의 실제 경로에 맞게 연결)
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
}

// 사용자 정의 서버 애플리케이션
class test_server_application : public mino::core::server::server_application {
protected:
    // pre_run() 오버라이드: 메인 루프 진입 전 초기화 작업 수행
    void pre_run() override {
        if (logger()) { logger()->info("[TestApp] 로거가 주입되어 있음"); }
        else { print(tce("[TestApp] 로거가 주입되지 않음")); }
    }

    // run() 오버라이드: 메인 루프 구현
    int run(const std::vector<std::string>& args) override {
        if (logger()) { logger()->info("[TestApp] 메인 루프 시작. 전달받은 인자 수: {}", args.size()); }
        else { print(tce("[TestApp] 메인 루프 시작. 전달받은 인자 수: "), args.size()); }

        for (size_t i = 0; i < args.size(); ++i) {
            if (logger()) { logger()->info("  - args[{}]: {}", i, args[i]); }
            else { print(tce("  - args["), i, tce("]: "), args[i]); }
        }

        int step = 0;

        // is_cancelled() 공개 메서드 확인
        while (!is_cancelled()) {
            // 내부 일시정지 상태 대기
            check_pause_status();

            if (is_cancelled()) { // 안전 종료 요청이 들어오면 루프 탈출
                break;
            }

            // is_paused() 공개 메서드 확인

            if (logger()) { logger()->info("[TestApp] 루프 실행 중... (Step: {}, is_paused: {}, is_cancelled: {})", ++step, is_paused(), is_cancelled()); }
            else { print(tce("[TestApp] 루프 실행 중... (Step: "), ++step, tce(", is_paused: "), is_paused(), tce(", is_cancelled: "), is_cancelled(), tce(")")); }

            if (step == std::numeric_limits<int>::max()) {
                step = 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (logger()) { logger()->info("[TestApp] 메인 루프 안전 종료."); }
        else { print(tce("[TestApp] 메인 루프 안전 종료.")); }

        return 0;
    }

    // on_pause() 오바라이드: 일시정지 이벤트 훅
    void on_pause() override {
        if ( logger()) { logger()->info("[TestApp Hook] on_pause() 호출됨");  }
        else { print(tce("[TestApp Hook] on_pause() 호출됨")); }
    }

    // on_resume() 오버라이드: 재개 이벤트 훅
    void on_resume() override {
        if (logger()) { logger()->info("[TestApp Hook] on_resume() 호출됨"); }
        else { print(tce("[TestApp Hook] on_resume() 호출됨")); }
    }

    // on_terminate() 오버라이드: 안전 종료 이벤트 훅
    void on_terminate() override {
        if (logger()) { logger()->info("[TestApp Hook] on_terminate() 호출됨"); }
        else { print(tce("[TestApp Hook] on_terminate() 호출됨")); }
    }
};

int main(int argc, char** argv) {
    test_server_application app;

    // 1. set_logger() 테스트 (nullptr 또는 유효 로거 주입 가능)
    namespace mclt = mino::core::log::tinylog;
    using console_sink_config = mclt::console_sink_config;
    console_sink_config ccfg;
#ifdef _WIN32
    ccfg.encoding = mclt::encoding_type::cp949;
    ccfg.eol = mclt::eol_type::crlf;
#else
    ccfg.encoding = mclt::encoding_type::utf8;
    ccfg.eol = mclt::eol_type::lf;
#endif
    auto logger = std::make_shared<mclt::logger>("server_logger");
    logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));
    app.set_logger(logger);
    // app.set_logger(nullptr); // 로거 사용 않함

    // 2. 별도 제어 스레드를 통한 pause(), resume(), terminate(), is_paused(), is_cancelled() 테스트
    std::thread control_thread([&app]() {
        // 서버 루프가 시작될 때까지 잠시 대기
        std::this_thread::sleep_for(std::chrono::seconds(3));

        print(endl, tce("\n>>> [ControlThread] 1. 일시정지(pause) 요청"), endl);
        app.pause();
        print(endl, tce(">>> [ControlThread] app.is_paused(): "), app.is_paused(), endl);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        print(endl, tce(">>> [ControlThread] 2. 재개(resume) 요청"), endl);
        app.resume();
        print(endl, tce(">>> [ControlThread] app.is_paused(): "), app.is_paused(), endl);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        print(endl, tce(">>> [ControlThread] 3. 안전 종료(terminate) 요청"), endl);
        app.terminate();
        print(endl, tce(">>> [ControlThread] app.is_cancelled(): "), app.is_cancelled(), endl);
    });

    // 3. start() 테스트 (메인 루프 진입점)
    int exit_code = app.start(argc, argv);

    if (control_thread.joinable()) {
        control_thread.join();
    }

    if (exit_code != 0) {
        eprint(tce("[Error] 애플리케이션 비정상 종료 (Code: "), exit_code, tce(")"));
    }
    else {
        print(tce("[Success] 애플리케이션 정상 종료 완료 (Code: "), exit_code, tce(")"));
    }

    return exit_code;
}
