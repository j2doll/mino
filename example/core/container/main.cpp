#include <iostream>
#include <exception>

// ============================================================================
// 테스트 함수 extern 선언
// ============================================================================
extern void test_bimap_all_public();
extern void test_binomial_heap_all_public();
extern void test_circular_buffer_all_public();
extern void test_concurrent_queue_all_public();
extern void test_d_ary_heap_all_public();
extern void test_devector_all_public();
extern void test_fibonacci_heap_all_public();
extern void test_flat_map_all_public();
extern void test_flat_multimap_all_public();
extern void test_flat_multiset_all_public();
extern void test_red_black_tree();

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
        test_red_black_tree();

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
