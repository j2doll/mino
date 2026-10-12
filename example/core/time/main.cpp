#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

#include "mino/core/time/hierarchical_timer_wheel.hpp"
#include "mino/core/log/log.hpp"

int main() {
    namespace tinylog = mino::core::log::tinylog;
    using console_sink_config = tinylog::console_sink_config;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;
    using log_level = tinylog::log_level;

    using namespace std::chrono_literals;
    using namespace std::this_thread;

    // ----------------------------------------------
    // 1. tinylog 콘솔 싱크 및 로거 초기화
    console_sink_config config;
#ifdef _WIN32
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::crlf;
#else
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::lf;
#endif 
    auto console = std::make_shared<tinylog::console_sink>("console", config);
    auto log = std::make_shared<tinylog::logger>("timer_example");
    log->add_sink(console);
    log->set_level(log_level::trace);

    tinylog::logger::register_logger(log);

    log->info("<bold><cyan>=== Mino 비동기 타이머 휠 테스트 시작 ===</cyan></bold>");

    // ----------------------------------------------
    // 2. 10ms 단위 틱을 사용하는 타이머 휠 생성.
    namespace mct = mino::core::time;
    using hierarchical_timer_wheel = mct::hierarchical_timer_wheel;
    hierarchical_timer_wheel wheel( 10ms );
    // 내부 워커 스레드가 10ms마다 깨어나서 시계 바늘(틱)을 한 칸 전진시키고
    // 만료된 타이머가 있는지 점검하여 실행.
    // 즉, 점검 주기가 10ms 라는 의미임.

    // ----------------------------------------------
    // 3. 단기 1회성 타이머 (Level 0에서 즉시 만료)
    wheel.schedule_after(50ms, []() {
        auto log = mino::core::log::tinylog::logger::get("timer_example");
        if (log) log->info("<bright_green>[Timer]</bright_green> 단기 50ms 타이머 만료 실행");
    });

    // ----------------------------------------------
    // 4. 상위 휠 배치 및 캐스케이딩(Cascading) 검증
    // 10ms 틱 * 256 슬롯 = 2,560ms 초과 시 Level 1로 배치된 후 Level 0으로 강하
    wheel.schedule_after(2600ms, []() {
        auto log = mino::core::log::tinylog::logger::get("timer_example");
        if (log) log->info("<bright_yellow>[Timer]</bright_yellow> 2600ms 장기 타이머 (Wheel 1 -> Wheel 0 캐스케이딩 성공)");
    });

    // ----------------------------------------------
    // 5. 주기적 반복 타이머 (100ms마다 실행, 3회 실행 후 자체 취소)
    std::atomic<int> count{ 0 };
    mino::core::time::timer_id periodic_id = 0;

    periodic_id = wheel.schedule_every(100ms, [&count, &wheel, &periodic_id]() {
        auto log = mino::core::log::tinylog::logger::get("timer_example");
        int current = ++count;
        if (log) log->info("[Timer] 주기적 타이머 틱: {}", current);

        if (current >= 3) {
            if (log) log->warn("[Timer] 3회 도달 -> 주기 타이머 자체 취소");
            wheel.cancel(periodic_id); // 주기적 타이머 자체 취소
        }
    });

    // ----------------------------------------------
    // 6. 타이머 사전 취소 검증 (만료 전 취소되어 에러 로그가 출력되지 않아야 함)
    log->info("[Test] 200ms 타이머를 등록 후 50ms 후 취소 시도");
    auto cancel_target_id = wheel.schedule_after(200ms, []() {
        auto log = mino::core::log::tinylog::logger::get("timer_example");
        if (log) log->error("<bright_red>[Error] 이 메시지가 출력되면 안 됩니다 (취소 실패)!</bright_red>");
    });
    sleep_for(50ms);
    bool cancelled = wheel.cancel(cancel_target_id);
    log->info("[Test] 200ms 타이머 취소 결과: {}", cancelled ? "<bright_green>성공</bright_green>" : "<bright_red>실패</bright_red>");

    // 7. 모든 타이머(장기 2,600ms 포함)가 완전히 만료될 때까지 대기
    sleep_for(2800ms);
    wheel.stop();
    log->info("<bold><bright_green>=== 모든 타이머 테스트 완료 ===</bright_green></bold>");

    return 0;
}
