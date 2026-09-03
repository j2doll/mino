#include "test_common.hpp"

void test_tokenizer() {
    namespace mcs = mino::core::string;
    TEST_SECTION("tokenizer");

    auto toks = mcs::tokenize_string("alpha,beta\ngamma\rdelta", ",\n\r");
    TEST_CHECK(toks.size() == 4);
    TEST_CHECK(toks[0] == "alpha" &&
        toks[1] == "beta" &&
        toks[2] == "gamma" &&
        toks[3] == "delta");

    auto empty_tok = mcs::tokenize_string("", ",");
    TEST_CHECK(empty_tok.size() == 1 &&
        empty_tok[0].empty());
}
