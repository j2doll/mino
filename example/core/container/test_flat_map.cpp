#include <iostream>
#include <string>
#include <cassert>
#include <utility>
#include <stdexcept>

#include "mino/core/container/container.hpp"

void test_flat_map_all_public() {
    std::cout << "[Testing flat_map - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using FM = container::flat_map<int, std::string>;
    FM::key_type k = 1;
    FM::mapped_type m = "A";
    FM::value_type vt = { 1, "A" };
    FM::key_compare kc;
    FM::size_type st = 0;
    (void)k; (void)m; (void)vt; (void)kc; (void)st;

    // 1. 생성자
    container::flat_map<int, std::string> fm1;
    container::flat_map<int, std::string> fm2(kc);
    container::flat_map<int, std::string> fm3 = { {3, "Three"}, {1, "One"} };

    // 2. 용량
    assert(!fm3.empty());
    assert(fm3.size() == 2);
    fm1.reserve(10);

    // 3. 반복자
    auto it_b = fm3.begin();
    auto it_e = fm3.end();
    assert(it_b->first == 1);
    (void)it_e;

    const auto& const_fm = fm3;
    auto cit_b = const_fm.begin();
    auto cit_cb = const_fm.cbegin();
    auto cit_ce = const_fm.cend();
    assert(cit_b->first == 1);
    (void)cit_cb; (void)cit_ce;

    // 4. 원소 접근 (at, operator[])
    auto fm3_opt = fm3.at(1);
    if (fm3_opt) {
        assert(fm3_opt.value() == "One");
    }
    else {
        assert(false); // at(1)에서 값이 없으면 실패
    }

    try {
        auto fm3_opt2 = fm3.at(99); // 99는 범위 밖이고, 값이 없어야 함.
        if (fm3_opt2) {
            assert(false); // at(99)에서 값이 있으면 실패
        }
        else {
            assert(true); // at(99)에서 값이 없으면 성공
        }
    }
    catch (const std::out_of_range&) {
    }

    fm3[2] = "Two";              // const key_type&
    int key_tmp = 4;
    fm3[std::move(key_tmp)] = "Four"; // key_type&&
    assert(fm3.size() == 4);

    // 5. 수정자 (insert, emplace, erase)
    std::pair<int, std::string> p1 = { 5, "Five" };
    auto ins_res1 = fm3.insert(p1);                 // const value_type&
    assert(ins_res1.second == true);

    auto ins_res2 = fm3.insert(std::make_pair(6, "Six")); // value_type&&
    assert(ins_res2.second == true);

    auto emp_res = fm3.emplace(7, "Seven");         // emplace
    assert(emp_res.second == true);

    // erase
    fm3.erase(fm3.find(7));
    assert(fm3.erase(6) == 1);
    assert(fm3.erase(999) == 0);

    // 6. 탐색 (find, contains)
    assert(fm3.find(5) != fm3.end());
    assert(const_fm.find(5) != const_fm.end());
    assert(fm3.contains(5) == true);
    assert(fm3.contains(999) == false);

    // clear
    fm3.clear();
    assert(fm3.empty());

    std::cout << "  -> flat_map OK!\n\n";
}
