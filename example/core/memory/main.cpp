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

extern void test_basic_and_pod(); 
extern void test_string_and_vector();
extern void test_vector_of_structs();
extern void test_deep_copier();
extern void test_fixed_memory_pool();
extern void test_pool_allocator_with_stl();

//----------------------------------------------------------------
// main 진입점
//----------------------------------------------------------------

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== Mino Core Memory 테스트 시작 ===")
        << std::endl << std::endl;

    // 기존 메모리 직렬화 및 복사 테스트
    test_basic_and_pod();
    std::cout << std::endl;

    test_string_and_vector();
    std::cout << std::endl;

    test_vector_of_structs();
    std::cout << std::endl;

    test_deep_copier();
    std::cout << std::endl;

    test_fixed_memory_pool();
    std::cout << std::endl;

    test_pool_allocator_with_stl();
    std::cout << std::endl;

    std::cout
        << std::endl
        << to_console_encoding("=== 모든 테스트 통과 ===")
        << std::endl;

    return 0;
}
