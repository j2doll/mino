#include <iostream>
#include <cassert>
#include <array>
#include <stdexcept>

#include "mino/core/container/container.hpp"

void test_multi_array_all_public() {
    std::cout << "[Testing multi_array - All Public Members]" << std::endl;

    namespace container = mino::core::container;

    // 1. 2D multi_array (3x4)
    container::multi_array<int, 2> arr2d(3, 4);
    assert(arr2d.num_dimensions() == 2);
    assert(arr2d.size() == 12);
    assert(!arr2d.empty());

    auto ext2d = arr2d.extents();
    assert(ext2d[0] == 3 && ext2d[1] == 4);

    auto str2d = arr2d.strides();
    assert(str2d[0] == 4 && str2d[1] == 1);

    // 연산자 () 및 []
    arr2d(0, 0) = 1;
    arr2d(1, 2) = 10;

    std::array<std::size_t, 2> idx2d = { 2, 3 };
    arr2d[idx2d] = 20;

    assert(arr2d(0, 0) == 1);
    assert(arr2d(1, 2) == 10);
    assert(arr2d(2, 3) == 20);
    assert(arr2d[idx2d] == 20);

    // at 범위 검사
    assert(arr2d.at(1, 2) == 10);
    bool out_of_bounds_thrown = false;
    try {
        arr2d.at(3, 0);
    }
    catch (const std::out_of_range&) {
        out_of_bounds_thrown = true;
    }
    assert(out_of_bounds_thrown);

    // 선형 데이터 접근 및 fill
    int* raw_data = arr2d.data();
    assert(raw_data[1 * 4 + 2] == 10);

    arr2d.fill(5);
    for (int v : arr2d) {
        assert(v == 5);
    }

    // 2. 3D multi_array (2x3x4)
    container::multi_array<double, 3> arr3d(2, 3, 4);
    assert(arr3d.num_dimensions() == 3);
    assert(arr3d.size() == 24);

    arr3d(0, 1, 2) = 3.14;
    arr3d(1, 2, 3) = 2.71;
    assert(arr3d(0, 1, 2) == 3.14);

    // 전처리기 매크로(assert) 쉼표 충돌 방지를 위해 std::array 변수 사용
    std::array<std::size_t, 3> idx3d = { 1, 2, 3 };
    assert(arr3d[idx3d] == 2.71);

    std::cout << "  -> multi_array OK!\n\n";
}
