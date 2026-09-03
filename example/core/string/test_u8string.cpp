#include "test_common.hpp"

void test_u8string() {
    namespace mcsu8 = mino::core::string::u8;
    TEST_SECTION("u8string & u8str");

    // Global conversions
    std::string test_utf8 = "한글 및 English 123! 😀";
    auto u8_str_val = mcsu8::to_u8_string(test_utf8);
    TEST_CHECK(mcsu8::to_std_string(u8_str_val) == test_utf8);

    std::wstring ws = mcsu8::to_wstring(u8_str_val);
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(ws)) == test_utf8);

    std::u16string u16 = mcsu8::to_u16string(u8_str_val);
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(u16)) == test_utf8);

    std::u32string u32 = mcsu8::to_u32string(u8_str_val);
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(u32)) == test_utf8);

    // u8str class
    mcsu8::u8str ustr1;
    TEST_CHECK(ustr1.empty() && ustr1.size() == 0);

    mcsu8::u8str ustr2("Hello 유니코드");
    TEST_CHECK(!ustr2.empty());
    TEST_CHECK(ustr2.to_std_string() == "Hello 유니코드");
    mcsu8::u8str ustr3(ustr2);
    TEST_CHECK(ustr3 == ustr2);

    mcsu8::u8str ustr4(std::wstring(L"WideString 한글"));
    TEST_CHECK(!ustr4.empty());

    mcsu8::u8str ustr5(u16);
    mcsu8::u8str ustr6(u32);
    TEST_CHECK(ustr5 == ustr6);

    // Setters & value access
    ustr1.from_std_string("Test std string");
    TEST_CHECK(ustr1.to_std_string() == "Test std string");

    ustr1.from_cstr("Test C-string");
    TEST_CHECK(ustr1.to_std_string() == "Test C-string");

    ustr1.from_wstring(L"From WString");
    TEST_CHECK(!ustr1.empty());

    ustr1.from_u16string(u16);
    ustr1.from_u32string(u32);
    TEST_CHECK(ustr1.data() != nullptr);

    ustr1.clear();
    TEST_CHECK(ustr1.empty());

    // Parsers & Setters
    ustr1.from_int64(9876543210LL);
    int64_t parsed_i64 = 0;
    TEST_CHECK(ustr1.to_int64(parsed_i64) && parsed_i64 == 9876543210LL);

    ustr1.from_double(123.456);
    double parsed_dbl = 0.0;
    TEST_CHECK(ustr1.to_double(parsed_dbl) && std::abs(parsed_dbl - 123.456) < 1e-4);

    ustr1.from_bool(true);
    bool parsed_b = false;
    TEST_CHECK(ustr1.to_bool(parsed_b) && parsed_b == true);

    // Search and checks
    mcsu8::u8str search_target("가나다라_ABCD_가나다라");

    TEST_CHECK(search_target.starts_with("가나다"));
    TEST_CHECK(search_target.starts_with(mcsu8::u8str("가나다")));
    TEST_CHECK(search_target.ends_with("다라"));
    TEST_CHECK(search_target.ends_with(mcsu8::u8str("다라")));
    TEST_CHECK(search_target.contains("ABCD"));
    TEST_CHECK(search_target.contains(mcsu8::u8str("ABCD")));

    TEST_CHECK(search_target.index_of("ABCD") >= 0);
    TEST_CHECK(search_target.index_of(mcsu8::u8str("ABCD")) >= 0);
    TEST_CHECK(search_target.last_index_of("가나다라") > 0);
    TEST_CHECK(search_target.last_index_of(mcsu8::u8str("가나다라")) > 0);

    // Trim
    mcsu8::u8str trim_u("   \t가나다   \n");
    trim_u.trim();
    TEST_CHECK(trim_u.to_std_string() == "가나다");

    // Split & Replace
    mcsu8::u8str split_target("사과/오렌지/바나나/포도");
    auto split_res = split_target.split("/");
    TEST_CHECK(split_res.size() == 4 &&
        split_res[0].to_std_string() == "사과" &&
        split_res[1].to_std_string() == "오렌지" &&
        split_res[2].to_std_string() == "바나나" &&
        split_res[3].to_std_string() == "포도");

    mcsu8::u8str rep_target("foo bar foo baz");
    rep_target.replace("foo", "qux");
    TEST_CHECK(rep_target.to_std_string() == "qux bar foo baz");

    rep_target.replace_all(mcsu8::u8str("foo"), mcsu8::u8str("qux"));
    TEST_CHECK(rep_target.to_std_string() == "qux bar qux baz");

    // UTF-8 Codepoint based Left/Right/Mid/Reverse/Pad
    mcsu8::u8str cp_str("안녕하세요반갑습니다");
    TEST_CHECK(cp_str.left(2).to_std_string() == "안녕");
    TEST_CHECK(cp_str.right(4).to_std_string() == "갑습니다");
    TEST_CHECK(cp_str.mid(2, 3).to_std_string() == "하세요");
    TEST_CHECK(cp_str.substr_utf8(2, 3).to_std_string() == "하세요");

    mcsu8::u8str rev_str("123가나다");
    rev_str.reverse_utf8();
    TEST_CHECK(rev_str.to_std_string() == "다나가321");

    mcsu8::u8str pad_str("한글");
    pad_str.pad_left(5, '-');
    TEST_CHECK(pad_str.to_std_string() == "---한글");

    pad_str.pad_right(7, '+');
    TEST_CHECK(pad_str.to_std_string() == "---한글++");

    // Strip prefix/suffix & Case
    mcsu8::u8str strip_str("[TAG]Content[/TAG]");
    TEST_CHECK(strip_str.strip_prefix("[TAG]"));
    TEST_CHECK(strip_str.strip_suffix("[/TAG]"));
    TEST_CHECK(strip_str.to_std_string() == "Content");

    mcsu8::u8str case_u("Hello World");
    TEST_CHECK(case_u.equals_ignore_case_ascii("hELLO wORLD"));
    TEST_CHECK(case_u.equals_ignore_case_ascii(mcsu8::u8str("hELLO wORLD")));

    TEST_CHECK(case_u.to_upper_copy().to_std_string() == "HELLO WORLD");
    TEST_CHECK(case_u.to_lower_copy().to_std_string() == "hello world");

    case_u.to_lower();
    TEST_CHECK(case_u.to_std_string() == "hello world");

    case_u.to_upper();
    TEST_CHECK(case_u.to_std_string() == "HELLO WORLD");

    // Operators
    mcsu8::u8str op1("안녕");
    mcsu8::u8str op2("하세요");
    mcsu8::u8str op3 = op1 + op2;
    TEST_CHECK(op3.to_std_string() == "안녕하세요");

    op3 += "!";
    TEST_CHECK(op3.to_std_string() == "안녕하세요!");
    TEST_CHECK(op3 != op1);
    TEST_CHECK(op3 == (op1 + op2 + "!"));
}
