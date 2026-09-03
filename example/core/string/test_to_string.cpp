#include "test_common.hpp"

void test_to_string() {
    namespace mcs = mino::core::string;
    TEST_SECTION("to_string & is_equal");

    double val1 = 3.1415926535;
    double val2 = 3.1415926536;

    std::string s_fixed = mcs::to_string<double>(val1, 4);
    TEST_CHECK(s_fixed == "3.1416");

    std::string s_default = mcs::to_string<double>(val1);
    TEST_CHECK(!s_default.empty());

    TEST_CHECK(mcs::is_equal<double>(val1, val2, 2));
    TEST_CHECK(!mcs::is_equal<double>(val1, val2, 10));

    float f1 = 1.0f;
    float f2 = 1.0f;
    TEST_CHECK(mcs::is_equal<float>(f1, f2));
}
