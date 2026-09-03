#include "test_common.hpp"

void test_padding_quotes() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Padding / Repeat / Quotes / Indent");

    TEST_CHECK(mcs::pad_left("42", 5, '0') == "00042");
    TEST_CHECK(mcs::pad_right("42", 5, ' ') == "42   ");
    TEST_CHECK(mcs::pad_center("test", 8, '-') == "--test--");

    TEST_CHECK(mcs::remove_chars("a-b-c-d", "-") == "abcd");
    TEST_CHECK(mcs::repeat("ab", 3) == "ababab");

    TEST_CHECK(mcs::strip_quotes("\"quoted\"", '"') == "quoted");
    TEST_CHECK(mcs::strip_quotes("no_quotes", '"') == "no_quotes");

    bool escape_1 = false;
    TEST_CHECK(mcs::quote("hello", '"', escape_1) == "\"hello\"");

    bool escape_2 = true;
    TEST_CHECK(mcs::quote("he\"llo", '"', escape_2) == "\"he\\\"llo\"");

    TEST_CHECK(mcs::surround_if_missing("foo", "[", "]") == "[foo]");
    TEST_CHECK(mcs::surround_if_missing("[foo]", "[", "]") == "[foo]");

    TEST_CHECK(mcs::indent_lines("a\nb\nc", "  ") == "  a\n  b\n  c");
}
