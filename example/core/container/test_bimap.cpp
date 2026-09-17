#include <iostream>
#include <string>
#include <cassert>

#include "mino/core/container/container.hpp"

void test_bimap_all_public() {
    std::cout << "[Testing bimap - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using bmis = container::bimap<int, std::string>;

    bmis bm;

    // 1. size, empty, clear
    assert(bm.empty());
    assert(bm.size() == 0);

    // 2. insert
    assert(bm.insert(1, "One") == true);
    assert(bm.insert(2, "Two") == true);
    assert(bm.insert(1, "DuplicateKey") == false); // 이미 키 값인 1이 존재
    assert(bm.size() == 2);
    assert(!bm.empty());

    // 3. force_insert
    bm.force_insert(1, "Uno"); // 기존 (1, "One") -> (1, "Uno")로 강제 갱신
    // 현재 (1, "Uno")와 (2, "Two")만 남음
    bm.force_insert(3, "Uno"); // "Uno" 중복 -> 기존 1과의 매핑 강제 해제 후, (3, "Uno") 연결
    // 현재 (2, "Two")와 (3, "Uno")만 남음

    // 4. get_by_left, get_by_right
    auto res_l = bm.get_by_left(3); // 3에 매핑된 값 얻기 (해당 값은 "Uno")
    assert(res_l.has_value() && res_l.value() == "Uno");

    auto res_r = bm.get_by_right("Two"); // "Two"에 매핑된 키 얻기 (해당 키는 2)
    assert(res_r.has_value() && res_r.value() == 2);

    assert(!bm.get_by_left(1).has_value()); // 키가 1인 값은 현재 없음.

    // 5. erase_by_left, erase_by_right
    assert(bm.erase_by_left(2) == true); // 키가 2인 (2, "Two") 제거
    assert(bm.erase_by_left(999) == false); // 존재하지 않는 키 제거 시, false 반환

    assert(bm.erase_by_right("Uno") == true); // 값이 "Uno"인 (3, "Uno") 제거
    assert(bm.erase_by_right("NotExist") == false); // 존재하지 않는 값 제거 시, false 반환
    assert(bm.empty()); // 현재 모든 값이 제거되어 비어 있음

    // 6. begin(), end() 반복자 및 범위 기반 for문
    bm.insert(10, "Ten");
    bm.insert(20, "Twenty");

    int count = 0;
    for (auto it = bm.begin(); it != bm.end(); ++it) {
        count++;
    }
    assert(count == 2); // 반복자 순회 시, 2개의 요소 확인

    // clear
    bm.clear();
    assert(bm.empty());
    assert(bm.size() == 0);

    std::cout << "  -> bimap OK!\n\n";
}
