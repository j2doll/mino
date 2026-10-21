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
* [Case 2] any_event_dispatcher 테스트용 이벤트
* - 동적 바이너리 페이로드(std::vector<uint8_t>)를 포함합니다.
* - std::any 컨테이너 내부에 박싱(Boxing)될 수 있도록 복사 가능한(CopyConstructible) 상태를 유지합니다.
*/
struct network_packet_event {
    int opcode;
    std::vector<uint8_t> payload;
};

void test_any_event_dispatcher() {
    namespace mcd = mino::core::dispatch;

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

}
