#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <cassert>
#include <cstdint>

#include "mino/core/memory/memory.hpp"
#include "mino/core/memory/fixed_memory_pool.hpp"
#include "mino/core/memory/pool_allocator.hpp"
#include "mino/core/string/to_console_encoding.hpp"

//----------------------------------------------------------------
// 6. pool_allocator 및 STL 컨테이너 연동 테스트
//----------------------------------------------------------------

// 전역 스코프 타입 별칭 정의
template <typename T>
using pool_allocator = mino::core::memory::pool_allocator<T>;

void test_pool_allocator_with_stl() {
    using fixed_memory_pool = mino::core::memory::fixed_memory_pool;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] pool_allocator 및 STL 컨테이너(std::list, std::vector) 연동 테스트...")
        << std::endl;

    // 1. std::list 연동 (노드 단위 할당 시 메모리 풀 활용)
    try {
        fixed_memory_pool list_pool(sizeof(int) + 32, 256); // list_pool 을 생성 (int + list 노드 오버헤드 고려, 256개 블록)
        pool_allocator<int> alloc(&list_pool); // list_pool 메모리 풀과 연동되는 pool_allocator 생성

        std::list<int, pool_allocator<int>> pool_list(alloc); // std::list 생성 시 pool_allocator를 사용하여 메모리 풀과 연동

        for (int i = 1; i <= 5; ++i) {
            pool_list.push_back(i * 10);
        }

        assert(pool_list.size() == 5);

        int expected = 10;
        for (int val : pool_list) {
            assert(val == expected);
            expected += 10;
        }
        // expected: 60 (10(초기), 20, 30, 40, 50 순으로 증가)

        std::cout
            << to_console_encoding(" -> std::list 연동 성공 (원소 수: ")
            << pool_list.size() << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << to_console_encoding("std::list 연동 실패: ") << e.what() << std::endl;
        return;
    }

    // 2. std::vector 연동 (n > 1 연속 블록 요청 시 글로벌 힙 자동 폴백 확인)
    try {
        fixed_memory_pool vec_pool(sizeof(double), 64); // vec_pool 을 생성 (double 크기 고려, 64개 블록)
        pool_allocator<double> alloc(&vec_pool); // vec_pool 메모리 풀과 연동되는 pool_allocator 생성

        std::vector<double, pool_allocator<double>> pool_vec(alloc); // std::vector 생성 시 pool_allocator를 사용하여 메모리 풀과 연동

        assert(pool_vec.capacity() == 0);
        pool_vec.reserve(10);
        // 사용자가 실수로 노드 기반 컨테이너(`std::list`, `std::map`)가 아닌
        // 연속 메모리 컨테이너(`std::vector`, `std::string`)에 `pool_allocator`를 지정하더라도,
        // 프로그램이 죽거나 미정의 동작(UB)을 일으키지 않고
        // 유연하게 글로벌 힙으로 우회하여 정상 작동함을 테스트하는 코드.
        //
        // 1. `std::vector`와 `fixed_memory_pool`의 특성 차이
        //    - `fixed_memory_pool`: 단일 크기의 블록을 프리 리스트로 관리하므로,
        //       원소 1개(n = 1) 단위 할당에만 특화되어 있습니다.
        //    - `std::vector`: 모든 원소가 메모리상에 끊김 없이 연속된 배열 형태로 존재해야 합니다.
        // 
        // 2. `reserve(10)` 호출 시 발생하는 내부 동작
        //    - `reserve(10)`을 실행하면 `std::vector`는
        //      내부적으로 할당자의 `allocate(10)`을 호출합니다 (n = 10).
        //    - 이때 앞서 작성한 `pool_allocator::allocate`의 분기 로직이 동작합니다:
        //    ```cpp
        //    T* allocate(std::size_t n) {
        //        // 1. n == 1 (단일 객체, 예: std::list 노드): 메모리 풀 사용
        //        if (pool_ != nullptr && n == 1 && sizeof(T) <= pool_->block_size()) {
        //            return static_cast<T*>(pool_->allocate());
        //        }
        //    
        //        // 2. n > 1 (연속 메모리): 풀을 쓰지 않고 전역 힙으로 자동 우회
        //        return static_cast<T*>(::operator new(n * sizeof(T)));
        //    }
        //    ```
        //    - n = 10 이므로 조건문 `n == 1`을 만족하지 않습니다.
        //    - 따라서 고정 메모리 풀을 건드려 메모리가 깨지는 대신,
        //      안전하게 `::operator new`를 통해 시스템 힙에서 연속 공간 10개분을 할당받게 됩니다.
        //
        assert(pool_vec.capacity() == 10); // 재할당 없이 담을 수 있는 최대 원소 공간의 크기

        for (int i = 0; i < 5; ++i) {
            pool_vec.push_back(i * 1.5);
        }

        assert(pool_vec.size() == 5); // 현재 벡터 안에 실제로 생성되어 들어있는 원소의 개수
        assert(pool_vec[0] == 0.0);
        assert(pool_vec[2] == 3.0);

        std::cout
            << to_console_encoding(" -> std::vector 연동 및 힙 폴백 성공 (원소 수: ")
            << pool_vec.size() << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << to_console_encoding("std::vector 연동 실패: ") << e.what() << std::endl;
        return;
    }
}
