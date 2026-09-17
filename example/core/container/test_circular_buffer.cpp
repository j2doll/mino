#include <iostream>
#include <cassert>

#include "mino/core/container/container.hpp"

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
