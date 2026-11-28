#include "test_common.hpp"

void test_to_console_encoding() {
	namespace mcs = mino::core::string;
	TEST_SECTION("encoding_function");

    std::string utf8_sample = "한글 테스트 ABC 123";

    // Console encoding
    std::string console_bytes = mcs::to_console_encoding(utf8_sample);
    TEST_CHECK(!console_bytes.empty());

    std::string from_console = mcs::from_console_encoding(console_bytes);
    TEST_CHECK(!from_console.empty());

}
