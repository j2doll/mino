#include "test_common.hpp"

void test_case_contains() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Case, Contains, Starts/Ends With");

    TEST_CHECK(mcs::to_lower("HeLLo 123!") == "hello 123!");
    TEST_CHECK(mcs::to_upper("HeLLo 123!") == "HELLO 123!");

    TEST_CHECK(mcs::contains("Hello World", "World"));
    TEST_CHECK(mcs::contains("Hello World", ""));
    TEST_CHECK(!mcs::contains("Hello World", "xyz"));

    TEST_CHECK(mcs::starts_with("prefix_test", "prefix"));
    TEST_CHECK(!mcs::starts_with("prefix_test", "test"));
    TEST_CHECK(mcs::ends_with("prefix_test", "test"));
    TEST_CHECK(!mcs::ends_with("prefix_test", "prefix"));

    TEST_CHECK(mcs::iequals("Hello", "hELLO"));
    TEST_CHECK(!mcs::iequals("Hello", "World"));

    TEST_CHECK(mcs::icontains("Beautiful Day", "tIfUl"));
    TEST_CHECK(mcs::istarts_with("Hello World", "heLL"));
    TEST_CHECK(mcs::iends_with("Hello World", "rLD"));

    std::vector<std::string> prefixes = { "pre", "start", "init" };
    TEST_CHECK(mcs::starts_with_any("startup.cpp", prefixes));
    TEST_CHECK(!mcs::starts_with_any("main.cpp", prefixes));

    std::vector<std::string> suffixes = { ".cpp", ".hpp", ".h" };
    TEST_CHECK(mcs::ends_with_any("main.cpp", suffixes));
    TEST_CHECK(!mcs::ends_with_any("main.obj", suffixes));

    std::vector<std::string> items1 = { "interconnected", "intermediate", "intersection" };
    auto cprefix_i1 = mcs::common_prefix(items1);
    TEST_CHECK(cprefix_i1 == "inter");

    std::vector<std::string> items2 = { "running", "walking", "jumping" };
    TEST_CHECK(mcs::common_suffix(items2) == "ing");
}
