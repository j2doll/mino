#include <iostream>
#include <cassert>
#include <vector>

#include "mino/core/container/container.hpp"

void test_flat_multiset_all_public() {
    std::cout << "[Testing flat_multiset - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using FMS = container::flat_multiset<int>;
    FMS::key_type k = 1;
    FMS::value_type v = 1;
    FMS::key_compare kc;
    FMS::value_compare vc;
    FMS::allocator_type alloc;
    FMS::pointer p = nullptr;
    FMS::const_pointer cp = nullptr;
    FMS::reference ref = v;
    FMS::const_reference cref = v;
    FMS::size_type st = 0;
    FMS::difference_type dt = 0;
    (void)k; (void)v; (void)kc; (void)vc; (void)alloc; (void)p; (void)cp;
    (void)ref; (void)cref; (void)st; (void)dt;

    // 1. 생성자
    container::flat_multiset<int> fms1;
    container::flat_multiset<int> fms2(kc, alloc);
    container::flat_multiset<int> fms3(alloc);

    std::vector<int> init_vec = { 3, 1, 2, 1 };
    container::flat_multiset<int> fms4(init_vec.begin(), init_vec.end(), kc, alloc);
    container::flat_multiset<int> fms5 = { 5, 2, 5, 1 };

    // 2. 용량 관련
    assert(!fms5.empty());
    assert(fms5.size() == 4);
    assert(fms5.max_size() > 0);
    fms1.reserve(20);
    assert(fms1.capacity() >= 20);
    fms1.shrink_to_fit();

    // 3. 반복자 (정방향, 역방향, const)
    auto it_b = fms5.begin();
    auto it_e = fms5.end();
    auto rit_b = fms5.rbegin();
    auto rit_e = fms5.rend();
    (void)it_b; (void)it_e; (void)rit_b; (void)rit_e;

    const auto& const_fms5 = fms5;
    auto cit_b = const_fms5.begin();
    auto cit_cb = const_fms5.cbegin();
    auto cit_ce = const_fms5.cend();
    auto crit_b = const_fms5.rbegin();
    auto crit_cb = const_fms5.crbegin();
    auto crit_ce = const_fms5.crend();
    (void)cit_b; (void)cit_cb; (void)cit_ce; (void)crit_b; (void)crit_cb; (void)crit_ce;

    // 4. 수정자 (insert, emplace, erase)
    int val = 10;
    fms5.insert(val);                          // const value_type&
    fms5.insert(20);                           // value_type&&
    fms5.insert(init_vec.begin(), init_vec.end()); // InputIt
    fms5.insert({ 30, 40 });                     // initializer_list
    fms5.emplace(50);                          // emplace

    fms5.erase(fms5.begin());
    fms5.erase(fms5.begin(), fms5.begin() + 2);
    size_t removed = fms5.erase(5);            // 키 5 모두 삭제
    assert(removed == 2);

    // 5. 검색 (count, find, contains, lower/upper_bound, equal_range)
    fms4.insert(2);
    assert(fms4.count(2) == 2);

    const auto& const_fms4 = fms4;
    assert(fms4.find(2) != fms4.end());
    assert(const_fms4.find(2) != const_fms4.end());

    assert(fms4.contains(2) == true);
    assert(fms4.contains(999) == false);

    auto eq = fms4.equal_range(2);
    auto ceq = const_fms4.equal_range(2);
    (void)eq; (void)ceq;

    auto lb = fms4.lower_bound(2);
    auto clb = const_fms4.lower_bound(2);
    (void)lb; (void)clb;

    auto ub = fms4.upper_bound(2);
    auto cub = const_fms4.upper_bound(2);
    (void)ub; (void)cub;

    // 6. swap 및 clear
    fms1.swap(fms5);
    assert(!fms1.empty());
    fms1.clear();
    assert(fms1.empty());

    // 7. 옵저버 (key_comp, value_comp)
    auto kcomp = fms4.key_comp();
    auto vcomp = fms4.value_comp();
    (void)kcomp; (void)vcomp;

    std::cout << "  -> flat_multiset OK!\n\n";
}
