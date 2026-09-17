#include <iostream>
#include <cassert>

#include "mino/core/container/container.hpp"

void test_fibonacci_heap_all_public() {
    std::cout << "[Testing fibonacci_heap - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using FH = container::fibonacci_heap<int>;
    FH::value_type v = 1;
    FH::compare_type comp;
    FH::size_type s = 0;
    (void)v; (void)comp; (void)s;

    FH fh1;
    assert(fh1.empty());
    assert(fh1.size() == 0);

    // 1. push & emplace & node 생성자 / handle_type
    FH::handle_type h1 = fh1.push(10);
    FH::handle_type h2 = fh1.emplace(30);
    FH::handle_type h3 = fh1.push(20);
    (void)h1; (void)h2; (void)h3;

    assert(!fh1.empty());
    assert(fh1.size() == 3);

    // 2. top (std::less 기준 최댓값)
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 30);
    }

    // 3. merge
    FH fh2;
    fh2.push(50);
    fh2.push(40);

    fh1.merge(fh2);
    assert(fh1.size() == 5);
    assert(fh2.empty());
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 50);
    }

    // 자기 자신과의 merge / empty와의 merge
    fh1.merge(fh1);
    fh1.merge(fh2);

    // 4. pop
    assert(fh1.pop()); // 50 제거
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 40);
    }

    // 5. clear
    fh1.clear();
    assert(fh1.empty());
    assert(fh1.size() == 0);

    // Non-throwing checks
    {
        auto t = fh1.top();
        assert(!t.has_value());
    }
    {
        assert(!fh1.pop());
    }

    std::cout << "  -> fibonacci_heap OK!\n\n";
}
