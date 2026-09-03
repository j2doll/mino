#include "test_common.hpp"

void test_trim() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Trim");

    std::string s1 = "  \t hello \r\n ";
    mcs::trim(s1);
    TEST_CHECK(s1 == "hello");

    std::string s2 = "   left";
    mcs::ltrim(s2);
    TEST_CHECK(s2 == "left");

    std::string s3 = "right   ";
    mcs::rtrim(s3);
    TEST_CHECK(s3 == "right");

    TEST_CHECK(mcs::ltrim_copy("  abc") == "abc");
    TEST_CHECK(mcs::rtrim_copy("abc  ") == "abc");
    TEST_CHECK(mcs::trim_copy("  abc  ") == "abc");
}
