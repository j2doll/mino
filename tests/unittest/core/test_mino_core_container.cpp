#include "catch_amalgamated.hpp"

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>
#include <numeric>
#include <cstring>
#include <functional>
#include <atomic>
#include <array>

// mino core container 헤더 전체 include
#include "mino/core/container/bimap.hpp"
#include "mino/core/container/binomial_heap.hpp"
#include "mino/core/container/byte_buffer.hpp"
#include "mino/core/container/circular_buffer.hpp"
#include "mino/core/container/concurrent_queue.hpp"
#include "mino/core/container/d_ary_heap.hpp"
#include "mino/core/container/devector.hpp"
#include "mino/core/container/fibonacci_heap.hpp"
#include "mino/core/container/flat_map.hpp"
#include "mino/core/container/flat_multimap.hpp"
#include "mino/core/container/flat_multiset.hpp"
#include "mino/core/container/flat_set.hpp"
#include "mino/core/container/multi_array.hpp"
#include "mino/core/container/multi_index_container.hpp"
#include "mino/core/container/pairing_heap.hpp"
#include "mino/core/container/priority_queue.hpp"
#include "mino/core/container/red_black_tree.hpp"
#include "mino/core/container/skew_heap.hpp"
#include "mino/core/container/small_vector.hpp"
#include "mino/core/container/span.hpp"
#include "mino/core/container/stable_vector.hpp"
#include "mino/core/container/static_vector.hpp"
#include "mino/core/container/topic_queue.hpp"
#include "mino/core/container/work_stealing_deque.hpp"

// ============================================================================
// 1. bimap 테스트
// ============================================================================
TEST_CASE("bimap: Basic operations and bidirectional lookup", "[container][bimap]") {
    using namespace mino::core::container;

    bimap<int, std::string> bm;

    REQUIRE(bm.empty());
    REQUIRE(bm.size() == 0);

    SECTION("Insert unique pairs") {
        REQUIRE(bm.insert(1, "one"));
        REQUIRE(bm.insert(2, "two"));
        REQUIRE(bm.size() == 2);

        // 중복 키 삽입 거부 (Left 중복 및 Right 중복)
        REQUIRE_FALSE(bm.insert(1, "uno"));
        REQUIRE_FALSE(bm.insert(3, "two"));
        REQUIRE(bm.size() == 2);

        REQUIRE(bm.contains_left(1));
        REQUIRE(bm.contains_right("two"));
        REQUIRE_FALSE(bm.contains_left(3));
        REQUIRE_FALSE(bm.contains_right("three"));

        REQUIRE(bm.get_by_left(1).value() == "one");
        REQUIRE(bm.get_by_right("two").value() == 2);
    }

    SECTION("Force insert replaces existing associations") {
        bm.insert(1, "one");
        bm.insert(2, "two");

        // (1, "two") 강제 삽입 시 이전 (1, "one"), (2, "two") 관계 해제 및 갱신
        REQUIRE(bm.force_insert(1, "two"));
        REQUIRE(bm.size() == 1);
        REQUIRE(bm.get_by_left(1).value() == "two");
        REQUIRE(bm.get_by_right("two").value() == 1);
        REQUIRE_FALSE(bm.contains_left(2));
        REQUIRE_FALSE(bm.contains_right("one"));
    }

    SECTION("Erase operations") {
        bm.insert(10, "ten");
        bm.insert(20, "twenty");

        REQUIRE(bm.erase_by_left(10));
        REQUIRE_FALSE(bm.contains_left(10));
        REQUIRE_FALSE(bm.contains_right("ten"));
        REQUIRE(bm.size() == 1);

        REQUIRE(bm.erase_by_right("twenty"));
        REQUIRE(bm.empty());
    }
}

// ============================================================================
// 2. binomial_heap 테스트
// ============================================================================
TEST_CASE("binomial_heap: Push, Pop, Merge and Ordering", "[container][binomial_heap]") {
    using namespace mino::core::container;

    binomial_heap<int> heap;
    REQUIRE(heap.empty());

    SECTION("Max-Heap order") {
        heap.push(10);
        heap.push(30);
        heap.push(20);
        heap.push(5);

        REQUIRE(heap.size() == 4);
        REQUIRE(heap.top().value() == 30);

        REQUIRE(heap.pop());
        REQUIRE(heap.top().value() == 20);
        REQUIRE(heap.size() == 3);

        REQUIRE(heap.pop());
        REQUIRE(heap.top().value() == 10);

        REQUIRE(heap.pop());
        REQUIRE(heap.top().value() == 5);

        REQUIRE(heap.pop());
        REQUIRE(heap.empty());
        REQUIRE_FALSE(heap.pop());
    }

    SECTION("Merge two heaps") {
        binomial_heap<int> h1;
        h1.push(10);
        h1.push(40);

        binomial_heap<int> h2;
        h2.push(50);
        h2.push(20);

        h1.merge(h2);
        REQUIRE(h1.size() == 4);
        REQUIRE(h2.empty());
        REQUIRE(h1.top().value() == 50);

        REQUIRE(h1.pop());
        REQUIRE(h1.top().value() == 40);
    }
}

// ============================================================================
// 3. byte_buffer 테스트
// ============================================================================
TEST_CASE("byte_buffer: Read, Write, Peek and Compact", "[container][byte_buffer]") {
    using namespace mino::core::container;

    byte_buffer buf(16);
    REQUIRE(buf.is_empty());
    REQUIRE(buf.readable_bytes() == 0);

    const char* sample = "Hello, Mino!";
    std::size_t sample_len = std::strlen(sample);

    buf.write(sample, sample_len);
    REQUIRE(buf.readable_bytes() == sample_len);

    char peek_out[32] = { 0 };
    REQUIRE(buf.peek(peek_out, 5));
    REQUIRE(std::string(peek_out, 5) == "Hello");
    REQUIRE(buf.readable_bytes() == sample_len);

    char read_out[32] = { 0 };
    REQUIRE(buf.read(read_out, 5));
    REQUIRE(std::string(read_out, 5) == "Hello");
    REQUIRE(buf.readable_bytes() == sample_len - 5);

    buf.compact();
    REQUIRE(buf.readable_bytes() == sample_len - 5);

    char rest[32] = { 0 };
    REQUIRE(buf.read(rest, sample_len - 5));
    REQUIRE(std::string(rest, sample_len - 5) == ", Mino!");
    REQUIRE(buf.is_empty());
}

// ============================================================================
// 4. concurrent_byte_buffer 테스트
// ============================================================================
TEST_CASE("concurrent_byte_buffer: Thread-safe write and read", "[container][concurrent_byte_buffer]") {
    using namespace mino::core::container;

    concurrent_byte_buffer cbuf(64);
    const int val = 12345;

    cbuf.write(&val, sizeof(val));
    REQUIRE(cbuf.readable_bytes() == sizeof(val));

    int out_val = 0;
    REQUIRE(cbuf.read(&out_val, sizeof(out_val)));
    REQUIRE(out_val == val);
    REQUIRE(cbuf.is_empty());
}

// ============================================================================
// 5. circular_buffer 테스트
// ============================================================================
TEST_CASE("circular_buffer: FIFO, Overwrite and Indexing", "[container][circular_buffer]") {
    using namespace mino::core::container;

    circular_buffer<int> cb(3);
    REQUIRE(cb.is_valid());
    REQUIRE(cb.is_empty());
    REQUIRE(cb.capacity() == 3);

    cb.push_back(1);
    cb.push_back(2);
    REQUIRE(cb.size() == 2);
    REQUIRE(cb.front().value() == 1);
    REQUIRE(cb.back().value() == 2);

    cb.push_back(3);
    REQUIRE(cb.is_full());
    REQUIRE(cb[0] == 1);
    REQUIRE(cb[1] == 2);
    REQUIRE(cb[2] == 3);

    // 오버플로우: 가장 오래된 데이터(1) 덮어쓰기
    cb.push_back(4);
    REQUIRE(cb.size() == 3);
    REQUIRE(cb.front().value() == 2);
    REQUIRE(cb.back().value() == 4);
    REQUIRE(cb[0] == 2);
    REQUIRE(cb[1] == 3);
    REQUIRE(cb[2] == 4);

    REQUIRE(cb.pop_front().value() == 2);
    REQUIRE(cb.size() == 2);
}

// ============================================================================
// 6. concurrent_queue 테스트
// ============================================================================
TEST_CASE("concurrent_queue: Multi-threaded Producer-Consumer", "[container][concurrent_queue]") {
    using namespace mino::core::container;

    SECTION("Bounded Queue with drop_oldest policy") {
        concurrent_queue<int> q(2, overflow_policy::drop_oldest);
        q.enqueue(1);
        q.enqueue(2);
        q.enqueue(3); // 1 drop

        int out = 0;
        REQUIRE(q.try_dequeue(out));
        REQUIRE(out == 2);
        REQUIRE(q.try_dequeue(out));
        REQUIRE(out == 3);
        REQUIRE_FALSE(q.try_dequeue(out));
    }

    SECTION("Wait and notify between threads") {
        concurrent_queue<int> q;
        std::thread consumer([&q]() {
            int val = 0;
            q.wait_dequeue(val);
            REQUIRE(val == 999);
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        q.enqueue(999);
        consumer.join();
    }
}

// ============================================================================
// 7. d_ary_heap 테스트
// ============================================================================
TEST_CASE("d_ary_heap: 4-ary Max and Min heap operations", "[container][d_ary_heap]") {
    using namespace mino::core::container;

    SECTION("4-ary Max Heap") {
        d_ary_heap<int, 4> heap;
        heap.push(10);
        heap.push(50);
        heap.push(30);
        heap.push(70);
        heap.push(20);

        REQUIRE(heap.size() == 5);
        REQUIRE(heap.top().value() == 70);

        REQUIRE(heap.pop());
        REQUIRE(heap.top().value() == 50);
    }

    SECTION("3-ary Min Heap") {
        d_ary_heap<int, 3, std::greater<int>> min_heap;
        min_heap.push(20);
        min_heap.push(5);
        min_heap.push(15);

        REQUIRE(min_heap.top().value() == 5);
        min_heap.pop();
        REQUIRE(min_heap.top().value() == 15);
    }
}

// ============================================================================
// 8. devector 테스트
// ============================================================================
TEST_CASE("devector: Both front and back insertions and indexing", "[container][devector]") {
    using namespace mino::core::container;

    devector<int> dv;
    REQUIRE(dv.empty());

    dv.push_back(20);
    dv.push_back(30);
    dv.push_front(10);
    dv.push_front(0); // [0, 10, 20, 30]

    REQUIRE(dv.size() == 4);
    REQUIRE(dv.front() == 0);
    REQUIRE(dv.back() == 30);
    REQUIRE(dv[1] == 10);
    REQUIRE(dv[2] == 20);

    dv.pop_front();
    REQUIRE(dv.front() == 10);

    dv.pop_back();
    REQUIRE(dv.back() == 20);
    REQUIRE(dv.size() == 2);
}

// ============================================================================
// 9. fibonacci_heap 테스트
// ============================================================================
TEST_CASE("fibonacci_heap: Push, Pop and Merge", "[container][fibonacci_heap]") {
    using namespace mino::core::container;

    fibonacci_heap<int> heap;
    heap.push(10);
    heap.push(50);
    heap.push(30);

    REQUIRE(heap.size() == 3);
    REQUIRE(heap.top().value() == 50);

    REQUIRE(heap.pop());
    REQUIRE(heap.top().value() == 30);
    REQUIRE(heap.size() == 2);

    fibonacci_heap<int> heap2;
    heap2.push(40);
    heap2.push(60);

    heap.merge(heap2);
    REQUIRE(heap.size() == 4);
    REQUIRE(heap2.empty());
    REQUIRE(heap.top().value() == 60);
}

// ============================================================================
// 10. flat_map 테스트
// ============================================================================
TEST_CASE("flat_map: Key-value operations and binary search", "[container][flat_map]") {
    using namespace mino::core::container;

    flat_map<int, std::string> fm;
    fm[2] = "two";
    fm[1] = "one";
    fm[3] = "three";

    REQUIRE(fm.size() == 3);
    REQUIRE(fm.contains(2));
    REQUIRE_FALSE(fm.contains(99));

    REQUIRE(fm.at(1).value() == "one");
    REQUIRE(fm[3] == "three");

    REQUIRE(fm.erase(2) == 1);
    REQUIRE_FALSE(fm.contains(2));
    REQUIRE(fm.size() == 2);
}

// ============================================================================
// 11. flat_multimap 테스트
// ============================================================================
TEST_CASE("flat_multimap: Duplicate keys and range queries", "[container][flat_multimap]") {
    using namespace mino::core::container;

    flat_multimap<int, std::string> fmm;
    fmm.insert({ 1, "alpha" });
    fmm.insert({ 1, "beta" });
    fmm.insert({ 2, "gamma" });

    REQUIRE(fmm.size() == 3);
    REQUIRE(fmm.count(1) == 2);

    auto range = fmm.equal_range(1);
    std::vector<std::string> vals;
    for (auto it = range.first; it != range.second; ++it) {
        vals.push_back(it->second);
    }
    REQUIRE(vals == std::vector<std::string>{"alpha", "beta"});

    REQUIRE(fmm.erase(1) == 2);
    REQUIRE(fmm.size() == 1);
}

// ============================================================================
// 12. flat_set 테스트
// ============================================================================
TEST_CASE("flat_set: Unique sorted storage", "[container][flat_set]") {
    using namespace mino::core::container;

    flat_set<int> fs;
    REQUIRE(fs.insert(30).second);
    REQUIRE(fs.insert(10).second);
    REQUIRE(fs.insert(20).second);
    REQUIRE_FALSE(fs.insert(20).second); // 중복 삽입 실패

    REQUIRE(fs.size() == 3);
    REQUIRE(fs.contains(10));
    REQUIRE_FALSE(fs.contains(40));

    std::vector<int> sorted_vals(fs.begin(), fs.end());
    REQUIRE(sorted_vals == std::vector<int>{10, 20, 30});

    REQUIRE(fs.erase(20) == 1);
    REQUIRE(fs.size() == 2);
}

// ============================================================================
// 13. flat_multiset 테스트
// ============================================================================
TEST_CASE("flat_multiset: Duplicate sorted elements", "[container][flat_multiset]") {
    using namespace mino::core::container;

    flat_multiset<int> fms;
    fms.insert(10);
    fms.insert(20);
    fms.insert(10);

    REQUIRE(fms.size() == 3);
    REQUIRE(fms.count(10) == 2);
    REQUIRE(fms.erase(10) == 2);
    REQUIRE(fms.size() == 1);
}

// ============================================================================
// 14. multi_array 테스트
// ============================================================================
TEST_CASE("multi_array: Multi-dimensional indexing and strides", "[container][multi_array]") {
    using namespace mino::core::container;

    multi_array<int, 2> arr2d(3, 4);
    REQUIRE(arr2d.num_dimensions() == 2);
    REQUIRE(arr2d.size() == 12);

    arr2d(0, 0) = 100;
    arr2d(1, 2) = 200;
    arr2d(2, 3) = 300;

    REQUIRE(arr2d(0, 0) == 100);
    REQUIRE(arr2d(1, 2) == 200);
    REQUIRE(arr2d(2, 3) == 300);

    std::array<std::size_t, 2> idx = { 1, 2 };
    REQUIRE(arr2d[idx] == 200);

    // Row-major 연속 메모리 선형 인덱스 검증 (1 * 4 + 2 = 6)
    REQUIRE(arr2d.data()[6] == 200);
}

// ============================================================================
// 15. multi_index_container 테스트
// ============================================================================
TEST_CASE("multi_index_container: Multiple key lookups and consistency", "[container][multi_index_container]") {
    using namespace mino::core::container;

    struct TestUser {
        std::string name;
        int age;
        TestUser(std::string n, int a) : name(std::move(n)), age(a) {}
    };

    multi_index_container<TestUser, int, std::string> users;

    REQUIRE(users.insert(TestUser("Alice", 25), 1, "alice@example.com"));
    REQUIRE(users.insert(TestUser("Bob", 30), 2, "bob@example.com"));

    // Key 중복 시 삽입 실패 검증
    REQUIRE_FALSE(users.insert(TestUser("Fake Alice", 40), 1, "other@example.com"));
    REQUIRE_FALSE(users.insert(TestUser("Fake Bob", 50), 3, "bob@example.com"));
    REQUIRE(users.size() == 2);

    // 인덱스 기준 조회
    const auto* u1 = users.find_by_index<0>(1);
    REQUIRE(u1 != nullptr);
    REQUIRE(u1->name == "Alice");

    const auto* u2 = users.find_by_index<1>("bob@example.com");
    REQUIRE(u2 != nullptr);
    REQUIRE(u2->name == "Bob");

    // 인덱스 기준 삭제
    REQUIRE(users.erase_by_index<0>(1));
    REQUIRE(users.find_by_index<0>(1) == nullptr);
    REQUIRE(users.find_by_index<1>("alice@example.com") == nullptr);
    REQUIRE(users.size() == 1);
}

// ============================================================================
// 16. pairing_heap 테스트
// ============================================================================
TEST_CASE("pairing_heap: Push, Pop, Update and Erase", "[container][pairing_heap]") {
    using namespace mino::core::container;

    pairing_heap<int> ph;
    auto h1 = ph.push(10);
    auto h2 = ph.push(30);
    auto h3 = ph.push(20);

    REQUIRE(ph.size() == 3);
    REQUIRE(ph.top().value() == 30);

    // 핸들을 통한 값 갱신 (10 -> 50으로 변경 시 루트 승격)
    ph.update(h1, 50);
    REQUIRE(ph.top().value() == 50);

    // 특정 노드 핸들 삭제
    ph.erase(h2); // 30 삭제
    REQUIRE(ph.size() == 2);

    REQUIRE(ph.pop()); // 50 제거
    REQUIRE(ph.top().value() == 20);
}

// ============================================================================
// 17. priority_queue 테스트
// ============================================================================
TEST_CASE("priority_queue: Standard vector-backed binary heap", "[container][priority_queue]") {
    using namespace mino::core::container;

    priority_queue<int> pq;
    pq.push(10);
    pq.push(5);
    pq.push(30);

    REQUIRE(pq.top().value() == 30);
    REQUIRE(pq.pop());
    REQUIRE(pq.top().value() == 10);

    priority_queue<int, std::greater<int>> min_pq({ 5, 1, 9, 3 });
    REQUIRE(min_pq.top().value() == 1);
}

// ============================================================================
// 18. red_black_tree 테스트
// ============================================================================
TEST_CASE("red_black_tree: Insert, Nil sentinel, Color invariants and Merge", "[container][red_black_tree]") {
    using namespace mino::core::container;

    red_black_tree<int> rbt;
    REQUIRE(rbt.empty());

    rbt.insert(20);
    rbt.insert(10);
    rbt.insert(30);
    rbt.insert(5);

    REQUIRE(rbt.size() == 4);
    REQUIRE(rbt.contains(10));
    REQUIRE_FALSE(rbt.contains(99));

    const auto* root = rbt.get_root();
    REQUIRE(root != nullptr);
    REQUIRE(rbt.is_black(root)); // 루트 노드는 항상 BLACK

    const auto* left_child = rbt.get_node_at_path("L");
    REQUIRE(left_child != nullptr);
    REQUIRE(left_child->key == 10);

    red_black_tree<int> other;
    other.insert(25);
    rbt.merge(other);
    REQUIRE(rbt.contains(25));
    REQUIRE(rbt.size() == 5);
}

// ============================================================================
// 19. skew_heap 테스트
// ============================================================================
TEST_CASE("skew_heap: Push, Pop and Merge", "[container][skew_heap]") {
    using namespace mino::core::container;

    skew_heap<int> sh1;
    sh1.push(10);
    sh1.push(40);

    skew_heap<int> sh2;
    sh2.push(50);
    sh2.push(20);

    sh1.merge(sh2);
    REQUIRE(sh1.size() == 4);
    REQUIRE(sh2.empty());
    REQUIRE(sh1.top().value() == 50);

    REQUIRE(sh1.pop());
    REQUIRE(sh1.top().value() == 40);
}

// ============================================================================
// 20. small_vector 테스트
// ============================================================================
TEST_CASE("small_vector: SBO Stack to Heap transition and shrink_to_fit", "[container][small_vector]") {
    using namespace mino::core::container;

    small_vector<int, 4> sv;
    REQUIRE(sv.is_on_stack());

    sv.push_back(1);
    sv.push_back(2);
    sv.push_back(3);
    sv.push_back(4);
    REQUIRE(sv.size() == 4);
    REQUIRE(sv.is_on_stack()); // 4개까지는 스택 버퍼

    // 5번째 원소 삽입 시 힙으로 전환
    sv.push_back(5);
    REQUIRE(sv.size() == 5);
    REQUIRE_FALSE(sv.is_on_stack());

    // 다시 줄어들고 shrink_to_fit 호출 시 스택 버퍼로 복귀
    sv.pop_back();
    sv.pop_back(); // size = 3 (<= 4)
    sv.shrink_to_fit();
    REQUIRE(sv.is_on_stack());
    REQUIRE(sv.size() == 3);
    REQUIRE(sv[0] == 1);
    REQUIRE(sv[2] == 3);
}

// ============================================================================
// 21. span 테스트
// ============================================================================
TEST_CASE("span: Static and dynamic view over contiguous memory", "[container][span]") {
    using namespace mino::core::container;

    int raw_arr[] = { 1, 2, 3, 4, 5 };

    span<int> dyn_sp(raw_arr, 5);
    REQUIRE(dyn_sp.size() == 5);
    REQUIRE(dyn_sp.front() == 1);
    REQUIRE(dyn_sp.back() == 5);
    REQUIRE(dyn_sp[2] == 3);

    auto sub = dyn_sp.subspan(1, 3); // {2, 3, 4}
    REQUIRE(sub.size() == 3);
    REQUIRE(sub[0] == 2);
    REQUIRE(sub[2] == 4);

    // 고정 크기 span (Zero Overhead Storage)
    span<int, 5> fixed_sp(raw_arr);
    REQUIRE(fixed_sp.size() == 5);
}

// ============================================================================
// 22. stable_vector 테스트
// ============================================================================
TEST_CASE("stable_vector: Reference and pointer stability upon reallocation", "[container][stable_vector]") {
    using namespace mino::core::container;

    stable_vector<int> sv;
    sv.push_back(10);
    sv.push_back(20);

    int* ptr10 = &sv[0];
    int& ref20 = sv[1];

    // 대량 삽입으로 내부 포인터 인덱스 테이블 재할당 유발
    for (int i = 30; i <= 200; i += 10) {
        sv.push_back(i);
    }

    // 재할당 후에도 기존 요소의 메모리 주소와 참조가 불변인지 확인
    REQUIRE(*ptr10 == 10);
    REQUIRE(ref20 == 20);
    REQUIRE(&sv[0] == ptr10);
    REQUIRE(&sv[1] == &ref20);

    *ptr10 = 999;
    REQUIRE(sv[0] == 999);
}

// ============================================================================
// 23. static_vector 테스트
// ============================================================================
TEST_CASE("static_vector: Fixed capacity stack storage without dynamic allocation", "[container][static_vector]") {
    using namespace mino::core::container;

    static_vector<int, 3> vec;
    REQUIRE(vec.empty());
    REQUIRE(vec.capacity() == 3);

    REQUIRE(vec.push_back(10));
    REQUIRE(vec.push_back(20));
    REQUIRE(vec.push_back(30));
    REQUIRE(vec.size() == 3);

    // 용량 초과 시 예외 대신 false 반환 검증
    REQUIRE_FALSE(vec.push_back(40));

    REQUIRE(vec.front() == 10);
    REQUIRE(vec.back() == 30);
    REQUIRE(*vec.at(1) == 20);
    REQUIRE(vec.at(3) == nullptr);

    vec.pop_back();
    REQUIRE(vec.size() == 2);
    REQUIRE(vec.back() == 20);
}

// ============================================================================
// 24. topic_queue 테스트
// ============================================================================
TEST_CASE("topic_queue: Publish and subscribe asynchronous dispatch", "[container][topic_queue]") {
    using namespace mino::core::container;

    auto& tq = topic_queue::get_instance();
    std::atomic<int> received_sum{ 0 };

    // 토픽 구독 등록 (Stopped 상태에서 등록)
    REQUIRE(tq.subscribe("math/add", [&received_sum](const message_context& ctx) {
        int val = std::any_cast<int>(ctx.data);
        received_sum.fetch_add(val);
        }));

    tq.start();

    tq.publish("math/add", 10);
    tq.publish("math/add", 20);

    // 비동기 큐 작업 처리 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    tq.stop();

    REQUIRE(received_sum.load() == 30);
}

// ============================================================================
// 25. work_stealing_deque 테스트
// ============================================================================
TEST_CASE("work_stealing_deque: LIFO push/pop and FIFO steal", "[container][work_stealing_deque]") {
    using namespace mino::core::container;

    work_stealing_deque<int> wsd;

    wsd.push(1);
    wsd.push(2);
    wsd.push(3);
    REQUIRE(wsd.size() == 3);

    // 외부 워커가 FIFO 방식으로 전방(Front)에서 steal
    auto stolen = wsd.steal();
    REQUIRE(stolen.has_value());
    REQUIRE(stolen.value() == 1); // 1이 먼저 steal됨

    // 소유자 워커가 LIFO 방식으로 후방(Back)에서 pop
    auto popped = wsd.pop();
    REQUIRE(popped.has_value());
    REQUIRE(popped.value() == 3); // 가장 최근의 3이 pop됨

    REQUIRE(wsd.size() == 1);
    auto last = wsd.pop();
    REQUIRE(last.value() == 2);
    REQUIRE(wsd.empty());
}
