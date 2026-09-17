#include <iostream>
#include <cassert>
#include <utility>

#include "mino/core/container/container.hpp"

void test_small_vector_all_public() {
    std::cout << "[Testing small_vector - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // N=4: 4개 이하 원소는 스택 내부 버퍼(SBO)에 유지
    container::small_vector<int, 4> sv;
    assert(sv.empty());
    assert(sv.size() == 0);
    assert(sv.capacity() == 4);
    assert(sv.is_on_stack());

    // 1. 스택 버퍼 내 삽입
    sv.push_back(10);
    sv.push_back(20);
    sv.emplace_back(30);
    assert(sv.is_on_stack());
    assert(sv.size() == 3);
    assert(sv.front() == 10);
    assert(sv.back() == 30);

    // 2. SBO 한도 초과 -> 힙 동적 메모리 자동 전환
    sv.push_back(40);
    sv.push_back(50);
    assert(!sv.is_on_stack());
    assert(sv.size() == 5);
    assert(sv.capacity() >= 5);
    assert(sv.back() == 50);

    // 3. 인덱스 및 포인터 접근 (at)
    assert(sv[0] == 10);
    assert(sv.at(1) != nullptr && *sv.at(1) == 20);
    assert(sv.at(99) == nullptr);

    // 4. pop_back 및 shrink_to_fit (스택 버퍼 복귀 검증)
    sv.pop_back(); // 50 제거
    sv.pop_back(); // 40 제거
    assert(sv.size() == 3);
    sv.shrink_to_fit();
    assert(sv.is_on_stack());

    // 5. 복사 및 이동 시맨틱
    container::small_vector<int, 4> sv_copy = sv;
    assert(sv_copy.size() == 3);
    assert(sv_copy.front() == 10);

    container::small_vector<int, 4> sv_move = std::move(sv_copy);
    assert(sv_move.size() == 3);

    // 6. clear
    sv.clear();
    assert(sv.empty());
    assert(sv.size() == 0);

    std::cout << "  -> small_vector OK!\n\n";
}
