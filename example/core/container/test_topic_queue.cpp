#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include <string>

#include "mino/core/container/container.hpp"

void test_topic_queue_all_public() {
    std::cout << "[Testing topic_queue - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    auto& tq = container::topic_queue::get_instance();

    // 1. 초기 상태 확인
    assert(tq.get_state() == container::queue_state::stopped);

    std::vector<int> received_temp;
    std::vector<std::string> received_logs;

    // 2. 구독자 등록 (Stopped 상태에서만 가능)
    bool sub1 = tq.subscribe("sensor/temperature", [&received_temp](const container::message_context& ctx) {
        received_temp.push_back(std::any_cast<int>(ctx.data));
        });
    assert(sub1);

    bool sub2 = tq.subscribe("system/log", [&received_logs](const container::message_context& ctx) {
        received_logs.push_back(std::any_cast<std::string>(ctx.data));
        });
    assert(sub2);

    // 3. 시스템 시작
    tq.start();
    assert(tq.get_state() == container::queue_state::running);

    // 러닝 중에는 신규 구독 불가 검증
    bool sub_fail = tq.subscribe("sensor/temperature", [](const container::message_context&) {});
    assert(!sub_fail);

    // 4. 메시지 발행 (Pub-Sub)
    assert(tq.publish("sensor/temperature", 24));
    assert(tq.publish("sensor/temperature", 27));
    assert(tq.publish("system/log", std::string("Worker online")));

    // 비동기 워커 스레드 처리 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 5. 시스템 정지
    tq.stop();
    assert(tq.get_state() == container::queue_state::stopped);

    // 6. 결과 검증
    assert(received_temp.size() == 2);
    assert(received_temp[0] == 24 && received_temp[1] == 27);

    assert(received_logs.size() == 1);
    assert(received_logs[0] == "Worker online");

    std::cout << "  -> topic_queue OK!\n\n";
}
