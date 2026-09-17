#include <iostream>
#include <cassert>
#include <utility>

#include "mino/core/container/container.hpp"

void test_skew_heap_all_public() {
    std::cout << "[Testing skew_heap - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using SH = container::skew_heap<int>;

    SH h1;
    assert(h1.empty());
    assert(h1.size() == 0);

    // 1. push & emplace
    h1.push(10);
    h1.push(5);
    h1.emplace(20);
    h1.push(15);
    h1.push(3);
    h1.push(7);

    assert(!h1.empty());
    assert(h1.size() == 6);
    assert(h1.top().has_value() && h1.top().value() == 20);

    // 2. pop
    assert(h1.pop());
    assert(h1.top().has_value() && h1.top().value() == 15);
    assert(h1.size() == 5);

    // 3. merge
    SH h2;
    h2.push(30);
    h2.push(8);
    h2.push(12);

    h1.merge(h2);
    assert(h1.size() == 8);
    assert(h2.empty());
    assert(h1.top().has_value() && h1.top().value() == 30);

    // 4. 이동 시맨틱
    SH h3 = std::move(h1);
    assert(h1.empty());
    assert(h3.size() == 8);
    assert(h3.top().value() == 30);

    // 5. clear
    h3.clear();
    assert(h3.empty());
    assert(!h3.top().has_value());
    assert(!h3.pop());

    std::cout << "  -> skew_heap OK!\n\n";
}
