#include <iostream>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/core/exception/exception.hpp"

namespace {
    void step_three(int arg) {
        namespace mcstr = mino::core::string;
        namespace mce = mino::core::exception;

        int thirty_times = arg * 30;
        std::string exception_message =
            std::string("An error occurred during data processing: ") +
            std::to_string(thirty_times);

        throw mce::stack_traced_exception(exception_message);
    }

    void step_two(int arg) {
        int twelve_times = arg * 20;
        step_three(twelve_times);
    }

    void step_one(int arg) {
        int ten_times = arg * 10;
        step_two(ten_times);
    }
} // anonymous namespace

void test_stack_traced_exception() {
    namespace fs = ::std::filesystem;

    namespace mce = mino::core::exception;
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    auto tce = mcs::to_console_encoding;

    try {
        step_one(1);
    }
    catch (const mce::stack_traced_exception& e) {
        auto exc_string = e.to_string();
        
        mcsp::eprintln("[stack_traced_exception][예외 발생] {}", exc_string);
    }
    catch (const std::exception& e) {
        auto exc_string = std::string(e.what());
        mcsp::eprintln("[stack_traced_exception][예외 발생] 기타 표준 예외: {}", exc_string);
    }
    catch (...) {
        mcsp::eprintln("[stack_traced_exception][예외 발생] 알 수 없는 예외가 발생했습니다.");
    }

} // void test_stack_traced_exception()

