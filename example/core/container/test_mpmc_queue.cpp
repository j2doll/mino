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
    std::cout << "[Testing mpmc_queue]" << std::endl;

    using mino::core::string::print::println;

    mino::core::container::mpmc_queue<work_item> task_queue;

    // 1. 생산자 스레드: 다양한 우선순위를 가진 데이터 삽입
    std::thread producer([&task_queue]() {
        task_queue.push({ 1, 1, "Low Priority Task A" });
        task_queue.push({ 2, 10, "High Priority Task B" }); // 우선순위 높음
        task_queue.push({ 3, 2, "Normal Priority Task C" });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // 잠시 대기하여 데이터가 큐에 쌓이도록 함
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Peek 테스트: 값을 지우지 않고 미리 확인하기
    {
        auto front_item = task_queue.peek();
        if (front_item.has_value()) {
            println(" [Peek] 큐 맨 앞의 작업 ID: {}, 큐 사이즈: {}", front_item->id, task_queue.size());
        }

       // auto front_item2 = task_queue.pop();
       // if (front_item2.has_value()) {
       //     println(" [Pop] 큐에서 제거된 작업 ID: {}, 큐 사이즈: {}", front_item2->id, task_queue.size());
       // }
    }
    // peek() 은 큐에서 값을 제거하지 않음. 확인만 하는 용도로 사용. size가 변하지 않음.
    // pop() 은 큐에서 값을 제거하고 반환함. size가 -1 감소함.

    // 3. 조건부 팝 테스트: 우선순위가 5 이상인 긴급 작업만 골라서 먼저 처리하기
    std::thread high_priority_consumer([&task_queue]() {
        work_item item;
        bool success = task_queue.pop_if([](const work_item& w) {
            return w.priority >= 5;
            }, item);

        if (success) {
            println(" [Conditional Consumer] 긴급 작업 처리 완료! -> ID: {}, 내용: {}", item.id, item.message);
        }
        else {
            println(" [Conditional Consumer] 조건에 맞는 긴급 작업이 없거나 맨 앞에 없음.");
        }
        });

    producer.join();
    high_priority_consumer.join();

    // 4. 남은 일반 작업들 처리
    println("\n --- 남은 작업 처리 시작 ---");
    while (!task_queue.empty()) {
        work_item item;
        task_queue.wait_and_pop(item);
        println(" [General Consumer] 처리된 작업 -> ID: {}, 내용: {}", item.id, item.message);
    }
    
    std::cout << "[Testing mpmc_queue] Completed" << std::endl << std::endl;
}
