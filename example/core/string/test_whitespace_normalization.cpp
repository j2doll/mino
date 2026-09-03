#include "test_common.hpp"

void test_whitespace_normalization() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Whitespace / Newline Normalization");

    TEST_CHECK(mcs::is_blank("  \t\r\n "));
    TEST_CHECK(!mcs::is_blank("  a  "));

    TEST_CHECK(mcs::collapse_spaces("  hello   world  \t from  C++  ") == "hello world from C++");
    TEST_CHECK(mcs::normalize_newlines("line1\r\nline2\rline3\nline4", "\n") == "line1\nline2\nline3\nline4");
}
