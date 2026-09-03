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

// =============================================================================
// 비-POD(Non-POD) 및 다양한 메모리 구조를 가진 이벤트 정의
// =============================================================================

/**
 * [Case 1] dynamic_event_dispatcher 테스트용 이벤트
 * - 동적 힙 메모리(std::string, std::vector)와 참조 횟수 기반 자원 관리(std::shared_ptr)를 포함하는 대표적인 비-POD 구조체입니다.
 * - 내부적으로 역참조 캐스팅(static_cast<const event_t*>)을 수행하므로 복사나 이동 비용 없이 원본 주소로 안전하게 핸들러에 전달됩니다.
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

/**
 * [Case 2] any_event_dispatcher 테스트용 이벤트
 * - 동적 바이너리 페이로드(std::vector<uint8_t>)를 포함합니다.
 * - std::any 컨테이너 내부에 박싱(Boxing)될 수 있도록 복사 가능한(CopyConstructible) 상태를 유지합니다.
 */
struct network_packet_event {
    int opcode;
    std::vector<uint8_t> payload;
};

/**
 * [Case 3] static_dispatcher(std::variant) 테스트용 이벤트
 * - 힙 할당이 전혀 발생하지 않는 스택 기반 데이터 구조입니다.
 * - 게임 틱 루프나 렌더 루프처럼 초당 수만 건 이상의 빠른 처리가 필요한 환경에 적합합니다.
 */
struct melee_attack_event {
    int attacker_id;
    int target_id;
    int damage;
};

struct heal_event {
    int caster_id;
    int target_id;
    int amount;
};

/**
 * [Case 4] collecting_event_dispatcher 테스트용 이벤트
 * - 다중 보안 검증 파이프라인에서 핸들러들의 개별 검증 결과(bool)를 취합하기 위한 요청 데이터입니다.
 */
struct permission_check_event {
    std::string user_name;
    std::string action;
    int clear_level;
};

// =============================================================================
// 메인 테스트 진입점
// =============================================================================

int main() {
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

    // -------------------------------------------------------------------------
    // 2. any_event_dispatcher 테스트
    // 특징: 타입을 런타임에 결정해야 하는 네트워크 패킷/메시지 버스에 적합
    // -------------------------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "2. any_event_dispatcher (Dynamic Any Boxing)\n";
    std::cout << "========================================================\n";

    using any_event_dispatcher = mcd::any_event_dispatcher;
    any_event_dispatcher any_disp;

    // 구체 타입을 인자로 받는 핸들러 등록 (내부적으로 std::any_cast를 거쳐 안전하게 복원됨)
    any_disp.subscribe<network_packet_event>([](const auto& pkt) {
        auto ret = mino::core::datetime::util::current_time_string() +
            "  [Packet Handler] Opcode: 0x" + std::to_string(pkt.opcode) +
            " | Payload Size: " + std::to_string(pkt.payload.size()) + " bytes\n";
        std::cout << ret << std::flush;
        });

    // 런타임에 동적으로 들어온 std::any 객체 직접 디스패치 (event.type() 기반 라우팅)
    std::any network_stream = network_packet_event{ 0x1005, {0xDE, 0xAD, 0xBE, 0xEF} };
    any_disp.dispatch_any(network_stream);

    // 구체 객체를 전달할 경우 내부에서 std::make_any로 래핑하여 호출
    any_disp.dispatch(network_packet_event{ 0x2001, {0x01, 0x02} });
    std::cout << "\n";

    // -------------------------------------------------------------------------
    // 3. static_dispatcher 테스트
    // 특징: std::variant 기반 고성능 제로 할당, 컴파일 타임 점프 테이블 생성
    // -------------------------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "3. static_dispatcher (Zero-allocation Variant Queue)\n";
    std::cout << "========================================================\n";

    // 허용할 이벤트 목록을 템플릿 인자로 명시하여 고정 크기 variant 타입 정의
    using combat_dispatcher = mcd::static_dispatcher<melee_attack_event, heal_event>;
    using combat_event_t = combat_dispatcher::variant_type;

    // C++17 오버로드 패턴을 사용해 복수의 람다를 하나의 Callable 객체(Visitor)로 병합
    auto combat_visitor = mcd::overloaded{
        [](const melee_attack_event& ev) {
            auto ret = mino::core::datetime::util::current_time_string() +
                "  [Melee] Entity " + std::to_string(ev.attacker_id) +
                " dealt " + std::to_string(ev.damage) + " dmg to Entity " + std::to_string(ev.target_id) + "\n";
            std::cout << ret << std::flush;
        },
        [](const heal_event& ev) {
            auto ret = mino::core::datetime::util::current_time_string() +
                "  [Heal] Caster " + std::to_string(ev.caster_id) +
                " healed Entity " + std::to_string(ev.target_id) + " for " + std::to_string(ev.amount) + " HP\n";
            std::cout << ret << std::flush;
        }
    };

    // (1) 단일 구체 인스턴스 디스패치 (인라인 확장)
    combat_dispatcher::dispatch_value(melee_attack_event{ 1, 2, 45 }, combat_visitor);

    // (2) 연속 메모리 큐(std::vector<variant>)를 통한 일괄 처리 (배치 파이프라인)
    // - 모든 데이터가 연속된 메모리에 유지되므로 CPU 캐시 적중률(Cache Locality)이 극대화됩니다.
    std::vector<combat_event_t> battle_queue;
    battle_queue.reserve(3);
    battle_queue.emplace_back(melee_attack_event{ 2, 1, 30 });
    battle_queue.emplace_back(heal_event{ 3, 1, 50 });
    battle_queue.emplace_back(melee_attack_event{ 1, 2, 90 });

    std::cout << "  -- Flushing Batch Combat Queue --\n";
    combat_dispatcher::dispatch_queue(battle_queue, combat_visitor);
    std::cout << "\n";

    // -------------------------------------------------------------------------
    // 4. collecting_event_dispatcher 테스트
    // 특징: 등록된 핸들러들의 반환값을 std::vector<return_t>로 취합하여 반환
    // -------------------------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "4. collecting_event_dispatcher (Result Aggregation)\n";
    std::cout << "========================================================\n";

    // 핸들러가 bool 타입을 반환하는 검증용 디스패처 생성
    mcd::collecting_event_dispatcher<bool> auth_disp;

    // 검증 규칙 1: 시스템 종료 명령어의 권한 등급 확인
    auth_disp.subscribe<permission_check_event>([](const auto& ev) {
        bool allowed = !(ev.action == "SERVER_SHUTDOWN" && ev.clear_level < 5);
        auto ret = mino::core::datetime::util::current_time_string() +
            "  [Check: Action]        " + ev.action +
            " | Required Level: " + std::to_string(ev.clear_level) +
            " | Result: " + (allowed ? "PASS" : "FAIL") + "\n";
        std::cout << ret << std::flush;
        return allowed;
        });

    // 검증 규칙 2: 게스트 계정 제한 확인
    auth_disp.subscribe<permission_check_event>([](const auto& ev) {
        bool allowed = (ev.user_name != "guest");
        auto ret = mino::core::datetime::util::current_time_string() +
            "  [Check: User]          " + ev.user_name +
            " | Result: " + (allowed ? "PASS" : "FAIL") + "\n";
        std::cout << ret << std::flush;
        return allowed;
        });

    // 이벤트 디스패치 실행 후 각 핸들러의 실행 결과가 담긴 vector<bool> 수집
    permission_check_event req{ "guest", "SERVER_SHUTDOWN", 3 };
    std::cout << "  -- Executing Permission Validation for User: " << req.user_name << " --\n";
    std::vector<bool> inspection_results = auth_disp.dispatch(req);

    // 수집된 모든 검증 결과의 참/거짓 종합 평가
    bool all_passed = true;
    for (std::size_t i = 0; i < inspection_results.size(); ++i) {
        if (!inspection_results[i]) {
            all_passed = false;
        }
    }

    std::cout << "  [Final Verdict] Execution: " << (all_passed ? "ALLOWED" : "DENIED") << "\n";
    std::cout << "========================================================\n";

    return 0;
}
