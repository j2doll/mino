#include "test_common.hpp"

void test_korean_numeric() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Korean numeric formatters");

    auto str1 = mcs::to_string<int64_t>(1234567890LL);
    auto sep1 = mcs::add_separator(str1, 3, ',');
    TEST_CHECK(sep1 == "1,234,567,890");

    auto str2 = mcs::to_string<double>(-1234567.89, 2);
    auto sep2 = mcs::add_separator(str2, 3, ',');
    TEST_CHECK(sep2 == "-1,234,567.89");

    auto str3 = mcs::to_string<int>(12345678);
    auto sep3 = mcs::add_separator(str3, 4, ',');
    TEST_CHECK(sep3 == "1234,5678");

    bool include_comma_1 = false;
    TEST_CHECK(mcs::to_human_readable_korean("10000", include_comma_1) == "1만");

    bool include_comma_2 = true;
    TEST_CHECK(mcs::to_human_readable_korean("123456789", include_comma_2) == "1억 2,345만 6,789");

    bool include_comma_3 = false;
    TEST_CHECK(mcs::to_human_readable_korean("-50000000", include_comma_3) == "-5000만");
}
