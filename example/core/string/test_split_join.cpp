#include "test_common.hpp"

void test_split_join() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Split and Join");

    auto sp1 = mcs::split("apple,banana,orange", ',');
    TEST_CHECK(sp1.size() == 3 &&
        sp1[0] == "apple" &&
        sp1[1] == "banana" &&
        sp1[2] == "orange");

    bool keep_empty_sp2 = true;
    auto sp2 = mcs::split("a::b::::c", "::", keep_empty_sp2);
    TEST_CHECK(sp2.size() == 4 &&
        sp2[0] == "a" &&
        sp2[1] == "b" &&
        sp2[2] == "" &&
        sp2[3] == "c");

    bool keep_empty_tok1 = false;
    auto tok1 = mcs::tokenize_any_of("one,two;three|four", ",;|", keep_empty_tok1);
    TEST_CHECK(tok1.size() == 4 &&
        tok1[0] == "one" &&
        tok1[1] == "two" &&
        tok1[2] == "three" &&
        tok1[3] == "four");

    bool drop_empty_sp_trim = true;
    auto sp_trim = mcs::split_trimmed(" a , b ,, c ", ',', drop_empty_sp_trim);
    TEST_CHECK(sp_trim.size() == 3 &&
        sp_trim[0] == "a" &&
        sp_trim[1] == "b" &&
        sp_trim[2] == "c");

    bool keep_empty_lines = false;
    auto lines = mcs::split_lines("line1\r\nline2\nline3\rline4", keep_empty_lines);
    TEST_CHECK(lines.size() == 4 &&
        lines[0] == "line1" &&
        lines[1] == "line2" &&
        lines[2] == "line3" &&
        lines[3] == "line4");

    std::vector<std::string> joined_items = { "A", "B", "C" };
    TEST_CHECK(mcs::join(joined_items, " - ") == "A - B - C");
    TEST_CHECK(mcs::join({}, ",") == "");
}
