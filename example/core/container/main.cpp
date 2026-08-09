#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <vector>
#include <utility>
#include <algorithm>

#include "mino/core/container/bimap.hpp"
#include "mino/core/container/binomial_heap.hpp"
#include "mino/core/container/circular_buffer.hpp"
#include "mino/core/container/concurrent_queue.hpp"
#include "mino/core/container/d_ary_heap.hpp"
#include "mino/core/container/devector.hpp"
#include "mino/core/container/fibonacci_heap.hpp"
#include "mino/core/container/flat_map.hpp"
#include "mino/core/container/flat_multimap.hpp"
#include "mino/core/container/flat_multiset.hpp"

// ============================================================================
// 1. bimap 검증
// ============================================================================
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

// ============================================================================
// 2. binomial_heap 검증
// ============================================================================
void test_binomial_heap_all_public() {
    std::cout << "[Testing binomial_heap - All Public Members]" << std::endl;

    // 타입 정의 검증
    namespace container = mino::core::container;
    using bhint = container::binomial_heap<int>;

    bhint::value_type v = 10; // value_type 검증
    (void)v; // unused variable

    bhint::compare_type comp; // compare_type 검증
    (void)comp; // unused variable

    bhint::size_type s = 0; // size_type 검증
    (void)s; // unused variable

    bhint bh1;
    assert(bh1.empty());
    assert(bh1.size() == 0);

    // 1. push & emplace & node 생성자
    bhint::node* n1 = bh1.push(10); // bh1[10]
    (void)n1; // unused variable

    bhint::node* n2 = bh1.emplace(30); // bh1[30, 10] (std::less 기준 최댓값 우선)
    (void)n2; // unused variable

    bhint::node* n3 = bh1.push(20); // bh1[30, 20, 10]
    (void)n3; // unused variable

    assert(!bh1.empty());
    assert(bh1.size() == 3);

    // 2. top (std::less 기준 최댓값 우선)
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 30);
    }

    // 3. merge
    bhint bh2;
    bh2.push(50);
    bh2.push(40);

    bh1.merge(bh2); // bh1[30, 20, 10] + bh2[50, 40] -> bh1[50, 40, 30, 20, 10]
    assert(bh1.size() == 5);
    assert(bh2.empty());
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 50);
    }

    // 자기 자신과의 merge / empty와의 merge 처리
    bh1.merge(bh1); // 자기 자신과 merge 시, 아무 동작도 하지 않음
    bh1.merge(bh2); // empty 힙과 merge 시, 아무 동작도 하지 않음

    // 4. pop
    assert( bh1.pop()); // 50 제거 -> bh1[40, 30, 20, 10]
    {
        auto top_opt = bh1.top();
        assert(top_opt.has_value() && top_opt.value() == 40);
    }

    // 5. clear
    bh1.clear();
    assert(bh1.empty());
    assert(bh1.size() == 0);

    // Non-throwing checks (was exception tests before)
    {
        auto top_value = bh1.top(); // 비어있는 힙에서 top 시도
        assert(!top_value.has_value()); // top()은 std::optional 반환, 비어있으면 std::nullopt
    }

    {
        auto pop_result = bh1.pop(); // 비어있는 힙에서 pop 시도
        assert(!pop_result); // pop()은 bool 반환, 비어있으면 false
    }

    std::cout << "  -> binomial_heap OK!\n\n";
}

// ============================================================================
// 3. circular_buffer 검증
// ============================================================================
void test_circular_buffer_all_public() {
    std::cout << "[Testing circular_buffer - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using cbint = container::circular_buffer<int>;

    // capacity == 0 case: constructor no longer throws; check is_valid()
    cbint invalid_cb(0);
    assert(!invalid_cb.is_valid());

    cbint cb(3); // 용량 3인 circular_buffer 생성

    // 1. 용량 및 상태 함수
    assert(cb.capacity() == 3); // capacity는 3
    assert(cb.size() == 0); // 초기 상태에서 size는 0
    assert(cb.is_empty()); // 초기 상태에서 is_empty()는 true
    assert(!cb.is_full()); // 초기 상태에서 is_full()는 false

    // 2. push_back
    cb.push_back(10); // [10]
    cb.push_back(20); // [10, 20]
    cb.push_back(30); // [10, 20, 30]
    assert(cb.is_full()); // 용량 3이므로 full 상태
    assert(cb.size() == 3); // size는 3

    // 3. front, back (now return std::optional<T>)
    {
        auto f = cb.front();
        assert(f.has_value() && f.value() == 10);
    }
    {
        auto b = cb.back();
        assert(b.has_value() && b.value() == 30);
    }

    // 4. 오버플로우 push_back (가장 오래된 10 덮어씀)
    cb.push_back(40); // [20, 30, 40] (10이 제거되고 40이 추가됨)
    {
        auto f2 = cb.front();
        assert(f2.has_value() && f2.value() == 20);
    }
    {
        auto b2 = cb.back();
        assert(b2.has_value() && b2.value() == 40);
    }

    // 5. operator[] (Non-const & Const) - operator[] is unchecked; we only access valid indices
    assert(cb[0] == 20); // [20, 30, 40]에서 index 0은 20
    assert(cb[1] == 30); // [20, 30, 40]에서 index 1은 30
    assert(cb[2] == 40); // [20, 30, 40]에서 index 2는 40

    cb[0] = 25; // [25, 30, 40]으로 변경
    assert(cb[0] == 25); // [25, 30, 40]에서 index 0은 25

    const auto& const_cb = cb; // const_cb는 cb의 const 참조
    assert(const_cb[0] == 25);

    // Instead of expecting operator[] to throw, validate size and avoid out-of-range access
    assert(cb.size() == 3);

    // 6. pop_front
    auto item = cb.pop_front(); // cb[25, 30, 40] -> cb[30, 40]이 되고, item은 25
    assert(item.has_value() && item.value() == 25);
    assert(cb.size() == 2); // 현재 size는 2

    // 7. clear
    cb.clear();
    assert(cb.is_empty());
    assert(cb.size() == 0);
    assert(!cb.front().has_value());
    assert(!cb.back().has_value());
    assert(!cb.pop_front().has_value());

    std::cout << "  -> circular_buffer OK!\n\n";
}

// ============================================================================
// 4. concurrent_queue 검증
// ============================================================================
void test_concurrent_queue_all_public() {
    std::cout << "[Testing concurrent_queue - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    using cqstr = container::concurrent_queue<std::string>; // 문자열 동시성 큐
    using overflow_policy = container::overflow_policy;  

    // 1. 생성자, 용량, 정책 조회
    cqstr cq(2, overflow_policy::reject_new); // 용량 2, reject_new 정책인 큐를 생성
    // reject_new는 큐가 가득 찼을 때 새 데이터를 거부하는 정책
    assert(cq.is_bounded()); // 크기 제한 여부 확인
    assert(cq.capacity() == 2); // capacity 확인
    assert(cq.get_overflow_policy() == overflow_policy::reject_new); // 정책 확인
    assert(cq.empty()); // 초기 상태에서 empty 확인
    assert(cq.size() == 0); // size 확인

    // 무제한 큐
    using cqint = container::concurrent_queue<int>;
    cqint unbounded_cq(0); // 무제한 큐(용량 0) 생성
    assert(!unbounded_cq.is_bounded()); // 크기 제한 여부 확인

    // 2. enqueue (const&, &&, emplace)
    std::string s1 = "Hello";
    assert(cq.enqueue(s1) == true); // lvalue로 인큐 -> ["Hello"]                   
    assert(cq.enqueue(std::string("World")) == true); // rvalue로 인큐 -> ["Hello", "World"]
    assert(cq.size() == 2); // ["Hello", "World"]이므로 size는 2

    // reject_new 오버플로우
    assert(cq.enqueue("Overflow") == false);
    // reject_new 정책이므로 큐가 가득 찼을 때 enqueue()를 하면, 새 데이터를 거부하고 false 반환

    // drop_oldest 테스트
    cqint cq_drop(2, container::overflow_policy::drop_oldest); // 크기가 2인 drop_oldest 정책 큐 생성
    // drop_oldest는 큐가 가득 찼을 때, 가장 오래된 데이터를 버리고 새 데이터를 추가하는 정책
    cq_drop.enqueue(1); // [1]
    cq_drop.enqueue(2); // [1, 2]
    cq_drop.enqueue(3); // 1 버려짐 -> [2, 3]

    int popped_val;
    assert(cq_drop.try_dequeue(popped_val) // [2, 3]에서 2를 꺼냄
        && popped_val == 2);
    // 현재 cq_drop[3] 이며, size는 1

    // 3. emplace
    unbounded_cq.emplace(100); // 무제한 큐에 100 추가
    assert(unbounded_cq.size() == 1);

    // 4. try_dequeue
    std::string out;
    assert(cq.try_dequeue(out) == true // ["Hello", "World"]에서 Hello를 꺼냄
        && out == "Hello");
    // 현재 cq["World"] 이며, size는 1

    // 5. dequeue_if
    bool dequeued = cq.dequeue_if(out,
        [](const std::string& head, std::size_t sz) { return head == "World" && sz == 1; }
    ); // head가 "World"이고, size가 1일 때만 dequeue
    assert(dequeued && out == "World");

    // 비어있을 때 try_dequeue 및 dequeue_if
    assert(cq.try_dequeue(out) == false);
    assert(cq.dequeue_if(out, [](const std::string&, std::size_t) { return true; }) == false);

    // 6. wait_dequeue (스레드 활용)
    std::thread t([&cq]() {
            std::string val;
            cq.wait_dequeue(val); // 큐가 비어있으면 항목이 들어올 때까지 대기
            assert(val == "DelayedData"); // wait_dequeue가 완료되면 val은 "DelayedData"가 됨
        });

    cq.enqueue("DelayedData");
    t.join(); // wait_dequeue가 완료될 때까지 대기

    // 7. clear
    cq.enqueue("A");
    cq.enqueue("B");
    assert(cq.clear() == 2);
    assert(cq.empty());

    std::cout << "  -> concurrent_queue OK!\n\n";
}

// ============================================================================
// 5. d_ary_heap 검증
// ============================================================================
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

// ============================================================================
// 6. devector 검증
// ============================================================================
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

// ============================================================================
// 7. fibonacci_heap 검증
// ============================================================================
void test_fibonacci_heap_all_public() {
    std::cout << "[Testing fibonacci_heap - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 타입 정의 검증
    using FH = container::fibonacci_heap<int>;
    FH::value_type v = 1;
    FH::compare_type comp;
    FH::size_type s = 0;
    (void)v; (void)comp; (void)s;

    FH fh1;
    assert(fh1.empty());
    assert(fh1.size() == 0);

    // 1. push & emplace & node 생성자 / handle_type
    FH::handle_type h1 = fh1.push(10);
    FH::handle_type h2 = fh1.emplace(30);
    FH::handle_type h3 = fh1.push(20);
    (void)h1; (void)h2; (void)h3;

    assert(!fh1.empty());
    assert(fh1.size() == 3);

    // 2. top (std::less 기준 최댓값)
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 30);
    }

    // 3. merge
    FH fh2;
    fh2.push(50);
    fh2.push(40);

    fh1.merge(fh2);
    assert(fh1.size() == 5);
    assert(fh2.empty());
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 50);
    }

    // 자기 자신과의 merge / empty와의 merge
    fh1.merge(fh1);
    fh1.merge(fh2);

    // 4. pop
    assert(fh1.pop()); // 50 제거
    {
        auto t = fh1.top();
        assert(t.has_value() && t.value() == 40);
    }

    // 5. clear
    fh1.clear();
    assert(fh1.empty());
    assert(fh1.size() == 0);

    // Non-throwing checks
    {
        auto t = fh1.top();
        assert(!t.has_value());
    }
    {
        assert(!fh1.pop());
    }

    std::cout << "  -> fibonacci_heap OK!\n\n";
}

// ============================================================================
// 8. flat_map 검증
// ============================================================================
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
    assert(fm3.at(1) == "One");
    assert(const_fm.at(1) == "One");

    try {
        (void)fm3.at(99);
        assert(false);
    }
    catch (const std::out_of_range&) {
    }

    try {
        (void)const_fm.at(99);
        assert(false);
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

// ============================================================================
// 9. flat_multimap 검증
// ============================================================================
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

// ============================================================================
// 10. flat_multiset 검증
// ============================================================================
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

// ============================================================================
// 메인 함수
// ============================================================================
int main() {
    std::cout << "========================================================\n";
    std::cout << " Starting All Public Member Verification Tests\n";
    std::cout << "========================================================\n\n";

    try {
        test_bimap_all_public();
        test_binomial_heap_all_public();
        test_circular_buffer_all_public();
        test_concurrent_queue_all_public();
        test_d_ary_heap_all_public();
        test_devector_all_public();
        test_fibonacci_heap_all_public();
        test_flat_map_all_public();
        test_flat_multimap_all_public();
        test_flat_multiset_all_public();

        std::cout << "========================================================\n";
        std::cout << " ALL TESTS PASSED SUCCESSFULLY!\n";
        std::cout << "========================================================\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Test Exception Caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
