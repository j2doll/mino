#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/core/subsystem/subsystem_master_channel.hpp"

namespace mcss = mino::core::subsystem;
namespace mcl = mino::core::log::tinylog;

int main(int argc, char* argv[]) {
    using console_sink_config = mcl::console_sink_config;

    console_sink_config cscfg;
#if defined(_WIN32)
    cscfg.encoding = mcl::encoding_type::cp949;
    cscfg.eol = mcl::eol_type::crlf;
#else
    cscfg.encoding = mcl::encoding_type::utf8;
    cscfg.eol = mcl::eol_type::lf;
#endif
    auto console = std::make_shared<mcl::console_sink>("master_console", cscfg);
    auto logger = std::make_shared<mcl::logger>("master");
    logger->add_sink(console);

    std::string worker_bin = "./example_worker";
#if defined(_WIN32)
    worker_bin = "example_worker.exe";
#endif
    if (argc > 1) {
        worker_bin = argv[1];
    }

    logger->info("<cyan>=======================================================</cyan>");
    logger->info("<cyan>[서브시스템 마스터 세션 시작]</cyan> 워커 바이너리: {}", worker_bin);
    logger->info("<cyan>=======================================================</cyan>");

    mcss::master_channel channel;
    channel.set_logger(logger);

    // 워커(example_worker) => 마스터(example_master) 콜백 등록
    channel.on_event([logger](const std::string& event_name, const std::string& payload) {
        logger->info("<magenta>[워커 자율 푸시 수신] 이벤트: '{}' -> 본문: '{}'</magenta>",
            event_name, payload);
    });

    if (!channel.spawn(worker_bin)) { // 워커 프로세스 기동
        logger->error("<red>워커 프로세스 기동 실패</red>: {}", worker_bin);
        return 1;
    }

    // 편의를 위해 RPC 호출을 람다로 정의
    auto execute_rpc = [&](const std::string& cmd, const std::string& payload) {
        logger->info("[Master -> Worker] 송신: CMD='{}', DATA='{}'", cmd, payload);

        std::string status;
        std::string response;
        if (channel.call(cmd, payload, status, response)) {
            if (status == "OK") {
                logger->info("<green>[Worker -> Master] 성공: STATUS='{}', DATA='{}'</green>", status, response);
            }
            else {
                logger->error("<red>[Worker -> Master] 오류: STATUS='{}', 사유='{}'</red>", status, response);
            }
        }
        else {
            logger->error("<red>워커 프로세스로부터 응답을 읽지 못했습니다.</red>");
        }
    };

    // 1. 세션 초기화
    execute_rpc("INIT", "마스터 컨트롤러 클라이언트 (v1.0)");

    // 2. 워커의 백그라운드 이벤트가 도착할 시간을 주기 위해 대기하며 상호작용
    logger->info("--- 워커의 자율 백그라운드 이벤트 수신 대기 중 (1.5초) ---");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 3. 자율 이벤트 수신 도중에도 마스터의 동기 RPC가 안전하게 동작함을 확인
    execute_rpc("CALC", "150 + 350");

    logger->info("--- 워커의 추가 이벤트 수신 대기 중 (1.5초) ---");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 4. 세션 종료.
    // QUIT 명령은 워커 프로세스가 종료되도록 지시하며, 마스터는 워커의 종료를 기다린 후 채널을 닫음.
    execute_rpc("QUIT", "");

    channel.close(); // 워커 프로세스 종료 후 채널 닫기
    logger->info("<cyan>=======================================================</cyan>");
    logger->info("<cyan>[서브시스템 마스터 세션 정상 완료]</cyan>");
    logger->info("<cyan>=======================================================</cyan>");
    return 0;
}
