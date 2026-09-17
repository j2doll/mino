#include <iostream>
#include <cassert>
#include <vector>
#include <functional>

#include "mino/core/container/container.hpp"

void test_priority_queue_all_public() {
    std::cout << "[Testing priority_queue - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 1. 기본 Max-Heap
    container::priority_queue<int> pq;
    assert(pq.empty());
    assert(pq.size() == 0);

    pq.push(10);
    pq.push(5);
    pq.push(20);
    pq.push(15);
    pq.push(3);

    assert(!pq.empty());
    assert(pq.size() == 5);
    assert(pq.top().has_value() && pq.top().value() == 20);

    // pop 검증 (내림차순 추출)
    assert(pq.pop());
    assert(pq.top().has_value() && pq.top().value() == 15);
    assert(pq.size() == 4);

    // emplace
    pq.emplace(25);
    assert(pq.top().has_value() && pq.top().value() == 25);

    // 2. Min-Heap (std::greater)
    container::priority_queue<int, std::greater<int>> min_pq;
    min_pq.push(10);
    min_pq.push(5);
    min_pq.push(20);
    min_pq.push(3);

    assert(min_pq.top().has_value() && min_pq.top().value() == 3);
    assert(min_pq.pop());
    assert(min_pq.top().has_value() && min_pq.top().value() == 5);

    // 3. swap & clear
    container::priority_queue<int> pq_a = { 100, 200 };
    container::priority_queue<int> pq_b = { 1, 2 };

    pq_a.swap(pq_b);
    assert(pq_a.top().value() == 2);
    assert(pq_b.top().value() == 200);

    pq_a.clear();
    assert(pq_a.empty());
    assert(!pq_a.top().has_value());
    assert(!pq_a.pop());

    std::cout << "  -> priority_queue OK!\n\n";
}
