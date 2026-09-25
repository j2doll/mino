#include <iostream>
#include <exception>

// 분리된 테스트 함수 선언
extern void test_helper_functions();
extern void test_conversion_functions();
extern void test_formatting_functions();
extern void test_parsing_functions();
extern void test_current_time_functions();

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "   mino::core::datetime Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;

    try {
        test_helper_functions();
        test_conversion_functions();
        test_formatting_functions();
        test_parsing_functions();
        test_current_time_functions();

        std::cout << "\n=========================================" << std::endl;
        std::cout << "   ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
        std::cout << "=========================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
