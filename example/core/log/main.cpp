#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/string/to_console_encoding.hpp"

extern void test_utility_functions();
extern void test_console_sink(); 
extern void test_rolling_file_sink();
extern void test_logger();

int main() {
    try {
        std::cout << "===========================================" << std::endl;
        std::cout << "        Starting Tinylog Unit Tests        " << std::endl;
        std::cout << "===========================================\n" << std::endl;

        test_utility_functions();
        test_console_sink();
        test_rolling_file_sink();
        test_logger();

        std::cout << "===========================================" << std::endl;
        std::cout << "     All Public Member Tests Passed!       " << std::endl;
        std::cout << "===========================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
