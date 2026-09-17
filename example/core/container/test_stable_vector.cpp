#include <iostream>
#include <cassert>
#include <utility>

#include "mino/core/container/container.hpp"

void test_stable_vector_all_public() {
    std::cout << "[Testing stable_vector - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    container::stable_vector<int> sv;

    assert(sv.empty());
    assert(sv.size() == 0);

    // 1. 요소 추가 및 참조/포인터 보관
    sv.push_back(10);
    sv.push_back(20);

    int* ptr10 = &sv[0];
    const int& ref20 = sv[1];

    assert(*ptr10 == 10);
    assert(ref20 == 20);

    // 2. 대량 삽입으로 내부 인덱스 테이블 재할당 유발
    for (int i = 30; i <= 200; i += 10) {
        sv.push_back(i);
    }
    assert(sv.size() == 20);

    // 재할당 후에도 기존 요소의 포인터 및 참조가 무효화되지 않음을 확인
    assert(*ptr10 == 10);
    assert(ref20 == 20);

    // 3. 요소 수정
    sv[0] = 99;
    assert(*ptr10 == 99);

    // 4. at 및 범위 기반 접근
    assert(sv.at(1) != nullptr && *sv.at(1) == 20);
    assert(sv.at(999) == nullptr);
    assert(sv.front() == 99);
    assert(sv.back() == 200);

    // 5. pop_back
    sv.pop_back();
    assert(sv.back() == 190);
    assert(sv.size() == 19);

    // 6. clear
    sv.clear();
    assert(sv.empty());
    assert(sv.size() == 0);

    std::cout << "  -> stable_vector OK!\n\n";
}
