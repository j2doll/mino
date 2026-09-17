#include <iostream>
#include <cassert>
#include <utility>

#include "mino/core/container/container.hpp"

void test_devector_all_public() {
    std::cout << "[Testing devector - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using dvi = container::devector<int>;

    dvi::value_type v = 1;
    (void)v;

    dvi::allocator_type alloc;
    (void)alloc;

    dvi::size_type st = 0;
    (void)st;

    dvi::difference_type dt = 0;
    (void)dt;

    // 1. 생성자
    dvi dv1;

    dvi dv2(5, alloc); // 5개의 기본값으로 초기화된 devector 생성
    assert(dv2.size() == 5); // dv2[0, 0, 0, 0, 0] 상태

    dvi dv3 = { 10, 20, 30 }; // initializer_list로 초기화
    assert(dv3.size() == 3); // dv3[10, 20, 30] 상태

    // 복사 & 이동 생성자
    dvi dv_copy(dv3); // 복사 생성자
    assert(dv_copy.size() == 3); // dv_copy[10, 20, 30] 상태

    dvi dv_move(std::move(dv_copy)); // 이동 생성자. 이동 후 dv_copy는 비어있음
    assert(dv_move.size() == 3); // dv_move[10, 20, 30] 상태

    // 복사 & 이동 대입 연산자
    dv1 = dv_move; // 복사 대입
    assert(dv1.size() == 3); // dv1[10, 20, 30] 상태

    dv2 = std::move(dv_move); // 이동 대입
    assert(dv2.size() == 3); // dv2[10, 20, 30] 상태

    // 2. 용량 및 공간 관련
    assert(!dv1.empty());
    assert(dv1.size() == 3);
    assert(dv1.capacity() >= 3);
    (void)dv1.free_front();
    (void)dv1.free_back();

    // 3. Element Access
    assert(dv3[0] == 10);
    assert(dv3.at(1) != nullptr && *dv3.at(1) == 20);
    assert(dv3.front() == 10);
    assert(dv3.back() == 30);
    assert(dv3.data() != nullptr);

    const dvi& const_dv = dv3;
    assert(const_dv[0] == 10);
    assert(const_dv.at(1) != nullptr && *const_dv.at(1) == 20);
    assert(const_dv.front() == 10);
    assert(const_dv.back() == 30);
    assert(const_dv.data() != nullptr);

    // Out-of-range now returns nullptr instead of throwing
    assert(dv3.at(99) == nullptr);
    assert(const_dv.at(99) == nullptr);

    // 4. 반복자 (Iterators)
    int sum = 0;
    for (auto it = dv3.begin(); it != dv3.end(); ++it)
        sum += *it;
    for (auto it = const_dv.cbegin(); it != const_dv.cend(); ++it)
        sum += *it;
    assert(sum == 120);

    // 5. Modifiers
    dvi dv_mod;
    int val = 5;
    dv_mod.push_back(val);             // lvalue
    dv_mod.push_back(10);              // rvalue
    dv_mod.push_front(1);              // rvalue
    dv_mod.push_front(val);            // lvalue
    dv_mod.emplace_back(15);           // emplace_back
    dv_mod.emplace_front(0);            // emplace_front

    // 상태: 0, 5, 1, 5, 10, 15
    assert(dv_mod.front() == 0);
    assert(dv_mod.back() == 15);

    dv_mod.pop_front();
    assert(dv_mod.front() == 5);

    dv_mod.pop_back();
    assert(dv_mod.back() == 10);

    dv_mod.clear();
    assert(dv_mod.empty());

    std::cout << "  -> devector OK!\n\n";
}
