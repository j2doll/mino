#include <iostream>
#include <cassert>
#include <functional>

#include "mino/core/container/container.hpp"

void test_d_ary_heap_all_public() {
    std::cout << "[Testing d_ary_heap - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using dahi3 = container::d_ary_heap<int, 3>; // 3-ary integer heap

    dahi3::value_type v = 1; // value_type 검증
    (void)v;

    dahi3::compare_type comp; // compare_type 검증
    (void)comp;

    dahi3::size_type s = 0; // size_type 검증
    (void)s;

    // 1. 생성자
    dahi3 heap1;
    assert(heap1.empty());
    assert(heap1.size() == 0);

    std::less<int> custom_comp; // 사용자 정의 비교 함수: std::less<int>를 사용하여 최댓값 우선으로 설정
    dahi3 heap2(custom_comp); // 사용자 정의 비교 함수로 생성

    // 2. push (const&, &&), emplace
    int x = 10;
    heap1.push(x);           // lvalue로 push -> heap1[10]
    heap1.push(30);          // rvalue로 push -> heap1[30, 10]
    heap1.emplace(20);       // emplace -> heap1[30, 10, 20]
    heap1.push(40);          // rvalue로 push -> heap1[40, 30, 10, 20]

    assert(!heap1.empty());
    assert(heap1.size() == 4);

    // 3. top (std::less 기준 최댓값 top)
    {
        auto top_opt = heap1.top();
        assert(top_opt.has_value() && top_opt.value() == 40);
    }

    // 4. pop
    assert(heap1.pop()); // 40이 나오고, heap1[30, 20, 10]이 됨
    {
        auto top_opt = heap1.top();
        assert(top_opt.has_value() && top_opt.value() == 30);
    }
    assert(heap1.pop()); // 30이 나오고, heap1[20, 10]이 됨
    {
        auto top_opt = heap1.top();
        assert(top_opt.has_value() && top_opt.value() == 20);
    }

    // 5. clear
    heap1.clear();
    assert(heap1.empty());
    assert(heap1.size() == 0);

    // Non-throwing checks (was exception tests before)
    {
        auto top_empty = heap1.top();
        assert(!top_empty.has_value());
    }
    {
        assert(!heap1.pop());
    }

    std::cout << "  -> d_ary_heap OK!\n\n";
}
