#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "mino/core/service/cross_platform_service.hpp"

#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// 콘솔 출력 헬퍼 설정
namespace {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
}

// 테스트용 파생 서비스 클래스
class test_service : public mino::core::service::cross_platform_service {
public:
    // (Public) 생성자 호출 테스트
    test_service()
        : cross_platform_service(
            L"TestMinoService", // 유니코드 서비스 이름
            "TestMinoService") // ANSI 서비스 이름
    {
    }

    // (Public) 가상 소멸자 동작 검증
    ~test_service() override {
        log_info("test_service 소멸자가 호출되었습니다.");
        // 종료할 때까지 대기
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        log_info("test_service 소멸");
    }

    // protected 멤버 함수들(로깅 등)을 외부에서 테스트하기 위한 헬퍼
    void test_protected_logging() {
        log_trace("Testing trace log");
        log_debug("Testing debug log");
        log_info("Testing info log");
        log_warn("Testing warn log");
        log_error("Testing error log");
        log_critical("Testing critical log");
    }

protected:
    void on_start() override {
        log_info("on_start() 가 호출되었습니다.");

        worker_thread_ = std::thread([this]() {
            int count = 0;
            while (is_running()) {
                if (!is_paused()) {
                    log_info("Worker count: " + std::to_string(count));
                }
                else {
                    log_info("[WORKER] 일시 중지 상태입니다.");
                }
                service_loop_delay(1000); // 1초 대기
            }
            log_info("[WORKER] 워커 스레드가 종료되었습니다.");
        });
    }

    void on_stop() override {
        log_info("on_stop() 이 호출되었습니다.");
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        log_info("워커 스레드가 종료되었습니다.");
    }

    void on_pause() override {
        log_info("on_pause() 가 호출되었습니다.");
    }

    void on_continue() override {
        log_info("on_continue() 가 호출되었습니다.");
    }

private:
    std::thread worker_thread_;
};

int main() {
    print(tce("=========================================="));
    print(tce("cross_platform_service 퍼블릭 멤버 테스트 시작"));
    print(tce("=========================================="));

    // 1. (Public) 생성자 테스트
    auto service = std::make_unique<test_service>();
    print(tce("[PASS] cross_platform_service 생성자 호출 성공"));

    // 2. (Public) set_logger() 테스트
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
    service->set_logger(logger);
    print(tce("[PASS] set_logger() 호출 성공"));

    // 로깅 및 내부 동작 테스트
    service->test_protected_logging();

    // 3. (Public) run() 테스트
    print(tce("서비스 실행(run)을 시도합니다..."));
    if (!service->run()) {
#ifdef _WIN32
        eprint(tce("[FAIL] run() 실패 - 윈도우 환경에서는 콘솔 직접 실행 대신 SCM(Service Control Manager)을 통해 실행해야 정상 동작합니다."));
#else
        eprint(tce("[FAIL] run() 실행에 실패하였습니다."));
#endif
        return 1;
    }

    print(tce("[PASS] run() 정상 종료"));

    // 4. (Public) 가상 소멸자 테스트 (unique_ptr reset 시 호출)
    service.reset();
    print(tce("[PASS] ~cross_platform_service() 가상 소멸자 호출 성공"));

    return 0;
}
