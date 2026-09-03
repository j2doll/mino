#include <iostream>
#include <stdexcept>

#include "mino/core/string/string.hpp"

// 세부 기능별 extern 선언
extern void test_trim();
extern void test_replace();
extern void test_case_contains();
extern void test_split_join();
extern void test_whitespace_normalization();
extern void test_padding_quotes();
extern void test_affix_removal();
extern void test_safe_substr_ellipsize();
extern void test_parsing_wildcard();
extern void test_korean_numeric();
extern void test_tokenizer();
extern void test_to_string();
extern void test_mutex_string();
extern void test_u8string();
extern void test_encodings();

int main() {
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce("Starting mino::core::string Test Suite"));
    print(tce("========================================"));

    try {
        test_trim();
        test_replace();
        test_case_contains();
        test_split_join();
        test_whitespace_normalization();
        test_padding_quotes();
        test_affix_removal();
        test_safe_substr_ellipsize();
        test_parsing_wildcard();
        test_korean_numeric();
        test_tokenizer();
        test_to_string();
        test_mutex_string();
        test_u8string();
        test_encodings();

        print(endl, tce("========================================"));
        print(tce("[SUCCESS] All tests passed successfully!"));
        print(tce("========================================"));
    }
    catch (const std::exception& ex) {
        eprint(endl, tce("[EXCEPTION ERROR] "), ex.what());
        return 1;
    }

    return 0;
}
