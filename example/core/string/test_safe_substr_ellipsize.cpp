#include "test_common.hpp"

void test_safe_substr_ellipsize() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Safe Substr & Ellipsize");

    TEST_CHECK(mcs::safe_substr("hello", 1, 100) == "ello");
    TEST_CHECK(mcs::safe_substr("hello", 10, 5) == "");

    TEST_CHECK(mcs::ellipsize("Very long text message", 10, "...") == "Very lo...");
    TEST_CHECK(mcs::ellipsize("Short", 10) == "Short");

    std::string utf8_kr = "안녕하세요 반갑습니다";
    auto ellipsized_utf8_kr = mcs::ellipsize_utf8_safe(utf8_kr, 5, "..");
#ifdef _WIN32
    std::string cp949_out;
    if (!mcs::utf8_to_cp949(ellipsized_utf8_kr, cp949_out)) {
        assert(false);
    }
#endif
    TEST_CHECK(ellipsized_utf8_kr == "안녕하세요..");
}
