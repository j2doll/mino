#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/dispatch/dispatch.hpp"

/**
 * [Case 1] dynamic_event_dispatcher 테스트용 이벤트
 * - 동적 힙 메모리(std::string, std::vector)와 
 *   참조 횟수 기반 자원 관리(std::shared_ptr)를 포함하는 대표적인 비-POD 구조체입니다.
 * - 내부적으로 역참조 캐스팅(static_cast<const event_t*>)을 수행하므로
 *    복사나 이동 비용 없이 원본 주소로 안전하게 핸들러에 전달됩니다.
 */
struct inventory_event {
    std::string player_name;
    std::vector<std::string> item_list;
    std::shared_ptr<uint64_t> session_id;
};

/**
 * 시스템 로그 및 알림 전달용 단순 문자열 이벤트
 */
struct log_alert_event {
    std::string alert_level;
    std::string message;
};

void test_dynamic_event_dispatcher() {
    namespace mcd = mino::core::dispatch;

    // -------------------------------------------------------------------------
    // 1. dynamic_event_dispatcher 테스트
    // 특징: 런타임 다대다 구독/발행, std::shared_mutex 기반 읽기/쓰기 분리 동시성 보장
    // -------------------------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "1. dynamic_event_dispatcher (Thread-safe & Non-POD)\n";
    std::cout << "========================================================\n";

    using dynamic_event_dispatcher = mcd::dynamic_event_dispatcher;
    dynamic_event_dispatcher dyn_disp;

    // (1) 비-POD 이벤트(스마트 포인터/동적 배열) 핸들러 등록
    // - 내부적으로 구독자 맵에 추가할 때 배타적 쓰기 락(std::unique_lock)이 작동합니다.
    dyn_disp.subscribe<inventory_event>([](const auto& ev) {
        auto timestamp = mino::core::datetime::util::current_time_string();
        auto ret = timestamp +
            "  [Inventory] Player: " + ev.player_name +
            " | Items: " + std::to_string(ev.item_list.size()) +
            " | Token Ref Count: " + std::to_string(ev.session_id.use_count()) + "\n";
        std::cout << ret << std::flush;
        });

    // (2) 일반 로깅 이벤트 핸들러 등록
    dyn_disp.subscribe<log_alert_event>([](const auto& ev) {
        auto timestamp = mino::core::datetime::util::current_time_string();
        auto ret = timestamp +
            "  [Alert] Level: " + ev.alert_level +
            " | Message: " + ev.message + "\n";
        std::cout << ret << std::flush;
        });

    // (3) 단일 이벤트 디스패치
    // - 핸들러 호출 시 공유 읽기 락(std::shared_lock)을 획득하여 다중 스레드 디스패치를 허용합니다.
    auto shared_token = std::make_shared<uint64_t>(0xAABBCCDDEEFF);
    dyn_disp.dispatch(inventory_event{ "Warrior_Kim", {"Axe", "Shield"}, shared_token });

    // (4) C++17 Fold Expression 기반 가변 인자 일괄 디스패치
    // - (dispatch(events), ...) 형태로 컴파일 타임에 순차적 dispatch() 호출 코드로 전개됩니다.
    std::cout << "  -- Fold Expression Multi-Dispatch --\n";
    dyn_disp.dispatch_all(
        log_alert_event{ "WARN", "Disk capacity over 85%" },
        inventory_event{ "Mage_Lee", {"Staff", "Robe", "Potion"}, shared_token }
    );

    // (5) 멀티스레드 동시성 안전성 검증 (Stress Test)
    // - 읽기 스레드(dispatch)와 쓰기 스레드(subscribe/unsubscribe)가 동시에 실행될 때
    //   std::shared_mutex가 Data Race 및 이터레이터 무효화를 방지하는지 확인합니다.
    std::cout << "  -- Running Concurrent Thread Test (200ms) --\n";
    std::atomic<bool> is_running{ true };
    std::atomic<uint64_t> read_count{ 0 };

    // 읽기 작업: 다수의 스레드가 동시에 안전하게 dispatch를 실행 (Shared Lock)
    auto reader_task = [&dyn_disp, &is_running, &read_count, shared_token]() {
        inventory_event ev{ "Thread_Bot", {"Item"}, shared_token };
        while (is_running.load(std::memory_order_relaxed)) {
            dyn_disp.dispatch(ev);
            read_count.fetch_add(1, std::memory_order_relaxed);
        }
        };

    // 쓰기 작업: 핸들러를 동적으로 등록 및 해제 (Unique Lock)
    auto writer_task = [&dyn_disp, &is_running]() {
        while (is_running.load(std::memory_order_relaxed)) {
            auto id = dyn_disp.subscribe<log_alert_event>([](const auto&) {});
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            dyn_disp.unsubscribe<log_alert_event>(id);
        }
        };

    // Reader 2개, Writer 1개 동시 구동
    std::thread t1(reader_task);
    std::thread t2(reader_task);
    std::thread t3(writer_task);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    is_running.store(false, std::memory_order_relaxed);

    t1.join();
    t2.join();
    t3.join();

    std::cout << "  [Concurrency Result] Safe Dispatches: " << read_count.load() << "\n\n";

}
