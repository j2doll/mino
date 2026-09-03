#include "test_common.hpp"

void test_replace() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Replace");

    std::string s = "foo bar foo baz";
    TEST_CHECK(mcs::replace(s, "foo", "qux") == "qux bar qux baz");
    TEST_CHECK(mcs::replace(s, "", "qux") == s);

    std::string s_inplace = "a-b-a-b";
    mcs::replace_all_inplace(s_inplace, "a", "x");
    TEST_CHECK(s_inplace == "x-b-x-b");

    TEST_CHECK(mcs::replace_first("banana", "an", "XX") == "bXXana");
    TEST_CHECK(mcs::replace_last("banana", "an", "XX") == "banXXa");

    std::vector<std::pair<std::string, std::string>> map = {
        {"{name}", "Mino"},
        {"{action}", "Test"}
    };
    TEST_CHECK(mcs::replace_all_map("Hello {name}, do {action}!", map) == "Hello Mino, do Test!");
}
