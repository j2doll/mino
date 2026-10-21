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

void test_static_dispatcher() {
    namespace mcd = mino::core::dispatch;

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

}
