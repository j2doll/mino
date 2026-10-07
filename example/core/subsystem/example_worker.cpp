#include <string>
#include <utility>
#include <thread>
#include <chrono>
#include <atomic>

#include "mino/core/subsystem/subsystem_worker_service.hpp"

namespace mcss = mino::core::subsystem;

int main(int argc, char* argv[]) {
    mcss::worker_service service;
    auto logger = service.get_logger();

    std::atomic<bool> worker_running{ true };

    // 워커(example_worker) => 마스터(example_master) 백그라운드 스레드. 테스트 용도.
    std::thread background_task([&service, &worker_running, logger]() {
        int progress = 0;
        while (worker_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            if (!worker_running.load()) break;

            progress += 25;
            logger->info("Background worker generating autonomous telemetry event...");

            // 1. 진행률 보고 이벤트 자율 푸시
            service.notify("PROGRESS", "비동기 작업 진행률: " + std::to_string(progress) + "% 완료");

            // 2. 특정 조건 발생 시 자율 경보 이벤트 푸시
            if (progress == 50) {
                service.notify("ALERT", "워커 내부 상태 양호 (온도: 36.5도, 큐 비어있음)");
            }
            if (progress >= 100) progress = 0;
        }
    });

    // RPC 핸들러 1: 세션 초기화
    service.on_command("INIT", [logger](const std::string& client_info) -> std::pair<std::string, std::string> {
        logger->info("Processing session negotiation: {}", client_info);
        return { "OK", "서브시스템 준비 완료" };
    });

    // RPC 핸들러 2: 계산
    service.on_command("CALC", [logger](const std::string& expr) -> std::pair<std::string, std::string> {
        logger->info("Processing calculation: {}", expr);
        if (expr == "150 + 350") {
            return { "OK", "500" };
        }
        return { "ERR", "계산할 수 없는 수식: " + expr };
    });

    // 메인 루프 진입 (QUIT 수신 시 반환)
    service.run();

    // 워커 종료 시 백그라운드 스레드 정리
    worker_running.store(false);
    if (background_task.joinable()) {
        background_task.join();
    }

    return 0;
}
