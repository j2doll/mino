#include <iostream>
#include <cassert>
#include <utility>

#include "mino/core/container/container.hpp"

void test_static_vector_all_public() {
    std::cout << "[Testing static_vector - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 최대 용량 5인 스택 벡터
    container::static_vector<int, 5> vec;
    assert(vec.empty());
    assert(vec.size() == 0);
    assert(vec.capacity() == 5);
    assert(vec.max_size() == 5);

    // 1. 데이터 삽입 (push_back, emplace_back)
    assert(vec.push_back(10) == true);
    assert(vec.push_back(20) == true);
    assert(vec.push_back(30) == true);

    assert(vec.size() == 3);
    assert(!vec.empty());
    assert(vec.front() == 10);
    assert(vec.back() == 30);

    // 2. 인덱스 및 포인터 접근 (at, operator[])
    assert(vec[1] == 20);
    assert(vec.at(2) != nullptr && *vec.at(2) == 30);
    assert(vec.at(99) == nullptr);

    // 3. 최대 용량 초과 방어 검증 (Non-throwing: 용량 초과 시 false)
    assert(vec.push_back(40) == true);
    assert(vec.push_back(50) == true);
    assert(vec.push_back(60) == false); // Capacity 5 초과
    assert(vec.size() == 5);

    // 4. pop_back 및 resize
    vec.pop_back(); // 50 제거
    assert(vec.size() == 4);
    assert(vec.back() == 40);

    vec.resize(2); // 2개로 축소
    assert(vec.size() == 2);
    assert(vec.back() == 20);

    // 5. swap
    container::static_vector<int, 5> other = { 100, 200, 300 };
    vec.swap(other);
    assert(vec.size() == 3 && vec[0] == 100);
    assert(other.size() == 2 && other[0] == 10);

    // 6. clear
    vec.clear();
    assert(vec.empty());
    assert(vec.size() == 0);

    std::cout << "  -> static_vector OK!\n\n";
}
