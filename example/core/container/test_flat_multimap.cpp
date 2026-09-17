#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <utility>

#include "mino/core/container/container.hpp"

void test_flat_multimap_all_public() {
    std::cout << "[Testing flat_multimap - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using FMM = container::flat_multimap<int, std::string>;
    FMM::key_type k = 1;
    FMM::mapped_type m = "A";
    FMM::value_type vt = { 1, "A" };
    FMM::key_compare kc;
    FMM::size_type st = 0;
    (void)k; (void)m; (void)vt; (void)kc; (void)st;

    // 1. 생성자
    container::flat_multimap<int, std::string> fmm1;
    container::flat_multimap<int, std::string> fmm2(kc);

    std::vector<std::pair<int, std::string>> init_vec = { {1, "One_1"}, {2, "Two"} };
    container::flat_multimap<int, std::string> fmm3(init_vec.begin(), init_vec.end());
    container::flat_multimap<int, std::string> fmm4 = { {1, "One_1"}, {1, "One_2"} };

    // 2. 용량 및 반복자
    assert(!fmm4.empty());
    assert(fmm4.size() == 2);
    fmm1.reserve(10);
    assert(fmm1.capacity() >= 10);

    auto it = fmm4.begin();
    auto e = fmm4.end();
    (void)it; (void)e;

    const auto& const_fmm = fmm4;
    auto cit = const_fmm.begin();
    auto cbi = const_fmm.cbegin();
    auto cei = const_fmm.cend();
    (void)cit; (void)cbi; (void)cei;

    // 3. 삽입 (insert, emplace)
    std::pair<int, std::string> p = { 1, "One_3" };
    fmm4.insert(p);                                   // const value_type&
    fmm4.insert(std::make_pair(3, "Three"));           // value_type&&
    fmm4.insert(init_vec.begin(), init_vec.end());    // range
    fmm4.insert({ {4, "Four"}, {4, "Four_2"} });         // initializer_list
    fmm4.emplace(5, "Five");                           // emplace

    // 4. 탐색 (find, count, lower_bound, upper_bound, equal_range)
    assert(fmm4.find(1) != fmm4.end());
    assert(const_fmm.find(1) != const_fmm.end());

    assert(fmm4.count(1) >= 3);

    auto lb = fmm4.lower_bound(1);
    auto ub = fmm4.upper_bound(1);
    (void)lb; (void)ub;

    auto clb = const_fmm.lower_bound(1);
    auto cub = const_fmm.upper_bound(1);
    (void)clb; (void)cub;

    auto eq = fmm4.equal_range(1);
    auto ceq = const_fmm.equal_range(1);
    (void)eq; (void)ceq;

    // 5. 삭제 (erase)
    fmm4.erase(fmm4.begin());
    fmm4.erase(fmm4.begin(), fmm4.begin() + 2);
    size_t removed_cnt = fmm4.erase(4); // 키 4 모두 삭제
    assert(removed_cnt == 2);

    // clear
    fmm4.clear();
    assert(fmm4.empty());

    std::cout << "  -> flat_multimap OK!\n\n";
}
