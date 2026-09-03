#include "test_common.hpp"

void test_parsing_wildcard() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Parsing & Wildcard");

    TEST_CHECK(mcs::is_digits("1234567890"));
    TEST_CHECK(!mcs::is_digits("123a45"));
    TEST_CHECK(!mcs::is_digits(""));

    int64_t num64 = 0;
    TEST_CHECK(mcs::try_parse_int64("1234567890123", num64));
    TEST_CHECK(num64 == 1234567890123LL);
    TEST_CHECK(!mcs::try_parse_int64("123abc", num64));

    double dbl = 0.0;
    TEST_CHECK(mcs::try_parse_double("3.141592", dbl));
    TEST_CHECK(std::abs(dbl - 3.141592) < 1e-6);
    TEST_CHECK(!mcs::try_parse_double("invalid", dbl));

    TEST_CHECK(mcs::wildcard_match("test_file.cpp", "test_*.cpp"));
    TEST_CHECK(mcs::wildcard_match("sample_01.txt", "sample_??.txt"));
    TEST_CHECK(!mcs::wildcard_match("sample_001.txt", "sample_??.txt"));
}
