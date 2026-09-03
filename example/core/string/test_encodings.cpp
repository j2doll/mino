#include "test_common.hpp"

void test_encodings() {
    namespace mcs = mino::core::string;
    TEST_SECTION("encoding_function & console encoding");

    std::string utf8_sample = "한글 테스트 ABC 123";

    // UTF-8 <-> UTF-16
    std::u16string u16_out;
    TEST_CHECK(mcs::utf8_to_utf16(utf8_sample, u16_out));

    std::string utf8_back;
    TEST_CHECK(mcs::utf16_to_utf8(u16_out, utf8_back));
    TEST_CHECK(utf8_back == utf8_sample);

    // UTF-8 <-> wstring
    std::wstring w_out = mcs::utf8_to_utf16(utf8_sample);
    TEST_CHECK(!w_out.empty());
    TEST_CHECK(mcs::utf16_to_utf8(w_out) == utf8_sample);

    // UTF-8 <-> UTF-32
    std::u32string u32_out;
    TEST_CHECK(mcs::utf8_to_utf32(utf8_sample, u32_out));

    std::string utf8_from_u32;
    TEST_CHECK(mcs::utf32_to_utf8(u32_out, utf8_from_u32));
    TEST_CHECK(utf8_from_u32 == utf8_sample);

    // UTF-16 <-> UTF-32
    std::u32string u32_from_u16;
    TEST_CHECK(mcs::utf16_to_utf32(u16_out, u32_from_u16));

    std::u16string u16_from_u32;
    TEST_CHECK(mcs::utf32_to_utf16(u32_from_u16, u16_from_u32));
    TEST_CHECK(u16_from_u32 == u16_out);

    // CP949 / EUC-KR
    std::string cp949_out;
    if (mcs::utf8_to_cp949(utf8_sample, cp949_out)) {
        std::string utf8_from_cp949;
        TEST_CHECK(mcs::cp949_to_utf8(cp949_out, utf8_from_cp949));
        TEST_CHECK(utf8_from_cp949 == utf8_sample);

        std::string cp949_from_u16;
        TEST_CHECK(mcs::utf16_to_cp949(u16_out, cp949_from_u16));

        std::u16string u16_from_cp949;
        TEST_CHECK(mcs::cp949_to_utf16(cp949_from_u16, u16_from_cp949));
        TEST_CHECK(u16_from_cp949 == u16_out);
    }

    // ISO-2022-KR, JOHAB, MacKorean
    std::string iso_out;
    if (mcs::utf8_to_iso2022kr(utf8_sample, iso_out)) {
        std::string utf8_from_iso;
        TEST_CHECK(mcs::iso2022kr_to_utf8(iso_out, utf8_from_iso));
        TEST_CHECK(!utf8_from_iso.empty());
    }

    std::string johab_out;
    if (mcs::utf8_to_johab(utf8_sample, johab_out)) {
        std::string utf8_from_johab;
        TEST_CHECK(mcs::johab_to_utf8(johab_out, utf8_from_johab));
        TEST_CHECK(!utf8_from_johab.empty());
    }

    std::string mac_out;
    if (mcs::utf8_to_mackorean(utf8_sample, mac_out)) {
        std::string utf8_from_mac;
        TEST_CHECK(mcs::mackorean_to_utf8(mac_out, utf8_from_mac));
        TEST_CHECK(!utf8_from_mac.empty());
    }

    try {
        TEST_CHECK(mcs::utf8_to_utf16_or_throw(utf8_sample) == u16_out);
        TEST_CHECK(mcs::utf16_to_utf8_or_throw(u16_out) == utf8_sample);
        TEST_CHECK(mcs::utf8_to_utf32_or_throw(utf8_sample) == u32_out);
        TEST_CHECK(mcs::utf32_to_utf8_or_throw(u32_out) == utf8_sample);
        TEST_CHECK(mcs::utf16_to_utf32_or_throw(u16_out) == u32_out);
        TEST_CHECK(mcs::utf32_to_utf16_or_throw(u32_out) == u16_out);
    }
    catch (const std::exception& ex) {
        eprint(std::endl, tce("[EXCEPTION ERROR] "), ex.what());
        TEST_CHECK(false);
    }

    // Console encoding
    std::string console_bytes = mcs::to_console_encoding(utf8_sample);
    TEST_CHECK(!console_bytes.empty());

    std::string from_console = mcs::from_console_encoding(console_bytes);
    TEST_CHECK(!from_console.empty());
}
