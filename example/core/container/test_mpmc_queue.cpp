#include <iostream>
#include <cassert>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/core/container/container.hpp"

namespace {
    // 테스트용 작업 구조체
    struct work_item {
        int id;
        int priority; // 숫자가 클수록 높은 우선순위
        std::string message;
    };
}

void test_mpmc_queue() {
    namespace mcsp = mino::core::string::print;
    namespace mcc = mino::core::container;

    std::cout << "[Testing mpmc_queue]" << std::endl;

    mcc::mpmc_queue<work_item> task_queue; // 다중 생산자(MP)-다중 소비자(MC) 큐
    // 다수의 쓰레드에서 동시에 데이터를 생성(produce)하고 소비(consume)할 수 있는 안전한 큐

    // 1. 생산자 스레드: 다양한 우선순위를 가진 데이터 삽입
    std::thread producer([&task_queue]() {
        task_queue.push({ 1, 1, "Low Priority Task A" });
        task_queue.push({ 2, 10, "High Priority Task B" });  
        task_queue.push({ 3, 2, "Normal Priority Task C" });
        task_queue.push({ 4, 8, "Normal Priority Task D" });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // 잠시 대기하여 데이터가 큐에 쌓이도록 함
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Peek & Pop 테스트
    {
        // peek() : 값을 지우지 않고 미리 확인하기
        auto front_item = task_queue.peek();
        if (front_item.has_value()) {
            mcsp::println(" [Peek] 큐 맨 앞의 작업 ID: {}, 큐 사이즈: {}", front_item->id, task_queue.size());
        }

        // pop() : 값을 제거하고 반환하기
        // auto front_item2 = task_queue.pop();
        // if (front_item2.has_value()) {
        //     mcsp::println(" [Pop] 큐에서 제거된 작업 ID: {}, 큐 사이즈: {}", front_item2->id, task_queue.size());
        // }
    }
    // peek() 은 큐에서 값을 제거하지 않음. 확인만 하는 용도로 사용. size가 변하지 않음.
    // pop() 은 큐에서 값을 제거하고 반환함. size가 -1 감소함.

    // 3. 조건부 pop_if 테스트
    std::thread high_priority_consumer([&task_queue]() {
        work_item item;
        bool success = task_queue.pop_if(
            [](const work_item& w) {
                return w.priority >= 5; // 우선순위가 5 이상 
            },
            item); // 조건에 맞는 작업이 있으면 item에 저장하고 true 반환, 없으면 false 반환

        if (success) {
            mcsp::println(" [Conditional Consumer] 긴급 작업 처리 완료! -> ID: {}, 내용: {}", item.id, item.message);
        }
        else {
            mcsp::println(" [Conditional Consumer] 조건에 맞는 긴급 작업이 없거나 맨 앞에 없음.");
            // 현재 큐의 맨 앞 값은 { 1, 1, "Low Priority Task A" } 이고,
            // 조건(우선순위>=5)과 맞는 값이 아니므로 false 반환
        }
    });

    producer.join();
    high_priority_consumer.join();

    // 4. 남은 일반 작업들 처리
    mcsp::println("\n --- 남은 작업 처리 시작 ---");
    while (!task_queue.empty()) { // 큐가 비어있지 않으면(size>0) 반복
        work_item item;
        task_queue.wait_and_pop(item); // 큐가 비어있으면(size=0) 대기하고, 값이 들어오면(size>0) pop 한 다음 item에 저장  
        mcsp::println(" [General Consumer] 처리된 작업 -> ID: {}, 내용: {}", item.id, item.message);
    }
    // 
    // while(true) {
    //   work_item item;
    //   task_queue.wait_and_pop(item); // 큐에 입력이 될때까지 대기. 큐에 입력이 되면 다음 라인으로 넘어감.
    //   ... item을 처리하는 코드 ... 
    // } 를 하면, 큐의 값을 계속 빼서 무한히 처리하는 기능을 수행함.

    std::cout << "[Testing mpmc_queue] Completed" << std::endl << std::endl;
}
