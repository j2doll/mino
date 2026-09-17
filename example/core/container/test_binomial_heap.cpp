#include <iostream>
#include <cassert>

#include "mino/core/container/container.hpp"

void test_binomial_heap_all_public() {
    std::cout << "[Testing binomial_heap - All Public Members]" << std::endl;

    // 타입 정의 검증
    namespace container = mino::core::container;
    using bhint = container::binomial_heap<int>;

    bhint::value_type v = 10; // value_type 검증
    (void)v; // unused variable

    bhint::compare_type comp; // compare_type 검증
    (void)comp; // unused variable

    bhint::size_type s = 0; // size_type 검증
    (void)s; // unused variable

    bhint bh1;
    assert(bh1.empty());
    assert(bh1.size() == 0);

    // 1. push & emplace & node 생성자
    bhint::node* n1 = bh1.push(10); // bh1[10]
    (void)n1; // unused variable

    bhint::node* n2 = bh1.emplace(30); // bh1[30, 10] (std::less 기준 최댓값 우선)
    (void)n2; // unused variable

    bhint::node* n3 = bh1.push(20); // bh1[30, 20, 10]
    (void)n3; // unused variable

    assert(!bh1.empty());
    assert(bh1.size() == 3);

    // 2. top (std::less 기준 최댓값 우선)
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 30);
    }

    // 3. merge
    bhint bh2;
    bh2.push(50);
    bh2.push(40);

    bh1.merge(bh2); // bh1[30, 20, 10] + bh2[50, 40] -> bh1[50, 40, 30, 20, 10]
    assert(bh1.size() == 5);
    assert(bh2.empty());
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 50);
    }

    // 자기 자신과의 merge / empty와의 merge 처리
    bh1.merge(bh1); // 자기 자신과 merge 시, 아무 동작도 하지 않음
    bh1.merge(bh2); // empty 힙과 merge 시, 아무 동작도 하지 않음

    // 4. pop
    assert(bh1.pop()); // 50 제거 -> bh1[40, 30, 20, 10]
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 40);
    }

    // 5. clear
    bh1.clear();
    assert(bh1.empty());
    assert(bh1.size() == 0);

    // Non-throwing checks (was exception tests before)
    {
        auto top_value = bh1.top(); // 비어있는 힙에서 top 시도
        assert(!top_value.has_value()); // top()은 std::optional 반환, 비어있으면 std::nullopt
    }

    {
        auto pop_result = bh1.pop(); // 비어있는 힙에서 pop 시도
        assert(!pop_result); // pop()은 bool 반환, 비어있으면 false
    }

    std::cout << "  -> binomial_heap OK!\n\n";
}
