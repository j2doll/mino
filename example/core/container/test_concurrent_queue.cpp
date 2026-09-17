#include <iostream>
#include <string>
#include <cassert>
#include <thread>

#include "mino/core/container/container.hpp"

void test_concurrent_queue_all_public() {
    std::cout << "[Testing concurrent_queue - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using cqstr = container::concurrent_queue<std::string>; // 문자열 동시성 큐
    using overflow_policy = container::overflow_policy;

    // 1. 생성자, 용량, 정책 조회
    cqstr cq(2, overflow_policy::reject_new); // 용량 2, reject_new 정책인 큐를 생성
    // reject_new는 큐가 가득 찼을 때 새 데이터를 거부하는 정책
    assert(cq.is_bounded()); // 크기 제한 여부 확인
    assert(cq.capacity() == 2); // capacity 확인
    assert(cq.get_overflow_policy() == overflow_policy::reject_new); // 정책 확인
    assert(cq.empty()); // 초기 상태에서 empty 확인
    assert(cq.size() == 0); // size 확인

    // 무제한 큐
    using cqint = container::concurrent_queue<int>;
    cqint unbounded_cq(0); // 무제한 큐(용량 0) 생성
    assert(!unbounded_cq.is_bounded()); // 크기 제한 여부 확인

    // 2. enqueue (const&, &&, emplace)
    std::string s1 = "Hello";
    assert(cq.enqueue(s1) == true); // lvalue로 인큐 -> ["Hello"]                   
    assert(cq.enqueue(std::string("World")) == true); // rvalue로 인큐 -> ["Hello", "World"]
    assert(cq.size() == 2); // ["Hello", "World"]이므로 size는 2

    // reject_new 오버플로우
    assert(cq.enqueue("Overflow") == false);
    // reject_new 정책이므로 큐가 가득 찼을 때 enqueue()를 하면, 새 데이터를 거부하고 false 반환

    // drop_oldest 테스트
    cqint cq_drop(2, container::overflow_policy::drop_oldest); // 크기가 2인 drop_oldest 정책 큐 생성
    // drop_oldest는 큐가 가득 찼을 때, 가장 오래된 데이터를 버리고 새 데이터를 추가하는 정책
    cq_drop.enqueue(1); // [1]
    cq_drop.enqueue(2); // [1, 2]
    cq_drop.enqueue(3); // 1 버려짐 -> [2, 3]

    int popped_val;
    assert(cq_drop.try_dequeue(popped_val) // [2, 3]에서 2를 꺼냄
        && popped_val == 2);
    // 현재 cq_drop[3] 이며, size는 1

    // 3. emplace
    unbounded_cq.emplace(100); // 무제한 큐에 100 추가
    assert(unbounded_cq.size() == 1);

    // 4. try_dequeue
    std::string out;
    assert(cq.try_dequeue(out) == true // ["Hello", "World"]에서 Hello를 꺼냄
        && out == "Hello");
    // 현재 cq["World"] 이며, size는 1

    // 5. dequeue_if
    bool dequeued = cq.dequeue_if(out,
        [](const std::string& head, std::size_t sz) { return head == "World" && sz == 1; }
    ); // head가 "World"이고, size가 1일 때만 dequeue
    assert(dequeued && out == "World");

    // 비어있을 때 try_dequeue 및 dequeue_if
    assert(cq.try_dequeue(out) == false);
    assert(cq.dequeue_if(out, [](const std::string&, std::size_t) { return true; }) == false);

    // 6. wait_dequeue (스레드 활용)
    std::thread t([&cq]() {
        std::string val;
        cq.wait_dequeue(val); // 큐가 비어있으면 항목이 들어올 때까지 대기
        assert(val == "DelayedData"); // wait_dequeue가 완료되면 val은 "DelayedData"가 됨
        });

    cq.enqueue("DelayedData");
    t.join(); // wait_dequeue가 완료될 때까지 대기

    // 7. clear
    cq.enqueue("A");
    cq.enqueue("B");
    assert(cq.clear() == 2);
    assert(cq.empty());

    std::cout << "  -> concurrent_queue OK!\n\n";
}
