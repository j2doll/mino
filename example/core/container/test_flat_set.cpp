#include <iostream>
#include <cassert>
#include <vector>

#include "mino/core/container/container.hpp"

void test_flat_set_all_public() {
    std::cout << "[Testing flat_set - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using FS = container::flat_set<int>;

    // 1. 생성자 및 용량
    FS s1;
    assert(s1.empty());
    assert(s1.size() == 0);

    FS s2 = { 10, 5, 15, 10 }; // 중복 제거 및 정렬 검증
    assert(s2.size() == 3);
    assert(!s2.empty());

    s1.reserve(10);
    assert(s1.capacity() >= 10);
    s1.shrink_to_fit();

    // 2. 삽입 (insert, emplace)
    auto [it1, ok1] = s1.insert(20);
    assert(ok1 && *it1 == 20);

    auto [it2, ok2] = s1.insert(20); // 중복 삽입 실패
    assert(!ok2 && it2 == it1);

    auto [it3, ok3] = s1.emplace(10);
    assert(ok3 && *it3 == 10);

    std::vector<int> range_data = { 30, 40 };
    s1.insert(range_data.begin(), range_data.end());
    assert(s1.size() == 4);

    // 3. 탐색 (find, contains, count, bounds)
    assert(s1.contains(20));
    assert(!s1.contains(999));
    assert(s1.count(20) == 1);
    assert(s1.count(999) == 0);

    auto f_it = s1.find(30);
    assert(f_it != s1.end() && *f_it == 30);

    auto lb = s1.lower_bound(20);
    auto ub = s1.upper_bound(20);
    assert(*lb == 20 && *ub == 30);

    // 4. 반복자 순회 (정렬 상태 검증)
    int prev_val = -1;
    for (const auto& val : s1) {
        assert(val > prev_val);
        prev_val = val;
    }

    // 5. 삭제 (erase)
    assert(s1.erase(20) == 1);
    assert(s1.erase(999) == 0);
    assert(!s1.contains(20));

    // 6. swap 및 clear
    s1.swap(s2);
    assert(s1.size() == 3);
    s1.clear();
    assert(s1.empty());

    std::cout << "  -> flat_set OK!\n\n";
}
