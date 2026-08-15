#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

#include "mino/core/string/string.hpp"

// 단언 헬퍼 매크로 (함수 내부의 eprint, tce를 참조)
#define TEST_CHECK(expr) \
    do { \
        if (!(expr)) { \
            eprint(tce("[FAIL] Line "), __LINE__, tce(": " #expr)); \
            std::abort(); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    print(tce(">>> Running Test Suite: "), tce(name), tce("..."))

// =========================================================================
// 1. Test string_basic.hpp
// =========================================================================
void test_string_basic() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("string_basic");

    // Trim functions
    {
        std::string s1 = "  \t hello \r\n ";
        string::trim(s1); // 정리 (space, tab, newline 제거) 
        TEST_CHECK(s1 == "hello");

        std::string s2 = "   left";
        string::ltrim(s2); // 왼쪽 정리
        TEST_CHECK(s2 == "left");

        std::string s3 = "right   ";
        string::rtrim(s3); // 오른쪽 정리
        TEST_CHECK(s3 == "right");

        TEST_CHECK( string::ltrim_copy("  abc") == "abc"); // 왼쪽 공백 제거 후, 반환
        TEST_CHECK( string::rtrim_copy("abc  ") == "abc"); // 오른쪽 공백 제거 후, 반환
        TEST_CHECK( string::trim_copy("  abc  ") == "abc"); // 양쪽 공백 제거 후, 반환
    }

    // Replace functions
    {
        std::string s = "foo bar foo baz";
        TEST_CHECK(string::replace(s, "foo", "qux") == "qux bar qux baz"); // 문자열 내 "foo"를 "qux"로 교체
        TEST_CHECK(string::replace(s, "", "qux") == s); // 빈 문자열 교체는 무시

        std::string s_inplace = "a-b-a-b";
        string::replace_all_inplace(s_inplace, "a", "x"); // "a"를 "x"로 교체
        TEST_CHECK(s_inplace == "x-b-x-b");

        TEST_CHECK(string::replace_first("banana", "an", "XX") == "bXXana"); // 첫 번째 "an"만 교체
        TEST_CHECK(string::replace_last("banana", "an", "XX") == "banXXa"); // 마지막 "an"만 교체

        std::vector< std::pair<std::string, std::string> > map = {
            {"{name}", "Mino"},
            {"{action}", "Test"}
        };
        TEST_CHECK(string::replace_all_map("Hello {name}, do {action}!", map) == "Hello Mino, do Test!"); // 맵 기반 교체. {name}을 Mino로, {action}을 Test로 교체.

    }

    // Case, Contains, Starts/Ends With
    {
        TEST_CHECK(string::to_lower("HeLLo 123!") == "hello 123!");
        TEST_CHECK(string::to_upper("HeLLo 123!") == "HELLO 123!");

        TEST_CHECK(string::contains("Hello World", "World"));
        TEST_CHECK(string::contains("Hello World", ""));
        TEST_CHECK(!string::contains("Hello World", "xyz"));

        TEST_CHECK(string::starts_with("prefix_test", "prefix"));
        TEST_CHECK(!string::starts_with("prefix_test", "test"));
        TEST_CHECK(string::ends_with("prefix_test", "test"));
        TEST_CHECK(!string::ends_with("prefix_test", "prefix"));

        TEST_CHECK(string::iequals("Hello", "hELLO"));
        TEST_CHECK(!string::iequals("Hello", "World"));

        TEST_CHECK(string::icontains("Beautiful Day", "tIfUl"));
        TEST_CHECK(string::istarts_with("Hello World", "heLL"));
        TEST_CHECK(string::iends_with("Hello World", "rLD"));

        std::vector<std::string> prefixes = { "pre", "start", "init" };
        TEST_CHECK(string::starts_with_any("startup.cpp", prefixes));
        TEST_CHECK(!string::starts_with_any("main.cpp", prefixes));

        std::vector<std::string> suffixes = { ".cpp", ".hpp", ".h" };
        TEST_CHECK(string::ends_with_any("main.cpp", suffixes));
        TEST_CHECK(!string::ends_with_any("main.obj", suffixes));

        std::vector<std::string> items1 = { "interconnected", "intermediate", "intersection" };
        auto cprefix_i1 = string::common_prefix(items1);
        TEST_CHECK(cprefix_i1 == "inter");

        std::vector<std::string> items2 = { "running", "walking", "jumping" };
        TEST_CHECK(string::common_suffix(items2) == "ing");
    }

    // Split and Join
    {
        auto sp1 = string::split("apple,banana,orange", ',');
        TEST_CHECK(sp1.size() == 3 && sp1[0] == "apple" && sp1[1] == "banana" && sp1[2] == "orange");

        auto sp2 = string::split("a::b::::c", "::", true);
        TEST_CHECK(sp2.size() == 4 && sp2[0] == "a" && sp2[1] == "b" && sp2[2] == "" && sp2[3] == "c");

        auto tok1 = string::tokenize_any_of("one,two;three|four", ",;|", false);
        TEST_CHECK(tok1.size() == 4 && tok1[0] == "one" && tok1[3] == "four");

        auto sp_trim = string::split_trimmed(" a , b ,, c ", ',', true);
        TEST_CHECK(sp_trim.size() == 3 && sp_trim[0] == "a" && sp_trim[1] == "b" && sp_trim[2] == "c");

        auto lines = string::split_lines("line1\r\nline2\nline3\rline4", false);
        TEST_CHECK(lines.size() == 4 && lines[0] == "line1" && lines[3] == "line4");

        std::vector<std::string> joined_items = { "A", "B", "C" };
        TEST_CHECK(string::join(joined_items, " - ") == "A - B - C");
        TEST_CHECK(string::join({}, ",") == "");
    }

    // Whitespace / Newline Normalization
    {
        TEST_CHECK(string::is_blank("  \t\r\n "));
        TEST_CHECK(!string::is_blank("  a  "));

        TEST_CHECK(string::collapse_spaces("  hello   world  \t from  C++  ") == "hello world from C++");
        TEST_CHECK(string::normalize_newlines("line1\r\nline2\rline3\nline4", "\n") == "line1\nline2\nline3\nline4");
    }

    // Padding / Repeat / Quotes / Indent
    {
        TEST_CHECK(string::pad_left("42", 5, '0') == "00042");
        TEST_CHECK(string::pad_right("42", 5, ' ') == "42   ");
        TEST_CHECK(string::pad_center("test", 8, '-') == "--test--");

        TEST_CHECK(string::remove_chars("a-b-c-d", "-") == "abcd");
        TEST_CHECK(string::repeat("ab", 3) == "ababab");

        TEST_CHECK(string::strip_quotes("\"quoted\"", '"') == "quoted");
        TEST_CHECK(string::strip_quotes("no_quotes", '"') == "no_quotes");
        TEST_CHECK(string::quote("hello", '"', false) == "\"hello\"");
        TEST_CHECK(string::quote("he\"llo", '"', true) == "\"he\\\"llo\"");

        TEST_CHECK(string::surround_if_missing("foo", "[", "]") == "[foo]");
        TEST_CHECK(string::surround_if_missing("[foo]", "[", "]") == "[foo]");

        TEST_CHECK(string::indent_lines("a\nb\nc", "  ") == "  a\n  b\n  c");
    }

    // Prefix/Suffix removal
    {
        std::string pref = "PREFIX_content";
        TEST_CHECK(string::remove_prefix(pref, "PREFIX_"));
        TEST_CHECK(pref == "content");
        TEST_CHECK(!string::remove_prefix(pref, "NONEXIST"));

        std::string suff = "content_SUFFIX";
        TEST_CHECK(string::remove_suffix(suff, "_SUFFIX"));
        TEST_CHECK(suff == "content");

        TEST_CHECK(string::removed_prefix("lib_file.so", "lib_") == "file.so");
        TEST_CHECK(string::removed_suffix("file.tar.gz", ".gz") == "file.tar");
    }

    // Safe Substr & Ellipsize
    {
        TEST_CHECK(string::safe_substr("hello", 1, 100) == "ello");
        TEST_CHECK(string::safe_substr("hello", 10, 5) == "");

        TEST_CHECK(string::ellipsize("Very long text message", 10, "...") == "Very lo...");
        TEST_CHECK(string::ellipsize("Short", 10) == "Short");

        std::string utf8_kr = "안녕하세요 반갑습니다";
        auto ellipsized_utf8_kr = string::ellipsize_utf8_safe(utf8_kr, 5, "..");

        std::string cp949_out;
        if (!string::utf8_to_cp949(ellipsized_utf8_kr, cp949_out)) {
            assert(false);
        }
        TEST_CHECK(ellipsized_utf8_kr == "안녕하세요..");
    }

    // Parsing & Wildcard
    {
        TEST_CHECK(string::is_digits("1234567890"));
        TEST_CHECK(!string::is_digits("123a45"));
        TEST_CHECK(!string::is_digits(""));

        int64_t num64 = 0;
        TEST_CHECK(string::try_parse_int64("1234567890123", num64));
        TEST_CHECK(num64 == 1234567890123LL);
        TEST_CHECK(!string::try_parse_int64("123abc", num64));

        double dbl = 0.0;
        TEST_CHECK(string::try_parse_double("3.141592", dbl));
        TEST_CHECK(std::abs(dbl - 3.141592) < 1e-6);
        TEST_CHECK(!string::try_parse_double("invalid", dbl));

        TEST_CHECK(string::wildcard_match("test_file.cpp", "test_*.cpp"));
        TEST_CHECK(string::wildcard_match("sample_01.txt", "sample_??.txt"));
        TEST_CHECK(!string::wildcard_match("sample_001.txt", "sample_??.txt"));
    }

    // Korean numeric formatters
    {
        TEST_CHECK(string::add_separator("1234567890", 3, ',') == "1,234,567,890");
        TEST_CHECK(string::add_separator("-1234567.89", 3, ',') == "-1,234,567.89");
        TEST_CHECK(string::add_separator("12345678", 4, ',') == "1234,5678");

        TEST_CHECK(string::to_human_readable_korean("10000", false) == "1만");
        TEST_CHECK(string::to_human_readable_korean("123456789", true) == "1억 2,345만 6,789");
        TEST_CHECK(string::to_human_readable_korean("-50000000", false) == "-5000만");
    }
}

// =========================================================================
// 2. Test tokenizer.hpp
// =========================================================================
void test_tokenizer() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("tokenizer");

    auto toks = string::tokenize_string("alpha,beta\ngamma\rdelta", ",\n\r");
    TEST_CHECK(toks.size() == 4);
    TEST_CHECK(toks[0] == "alpha" && toks[1] == "beta" && toks[2] == "gamma" && toks[3] == "delta");

    auto empty_tok = string::tokenize_string("", ",");
    TEST_CHECK(empty_tok.size() == 1 && empty_tok[0].empty());
}

// =========================================================================
// 3. Test to_string.hpp
// =========================================================================
void test_to_string() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("to_string & is_equal");

    double val1 = 3.1415926535;
    double val2 = 3.1415926536;

    std::string s_fixed = string::to_string<double>(val1, 4);
    TEST_CHECK(s_fixed == "3.1416");

    std::string s_default = string::to_string<double>(val1);
    TEST_CHECK(!s_default.empty());

    TEST_CHECK(string::is_equal<double>(val1, val2, 4));
    TEST_CHECK(!string::is_equal<double>(val1, val2, 10));

    float f1 = 1.0f;
    float f2 = 1.0f;
    TEST_CHECK(string::is_equal<float>(f1, f2));
}

// =========================================================================
// 4. Test mutex_string.hpp
// =========================================================================
void test_mutex_string() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("mutex_string");

    // Constructors & Assignment
    string::mutex_string ms1;
    TEST_CHECK(ms1.empty() && ms1.size() == 0);

    string::mutex_string ms2("initial_str");
    TEST_CHECK(ms2 == "initial_str");
    TEST_CHECK(ms2.length() == 11);

    string::mutex_string ms3 = ms2;
    TEST_CHECK(ms3.str() == ms2.str());

    string::mutex_string ms4 = std::move(ms3);
    TEST_CHECK(ms4 == "initial_str");

    ms1 = "assigned";
    TEST_CHECK(ms1 == "assigned");
    ms1 = std::string("assigned_std");
    TEST_CHECK(ms1 == "assigned_std");

    // Comparisons
    TEST_CHECK(ms1.str() != ms2.str());
    TEST_CHECK(ms1 != "other");
    TEST_CHECK("assigned_std" == ms1);
    TEST_CHECK(std::string("assigned_std") == ms1);
    TEST_CHECK("other" != ms1);
    TEST_CHECK(std::string("other") != ms1);

    // Explicit std::string cast
    std::string snap = static_cast<std::string>(ms1);
    TEST_CHECK(snap == "assigned_std");
    TEST_CHECK(ms1.str() == "assigned_std");

    // Capacity & state
    ms1.reserve(64);
    TEST_CHECK(ms1.capacity() >= 64);
    TEST_CHECK(ms1.max_size() > 0);
    ms1.shrink_to_fit();

    // Element access and setters
    ms1 = "hello";
    TEST_CHECK(ms1.at(0) == 'h');
    TEST_CHECK(ms1[1] == 'e');
    TEST_CHECK(ms1.front() == 'h');
    TEST_CHECK(ms1.back() == 'o');

    ms1.set(0, 'H');
    ms1.front('J');
    ms1.back('!');
    TEST_CHECK(ms1 == "Jell!");

    // Modifiers
    ms1.clear();
    TEST_CHECK(ms1.empty());

    ms1.push_back('A');
    ms1.push_back('B');
    ms1.pop_back();
    TEST_CHECK(ms1 == "A");

    ms1.assign("Base");
    TEST_CHECK(ms1 == "Base");
    ms1.assign(std::string("NewBase"));
    TEST_CHECK(ms1 == "NewBase");
    ms1.assign(3, 'X');
    TEST_CHECK(ms1 == "XXX");

    ms1.append("123");
    ms1.append(std::string("456"));
    ms1.append(2, '7');
    TEST_CHECK(ms1 == "XXX12345677");

    ms1 += "_";
    ms1 += std::string("end");
    ms1 += '!';
    TEST_CHECK(ms1.str().find("end!") != std::string::npos);

    ms1 = "ABCDEF";
    ms1.insert(3, "_INS_");
    TEST_CHECK(ms1 == "ABC_INS_DEF");
    ms1.insert(0, std::string("["));
    ms1.insert(ms1.size(), 1, ']');
    TEST_CHECK(ms1 == "[ABC_INS_DEF]");

    ms1 = "0123456789";
    ms1.erase(3, 4);
    TEST_CHECK(ms1 == "012789");

    ms1.replace(1, 2, "XX");
    TEST_CHECK(ms1 == "0XX789");
    ms1.replace(0, 1, std::string("ZZ"));
    TEST_CHECK(ms1 == "ZZXX789");
    ms1.replace(0, 2, 3, 'Q');
    TEST_CHECK(ms1 == "QQQXX789");

    ms1.resize(5);
    TEST_CHECK(ms1.size() == 5);
    ms1.resize(8, '#');
    TEST_CHECK(ms1 == "QQQXX###");

    // String operations
    TEST_CHECK(ms1.substr(0, 3) == "QQQ");
    char c_buf[10] = { 0 };
    ms1.copy(c_buf, 3, 0);
    TEST_CHECK(std::string(c_buf) == "QQQ");

    TEST_CHECK(ms1.compare("QQQXX###") == 0);
    TEST_CHECK(ms1.compare(0, 3, "QQQ") == 0);

    ms1 = "banana split";
    TEST_CHECK(ms1.find("na") == 2);
    TEST_CHECK(ms1.find(std::string("na"), 3) == 4);
    TEST_CHECK(ms1.find('s') == 7);

    TEST_CHECK(ms1.rfind("na") == 4);
    TEST_CHECK(ms1.rfind(std::string("na")) == 4);
    TEST_CHECK(ms1.rfind('a') == 5);

    TEST_CHECK(ms1.find_first_of("aeiou") == 1);
    TEST_CHECK(ms1.find_first_of(std::string("xyzs")) == 7);
    TEST_CHECK(ms1.find_first_of('p') == 8);

    TEST_CHECK(ms1.find_last_of("aeiou") == 10);
    TEST_CHECK(ms1.find_last_of(std::string("aeiou")) == 10);
    TEST_CHECK(ms1.find_last_of('b') == 0);

    TEST_CHECK(ms1.find_first_not_of("abn ") == 7);
    TEST_CHECK(ms1.find_first_not_of(std::string("abn ")) == 7);
    TEST_CHECK(ms1.find_first_not_of('b') == 1);

    TEST_CHECK(ms1.find_last_not_of("it") == 9);
    TEST_CHECK(ms1.find_last_not_of(std::string("it")) == 9);
    TEST_CHECK(ms1.find_last_not_of('t') == 10);

    // Swap
    string::mutex_string sw1("AAA"), sw2("BBB");
    sw1.swap(sw2);
    TEST_CHECK(sw1.str() == "BBB" && sw2.str() == "AAA");
    string::swap(sw1, sw2);
    TEST_CHECK(sw1.str() == "AAA" && sw2.str() == "BBB");
    std::string str_std = "CCC";
    sw1.swap(str_std);
    TEST_CHECK(sw1.str() == "CCC" && str_std == "AAA");

    // with / with_lock / synchronize / guard
    ms1 = "thread_safe_data";
    ms1.with([](std::string& s) {
        s += "_modified";
        });
    TEST_CHECK(ms1 == "thread_safe_data_modified");

    const string::mutex_string& const_ms = ms1;
    bool checked = const_ms.with_lock([](const std::string& s) {
        return s.size() > 0;
        });
    TEST_CHECK(checked);

    {
        std::string expected = ms1.str(); // 가드 진입 전에 스냅샷을 뜨거나
        auto locked = ms1.synchronize();
        TEST_CHECK(locked.owns_lock());
        TEST_CHECK(locked->size() == expected.size());
        TEST_CHECK(*locked == expected);
        TEST_CHECK(locked->find("thread") != std::string::npos);
    }

    {
        const auto guard = const_ms.guard();
        TEST_CHECK(guard.owns_lock());
        TEST_CHECK(guard->find("thread") != std::string::npos);
    }
}

// =========================================================================
// 5. Test u8string.hpp & u8str
// =========================================================================
void test_u8str() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("u8string & u8str");

    // Global conversions
    std::string test_utf8 = "한글 및 English 123! 😀";
    auto u8_str_val = u8::to_u8_string(test_utf8);
    TEST_CHECK(u8::to_std_string(u8_str_val) == test_utf8);

    std::wstring ws = u8::to_wstring(u8_str_val);
    TEST_CHECK(u8::to_std_string(u8::to_u8_string(ws)) == test_utf8);

    std::u16string u16 = u8::to_u16string(u8_str_val);
    TEST_CHECK(u8::to_std_string(u8::to_u8_string(u16)) == test_utf8);

    std::u32string u32 = u8::to_u32string(u8_str_val);
    TEST_CHECK(u8::to_std_string(u8::to_u8_string(u32)) == test_utf8);

    // u8str class
    u8::u8str ustr1;
    TEST_CHECK(ustr1.empty() && ustr1.size() == 0);

    u8::u8str ustr2("Hello 유니코드");
    TEST_CHECK(!ustr2.empty());
    TEST_CHECK(ustr2.to_std_string() == "Hello 유니코드");

    u8::u8str ustr3(ustr2);
    TEST_CHECK(ustr3 == ustr2);

    u8::u8str ustr4(std::wstring(L"WideString 한글"));
    TEST_CHECK(!ustr4.empty());

    u8::u8str ustr5(u16);
    u8::u8str ustr6(u32);
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
    u8::u8str search_target("가나다라_ABCD_가나다라");
    TEST_CHECK(search_target.starts_with("가나다"));
    TEST_CHECK(search_target.starts_with(u8::u8str("가나다")));
    TEST_CHECK(search_target.ends_with("다라"));
    TEST_CHECK(search_target.ends_with(u8::u8str("다라")));
    TEST_CHECK(search_target.contains("ABCD"));
    TEST_CHECK(search_target.contains(u8::u8str("ABCD")));

    TEST_CHECK(search_target.index_of("ABCD") >= 0);
    TEST_CHECK(search_target.index_of(u8::u8str("ABCD")) >= 0);
    TEST_CHECK(search_target.last_index_of("가나다라") > 0);
    TEST_CHECK(search_target.last_index_of(u8::u8str("가나다라")) > 0);

    // Trim
    u8::u8str trim_u("   \t가나다   \n");
    trim_u.trim();
    TEST_CHECK(trim_u.to_std_string() == "가나다");

    // Split & Replace
    u8::u8str split_target("사과/오렌지/바나나/포도");
    auto split_res = split_target.split("/");
    TEST_CHECK(split_res.size() == 4 && split_res[0].to_std_string() == "사과");

    u8::u8str rep_target("foo bar foo baz");
    rep_target.replace("foo", "qux");
    TEST_CHECK(rep_target.to_std_string() == "qux bar foo baz");
    rep_target.replace_all(u8::u8str("foo"), u8::u8str("qux"));
    TEST_CHECK(rep_target.to_std_string() == "qux bar qux baz");

    // UTF-8 Codepoint based Left/Right/Mid/Reverse/Pad
    u8::u8str cp_str("안녕하세요반갑습니다"); // 10 code points
    TEST_CHECK(cp_str.left(2).to_std_string() == "안녕"); 
    TEST_CHECK(cp_str.right(4).to_std_string() == "갑습니다"); 
    TEST_CHECK(cp_str.mid(2, 3).to_std_string() == "하세요"); // [0]안 [1]녕 [2]하 [3]세 [4]요
    TEST_CHECK(cp_str.substr_utf8(2, 3).to_std_string() == "하세요");

    u8::u8str rev_str("123가나다");
    rev_str.reverse_utf8();
    TEST_CHECK(rev_str.to_std_string() == "다나가321");

    u8::u8str pad_str("한글");
    pad_str.pad_left(5, '-');
    TEST_CHECK(pad_str.to_std_string() == "---한글");
    pad_str.pad_right(7, '+');
    TEST_CHECK(pad_str.to_std_string() == "---한글++");

    // Strip prefix/suffix & Case
    u8::u8str strip_str("[TAG]Content[/TAG]");
    TEST_CHECK(strip_str.strip_prefix("[TAG]"));
    TEST_CHECK(strip_str.strip_suffix("[/TAG]"));
    TEST_CHECK(strip_str.to_std_string() == "Content");

    u8::u8str case_u("Hello World");
    TEST_CHECK(case_u.equals_ignore_case_ascii("hELLO wORLD"));
    TEST_CHECK(case_u.equals_ignore_case_ascii(u8::u8str("hELLO wORLD")));

    TEST_CHECK(case_u.to_upper_copy().to_std_string() == "HELLO WORLD");
    TEST_CHECK(case_u.to_lower_copy().to_std_string() == "hello world");
    case_u.to_lower();
    TEST_CHECK(case_u.to_std_string() == "hello world");
    case_u.to_upper();
    TEST_CHECK(case_u.to_std_string() == "HELLO WORLD");

    // Operators
    u8::u8str op1("안녕");
    u8::u8str op2("하세요");
    u8::u8str op3 = op1 + op2;
    TEST_CHECK(op3.to_std_string() == "안녕하세요");
    op3 += "!";
    TEST_CHECK(op3.to_std_string() == "안녕하세요!");
    TEST_CHECK(op3 != op1);
    TEST_CHECK(op3 == (op1 + op2 + "!"));
}

// =========================================================================
// 6. Test encoding_function.hpp & to_console_encoding.hpp
// =========================================================================
void test_encodings() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("encoding_function & console encoding");

    std::string utf8_sample = "한글 테스트 ABC 123";

    // UTF-8 <-> UTF-16
    std::u16string u16_out;
    TEST_CHECK(string::utf8_to_utf16(utf8_sample, u16_out));
    std::string utf8_back;
    TEST_CHECK(string::utf16_to_utf8(u16_out, utf8_back));
    TEST_CHECK(utf8_back == utf8_sample);

    // UTF-8 <-> wstring
    std::wstring w_out = string::utf8_to_utf16(utf8_sample);
    TEST_CHECK(!w_out.empty());
    TEST_CHECK(string::utf16_to_utf8(w_out) == utf8_sample);

    // UTF-8 <-> UTF-32
    std::u32string u32_out;
    TEST_CHECK(string::utf8_to_utf32(utf8_sample, u32_out));
    std::string utf8_from_u32;
    TEST_CHECK(string::utf32_to_utf8(u32_out, utf8_from_u32));
    TEST_CHECK(utf8_from_u32 == utf8_sample);

    // UTF-16 <-> UTF-32
    std::u32string u32_from_u16;
    TEST_CHECK(string::utf16_to_utf32(u16_out, u32_from_u16));
    std::u16string u16_from_u32;
    TEST_CHECK(string::utf32_to_utf16(u32_from_u16, u16_from_u32));
    TEST_CHECK(u16_from_u32 == u16_out);

    // CP949 / EUC-KR
    std::string cp949_out;
    if (string::utf8_to_cp949(utf8_sample, cp949_out)) {
        std::string utf8_from_cp949;
        TEST_CHECK(string::cp949_to_utf8(cp949_out, utf8_from_cp949));
        TEST_CHECK(utf8_from_cp949 == utf8_sample);

        std::string cp949_from_u16;
        TEST_CHECK(string::utf16_to_cp949(u16_out, cp949_from_u16));
        std::u16string u16_from_cp949;
        TEST_CHECK(string::cp949_to_utf16(cp949_from_u16, u16_from_cp949));
        TEST_CHECK(u16_from_cp949 == u16_out);
    }

    // ISO-2022-KR, JOHAB, MacKorean (환경에 따라 지원 여부 체크)
    std::string iso_out;
    if (string::utf8_to_iso2022kr(utf8_sample, iso_out)) {
        std::string utf8_from_iso;
        TEST_CHECK(string::iso2022kr_to_utf8(iso_out, utf8_from_iso));
        TEST_CHECK(!utf8_from_iso.empty());
    }

    std::string johab_out;
    if (string::utf8_to_johab(utf8_sample, johab_out)) {
        std::string utf8_from_johab;
        TEST_CHECK(string::johab_to_utf8(johab_out, utf8_from_johab));
        TEST_CHECK(!utf8_from_johab.empty());
    }

    std::string mac_out;
    if (string::utf8_to_mackorean(utf8_sample, mac_out)) {
        std::string utf8_from_mac;
        TEST_CHECK(string::mackorean_to_utf8(mac_out, utf8_from_mac));
        TEST_CHECK(!utf8_from_mac.empty());
    }

    // *_or_throw helpers
    TEST_CHECK(string::utf8_to_utf16_or_throw(utf8_sample) == u16_out);
    TEST_CHECK(string::utf16_to_utf8_or_throw(u16_out) == utf8_sample);
    TEST_CHECK(string::utf8_to_utf32_or_throw(utf8_sample) == u32_out);
    TEST_CHECK(string::utf32_to_utf8_or_throw(u32_out) == utf8_sample);
    TEST_CHECK(string::utf16_to_utf32_or_throw(u16_out) == u32_out);
    TEST_CHECK(string::utf32_to_utf16_or_throw(u32_out) == u16_out);

    // Console encoding
    std::string console_bytes = string::to_console_encoding(utf8_sample);
    TEST_CHECK(!console_bytes.empty());
    std::string from_console = string::from_console_encoding(console_bytes);
    TEST_CHECK(!from_console.empty());
}

// =========================================================================
// Main Entry
// =========================================================================
int main() {
    namespace string = mino::core::string;
    namespace u8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    print(tce("========================================"));
    print(tce("Starting mino::core::string Test Suite"));
    print(tce("========================================"));

    try {
        test_string_basic();
        test_tokenizer();
        test_to_string();
        test_mutex_string();
        test_u8str();
        test_encodings();

        print(endl, tce("========================================"));
        print(tce("[SUCCESS] All tests passed successfully!"));
        print(tce("========================================"));
    }
    catch (const std::exception& ex) {
        eprint(endl, tce("[EXCEPTION ERROR] "), ex.what());
        return 1;
    }

    return 0;
}
