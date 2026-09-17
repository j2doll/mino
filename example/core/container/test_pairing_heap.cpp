#include <iostream>
#include <cassert>

#include "mino/core/container/container.hpp"

void test_pairing_heap_all_public() {
    std::cout << "[Testing pairing_heap - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using PH = container::pairing_heap<int>;

    PH ph1;
    assert(ph1.empty());
    assert(ph1.size() == 0);

    // 1. push & emplace
    auto h1 = ph1.push(10);
    auto h2 = ph1.push(5);
    auto h3 = ph1.push(20);
    auto h4 = ph1.emplace(15);
    auto h5 = ph1.push(3);
    (void)h2; (void)h3; (void)h5;

    assert(!ph1.empty());
    assert(ph1.size() == 5);
    assert(ph1.top().has_value() && ph1.top().value() == 20); // Max-Heap 기준 최댓값

    // 2. pop
    assert(ph1.pop());
    assert(ph1.top().has_value() && ph1.top().value() == 15);
    assert(ph1.size() == 4);

    // 3. merge
    PH ph2;
    ph2.push(30);
    ph2.push(8);
    ph2.push(12);

    ph1.merge(ph2);
    assert(ph1.size() == 7);
    assert(ph2.empty());
    assert(ph1.top().has_value() && ph1.top().value() == 30);

    // 4. update (노드 값 갱신)
    ph1.update(h1, 50); // h1(10 -> 50)
    assert(ph1.top().has_value() && ph1.top().value() == 50);

    // 5. erase (특정 노드 핸들 삭제)
    ph1.erase(h4); // 15 삭제
    assert(ph1.size() == 6);

    // 6. clear
    ph1.clear();
    assert(ph1.empty());
    assert(ph1.size() == 0);
    assert(!ph1.top().has_value());
    assert(!ph1.pop());

    std::cout << "  -> pairing_heap OK!\n\n";
}
