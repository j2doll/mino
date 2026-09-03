#include "test_common.hpp"

void test_affix_removal() {
    namespace mcs = mino::core::string;
    TEST_SECTION("string_basic - Prefix/Suffix removal");

    std::string pref = "PREFIX_content";
    TEST_CHECK(mcs::remove_prefix(pref, "PREFIX_"));
    TEST_CHECK(pref == "content");
    TEST_CHECK(!mcs::remove_prefix(pref, "NONEXIST"));

    std::string suff = "content_SUFFIX";
    TEST_CHECK(mcs::remove_suffix(suff, "_SUFFIX"));
    TEST_CHECK(suff == "content");

    TEST_CHECK(mcs::removed_prefix("lib_file.so", "lib_") == "file.so");
    TEST_CHECK(mcs::removed_suffix("file.tar.gz", ".gz") == "file.tar");
}
