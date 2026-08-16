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
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("string_basic");

    // Trim functions
    {
        std::string s1 = "  \t hello \r\n ";
        mcs::trim(s1); // 정리 (space, tab, newline 제거) 
        TEST_CHECK(s1 == "hello");

        std::string s2 = "   left";
        mcs::ltrim(s2); // 왼쪽 정리
        TEST_CHECK(s2 == "left");

        std::string s3 = "right   ";
        mcs::rtrim(s3); // 오른쪽 정리
        TEST_CHECK(s3 == "right");

        TEST_CHECK(mcs::ltrim_copy("  abc") == "abc"); // 왼쪽 공백 제거 후, 반환
        TEST_CHECK(mcs::rtrim_copy("abc  ") == "abc"); // 오른쪽 공백 제거 후, 반환
        TEST_CHECK(mcs::trim_copy("  abc  ") == "abc"); // 양쪽 공백 제거 후, 반환
    }

    // Replace functions
    {
        std::string s = "foo bar foo baz";
        TEST_CHECK(mcs::replace(s, "foo", "qux") == "qux bar qux baz"); // 문자열 내 "foo"를 "qux"로 교체
        TEST_CHECK(mcs::replace(s, "", "qux") == s); // 빈 문자열 교체는 무시

        std::string s_inplace = "a-b-a-b";
        mcs::replace_all_inplace(s_inplace, "a", "x"); // "a"를 "x"로 교체
        TEST_CHECK(s_inplace == "x-b-x-b");

        TEST_CHECK(mcs::replace_first("banana", "an", "XX") == "bXXana"); // 첫 번째 "an"만 교체
        TEST_CHECK(mcs::replace_last("banana", "an", "XX") == "banXXa"); // 마지막 "an"만 교체
        std::vector< std::pair<std::string, std::string> > map = {
            {"{name}", "Mino"},
            {"{action}", "Test"}
        };
        TEST_CHECK(mcs::replace_all_map("Hello {name}, do {action}!", map) == "Hello Mino, do Test!"); // 맵 기반 교체. {name}을 Mino로, {action}을 Test로 교체.

    }

    // Case, Contains, Starts/Ends With
    {
        TEST_CHECK(mcs::to_lower("HeLLo 123!") == "hello 123!"); // 소문자로 변환.   
        TEST_CHECK(mcs::to_upper("HeLLo 123!") == "HELLO 123!"); // 대문자로 변환.

        TEST_CHECK(mcs::contains("Hello World", "World")); // "Hello World"에 "World"가 포함되어 있는지 확인.
        TEST_CHECK(mcs::contains("Hello World", "")); // 빈 문자열은 항상 포함되어 있다고 간주.
        TEST_CHECK(!mcs::contains("Hello World", "xyz")); // "Hello World"에 "xyz"가 포함되어 있지 않음.

        TEST_CHECK(mcs::starts_with("prefix_test", "prefix")); // "prefix_test"가 "prefix"로 시작하는지 확인.
        TEST_CHECK(!mcs::starts_with("prefix_test", "test")); // "prefix_test"는 "test"로 시작하지 않음.
        TEST_CHECK(mcs::ends_with("prefix_test", "test")); // "prefix_test"가 "test"로 끝나는지 확인.
        TEST_CHECK(!mcs::ends_with("prefix_test", "prefix")); // "prefix_test"는 "prefix"로 끝나지 않음.

        TEST_CHECK(mcs::iequals("Hello", "hELLO")); // 대소문자 구분 없이 문자열 비교.
        TEST_CHECK(!mcs::iequals("Hello", "World")); // 대소문자 구분 없이 문자열 비교, "Hello"와 "World"는 같지 않음.

        TEST_CHECK(mcs::icontains("Beautiful Day", "tIfUl")); // 대소문자 구분 없이 문자열 포함 여부 확인.
        TEST_CHECK(mcs::istarts_with("Hello World", "heLL")); // 대소문자 구분 없이 문자열 시작 여부 확인.
        TEST_CHECK(mcs::iends_with("Hello World", "rLD")); // 대소문자 구분 없이 문자열 끝 여부 확인.

        std::vector<std::string> prefixes = { "pre", "start", "init" };
        TEST_CHECK(mcs::starts_with_any("startup.cpp", prefixes)); // "startup.cpp"는 {"pre", "start", "init"} 중 하나로 시작됨. ("start"로 시작)
        TEST_CHECK(!mcs::starts_with_any("main.cpp", prefixes)); // "main.cpp"는 {"pre", "start", "init"} 중 하나로 시작하지 않음.

        std::vector<std::string> suffixes = { ".cpp", ".hpp", ".h" };
        TEST_CHECK(mcs::ends_with_any("main.cpp", suffixes)); // "main.cpp"는 {"cpp", "hpp", "h"} 중 하나로 끝남. (".cpp"로 끝남)
        TEST_CHECK(!mcs::ends_with_any("main.obj", suffixes)); // "main.obj"는 {"cpp", "hpp", "h"} 중 하나로 끝나지 않음.

        std::vector<std::string> items1 = { "interconnected", "intermediate", "intersection" };
        auto cprefix_i1 = mcs::common_prefix(items1); // items1의 공통 접두어 찾기. (모두 "inter"로 시작됨.)    
        TEST_CHECK(cprefix_i1 == "inter");

        std::vector<std::string> items2 = { "running", "walking", "jumping" };
        TEST_CHECK(mcs::common_suffix(items2) == "ing"); // items2의 공통 접미어 찾기. (모두 "ing"로 끝남.)
    }

    // Split and Join
    {
        auto sp1 = mcs::split("apple,banana,orange", ','); // 쉼표를 기준으로 문자열을 분리.
        TEST_CHECK(sp1.size() == 3 && // 분리된 3개의 문자열이 있는지 확인.
            sp1[0] == "apple" && // 첫 번째 문자열이 "apple"인지 확인.
            sp1[1] == "banana" && // 두 번째 문자열이 "banana"인지 확인.
            sp1[2] == "orange"); // 세 번째 문자열이 "orange"인지 확인.

        bool keep_empty_sp2 = true;
        auto sp2 = mcs::split("a::b::::c", "::", keep_empty_sp2); // "::"를 기준으로 문자열을 분리하고, 빈 문자열도 포함(keep_empty=true).
        TEST_CHECK(sp2.size() == 4 && // 분리된 4개의 문자열이 있는지 확인.
            sp2[0] == "a" && // 첫 번째 문자열이 "a"인지 확인.
            sp2[1] == "b" && // 두 번째 문자열이 "b"인지 확인.
            sp2[2] == "" && // 세 번째 문자열이 빈 문자열인지 확인.
            sp2[3] == "c"); // 네 번째 문자열이 "c"인지 확인.

        bool keep_empty_tok1 = false;
        auto tok1 = mcs::tokenize_any_of("one,two;three|four", ",;|", keep_empty_tok1); // ",;|" 중 하나를 기준으로 문자열을 분리하고, 빈 문자열은 포함하지 않음(keep_empty=false).
        TEST_CHECK(tok1.size() == 4 && // 분리된 4개의 문자열이 있는지 확인.
            tok1[0] == "one" && // 첫 번째 문자열이 "one"인지 확인.
            tok1[1] == "two" && // 두 번째 문자열이 "two"인지 확인.
            tok1[2] == "three" && // 세 번째 문자열이 "three"인지 확인.
            tok1[3] == "four"); // 네 번째 문자열이 "four"인지 확인.

        bool drop_empty_sp_trim = true;
        auto sp_trim = mcs::split_trimmed(" a , b ,, c ", ',', drop_empty_sp_trim); // 쉼표(,)를 기준으로 문자열을 분리하고, 각 요소의 양쪽 공백을 제거하며, 빈 문자열은 포함하지 않음(drop_empty=true).
        TEST_CHECK(sp_trim.size() == 3 && // 분리된 3개의 문자열이 있는지 확인.
            sp_trim[0] == "a" && // 첫 번째 문자열이 "a"인지 확인.
            sp_trim[1] == "b" && // 두 번째 문자열이 "b"인지 확인.
            sp_trim[2] == "c"); // 세 번째 문자열이 "c"인지 확인.

        bool keep_empty_lines = false;
        auto lines = mcs::split_lines("line1\r\nline2\nline3\rline4", keep_empty_lines); // 문자열을 줄(\n \r\n \r) 단위로 분리하고, 빈 줄은 포함하지 않음(keep_empty=false).
        TEST_CHECK(lines.size() == 4 && // 분리된 4개의 줄이 있는지 확인.
            lines[0] == "line1" && // 첫 번째 줄이 "line1"인지 확인.
            lines[1] == "line2" && // 두 번째 줄이 "line2"인지 확인.
            lines[2] == "line3" && // 세 번째 줄이 "line3"인지 확인.
            lines[3] == "line4"); // 네 번째 줄이 "line4"인지 확인.

        std::vector<std::string> joined_items = { "A", "B", "C" };
        TEST_CHECK(mcs::join(joined_items, " - ") == "A - B - C"); // " - "를 구분자로 하여 문자열들을 결합. 결과는 "A - B - C"이어야 함.
        TEST_CHECK(mcs::join({}, ",") == ""); // 빈 벡터({})를 결합하면 빈 문자열("")이 반환됨.
    }

    // Whitespace / Newline Normalization
    {
        TEST_CHECK(mcs::is_blank("  \t\r\n ")); // 공백, 탭, 줄바꿈만 있는 문자열은 blank로 간주.
        TEST_CHECK(!mcs::is_blank("  a  ")); // 공백 외에 다른 문자가 있는 문자열은 blank가 아님.

        TEST_CHECK(mcs::collapse_spaces("  hello   world  \t from  C++  ") == "hello world from C++"); // 연속된 공백을 하나의 공백으로 축소하고, 양쪽 공백 제거.
        TEST_CHECK(mcs::normalize_newlines("line1\r\nline2\rline3\nline4", "\n") == "line1\nline2\nline3\nline4"); // 다양한 줄바꿈(\r\n, \r, \n)을 모두 "\n"으로 통일.
    }

    // Padding / Repeat / Quotes / Indent
    {
        TEST_CHECK(mcs::pad_left("42", 5, '0') == "00042"); // 문자열 "42"를 왼쪽으로 패딩하여 총 길이가 5가 되도록 하고, 빈 공간은 '0'으로 채움.
        TEST_CHECK(mcs::pad_right("42", 5, ' ') == "42   "); // 문자열 "42"를 오른쪽으로 패딩하여 총 길이가 5가 되도록 하고, 빈 공간은 공백(' ')으로 채움.
        TEST_CHECK(mcs::pad_center("test", 8, '-') == "--test--"); // 문자열 "test"를 가운데로 패딩하여 총 길이가 8이 되도록 하고, 빈 공간은 '-'으로 채움.

        TEST_CHECK(mcs::remove_chars("a-b-c-d", "-") == "abcd"); // 문자열 "a-b-c-d"에서 "-" 문자를 제거하여 "abcd"를 반환.
        TEST_CHECK(mcs::repeat("ab", 3) == "ababab"); // 문자열 "ab"를 3번 반복하여 "ababab"를 반환.

        TEST_CHECK(mcs::strip_quotes("\"quoted\"", '"') == "quoted"); // 문자열 "\"quoted\""에서 양쪽의 '"'를 제거하여 "quoted"를 반환.
        TEST_CHECK(mcs::strip_quotes("no_quotes", '"') == "no_quotes"); // 문자열 "no_quotes"에는 양쪽에 '"'가 없으므로 그대로 반환.

        bool escape_1 = false;
        TEST_CHECK(mcs::quote("hello", '"', escape_1) == "\"hello\"");
        // 문자열 hello 를 \" 로 감싸서 \"hello\" 를 반환.

        bool escape_2 = true;
        TEST_CHECK(mcs::quote("he\"llo", '"', escape_2) == "\"he\\\"llo\"");
        // 문자열 he\"llo 를 \" 로 감싸고 ===> \"he\"llo\" 
        // 내부의 \" 를 이스케이프 처리(escape=true)하여 \"he\\\"llo\" 를 반환. print하면 "he\"llo" 로 출력됨.

        TEST_CHECK(mcs::surround_if_missing("foo", "[", "]") == "[foo]"); // 문자열 "foo"를 "["와 "]"로 감싸되, 이미 감싸져 있으면 그대로 반환. 결과는 "[foo]".
        TEST_CHECK(mcs::surround_if_missing("[foo]", "[", "]") == "[foo]"); // 이미 "["와 "]"로 감싸져 있으므로 그대로 "[foo]"를 반환.

        TEST_CHECK(mcs::indent_lines("a\nb\nc", "  ") == "  a\n  b\n  c"); // 각 줄 앞에 "  "를 추가하여 들여쓰기. 결과는 "  a\n  b\n  c".
    }

    // Prefix/Suffix removal
    {
        std::string pref = "PREFIX_content";
        TEST_CHECK(mcs::remove_prefix(pref, "PREFIX_")); // "PREFIX_content"에서 접두어 "PREFIX_"를 제거하여 "content"로 변경.
        TEST_CHECK(pref == "content"); // pref가 "content"로 변경되었는지 확인.
        TEST_CHECK(!mcs::remove_prefix(pref, "NONEXIST")); // "content"에서 "NONEXIST"는 없으므로 제거되지 않음. false 반환.

        std::string suff = "content_SUFFIX";
        TEST_CHECK(mcs::remove_suffix(suff, "_SUFFIX")); // "content_SUFFIX"에서 접미어 "_SUFFIX"를 제거하여 "content"로 변경.
        TEST_CHECK(suff == "content"); // suff가 "content"로 변경되었는지 확인.

        TEST_CHECK(mcs::removed_prefix("lib_file.so", "lib_") == "file.so"); // "lib_file.so"에서 접두어 "lib_"를 제거한 새로운 문자열 "file.so"를 반환.
        TEST_CHECK(mcs::removed_suffix("file.tar.gz", ".gz") == "file.tar"); // "file.tar.gz"에서 접미어 ".gz"를 제거한 새로운 문자열 "file.tar"를 반환.
    }

    // Safe Substr & Ellipsize
    {
        // hello
        // 01234
        TEST_CHECK(mcs::safe_substr("hello", 1, 100) == "ello"); // "hello"에서 인덱스 1부터 100까지 안전하게 추출하여 "ello"를 반환.
        TEST_CHECK(mcs::safe_substr("hello", 10, 5) == ""); // "hello"에서 인덱스 10부터 5글자를 안전하게 추출하려고 시도하지만 범위(0~4)를 벗어나므로 빈 문자열을 반환.

        TEST_CHECK(mcs::ellipsize("Very long text message", 10, "...") == "Very lo..."); // "Very long text message"를 최대 길이 10으로 줄이고, 초과된 부분은 "..."로 대체하여 "Very lo..."를 반환.
        TEST_CHECK(mcs::ellipsize("Short", 10) == "Short"); //  "Short"는 최대 길이 10보다 짧으므로 그대로 "Short"를 반환.

        std::string utf8_kr = "안녕하세요 반갑습니다";
        auto ellipsized_utf8_kr = mcs::ellipsize_utf8_safe(utf8_kr, 5, ".."); // "안녕하세요 반갑습니다"를 최대 5개의 유니코드(UTF-8) 코드포인트로 줄이고, 초과된 부분은 ".."로 대체하여 "안녕하세요.."를 반환.
 #ifdef _WIN32
        std::string cp949_out; // 디버깅 확인 용도 (윈도우에서는 cp949 문자열로 디버깅)
        if (!mcs::utf8_to_cp949(ellipsized_utf8_kr, cp949_out)) { // UTF-8 문자열을 CP949로 변환하여 cp949_out에 변환. 실패 시 assert.
            assert(false);
        }
#endif
        TEST_CHECK(ellipsized_utf8_kr == "안녕하세요..");
    }

    // Parsing & Wildcard
    {
        TEST_CHECK(mcs::is_digits("1234567890")); // 모두 숫자로 이루어져 있는지 확인. (모두 숫자이므로 true 반환)
        TEST_CHECK(!mcs::is_digits("123a45")); // "123a45"에는 숫자가 아닌 'a'가 포함되어 있으므로 false 반환.
        TEST_CHECK(!mcs::is_digits("")); // 빈 문자열은 숫자가 아니므로 false 반환.

        int64_t num64 = 0;
        TEST_CHECK(mcs::try_parse_int64("1234567890123", num64)); // "1234567890123"을 int64_t로 변환하여 num64에 저장. 성공 시 true 반환.
        TEST_CHECK(num64 == 1234567890123LL); // num64가 1234567890123인지 확인.
        TEST_CHECK(!mcs::try_parse_int64("123abc", num64)); // "123abc"은 숫자가 아니므로 변환 실패. false 반환.

        double dbl = 0.0;
        TEST_CHECK(mcs::try_parse_double("3.141592", dbl)); // "3.141592"를 double로 변환하여 dbl에 저장. 성공 시 true 반환.
        TEST_CHECK(std::abs(dbl - 3.141592) < 1e-6); // dbl이 3.141592와 거의 같은지 확인.
        TEST_CHECK(!mcs::try_parse_double("invalid", dbl)); // "invalid"는 숫자가 아니므로 변환 실패. false 반환.

        // wildcard matching. 와일드카드로 사용가능한 문자: '*' (크기 제한 없는 문자열), '?' (1개의 문자)
        TEST_CHECK(mcs::wildcard_match("test_file.cpp", "test_*.cpp")); // "test_file.cpp"가 "test_*.cpp" 패턴과 일치하는지 확인. (일치하므로 true 반환)
        TEST_CHECK(mcs::wildcard_match("sample_01.txt", "sample_??.txt")); // "sample_01.txt"가 "sample_??.txt" 패턴과 일치하는지 확인. (일치하므로 true 반환) ??는 2개의 문자를 의미.
        TEST_CHECK(!mcs::wildcard_match("sample_001.txt", "sample_??.txt")); // "sample_001.txt"는 "sample_??.txt" 패턴과 일치하지 않으므로 false 반환.
    }

    // Korean numeric formatters
    {
        // 일반적으로 한중일 숫자 단위 변환 및 4자리마다 구분자 추가.
        // 유럽,미국식 숫자 단위 변환 및 3자리마다 구분자 추가.

        auto str1 = mcs::to_string<int64_t>(1234567890LL); // int64_t 숫자를 문자열로 변환하여 num_str에 저장.
        auto sep1 = mcs::add_separator(str1, 3, ',');
        TEST_CHECK(sep1 == "1,234,567,890"); // "1234567890"을 3자리마다 ','로 구분하여 "1,234,567,890"을 반환.

        auto str2 = mcs::to_string<double>(-1234567.89, 2); // 소수점 2자리 정밀도
        auto sep2 = mcs::add_separator(str2, 3, ',');
        TEST_CHECK(sep2 == "-1,234,567.89"); // "-1234567.89"을 3자리마다 ','로 구분하여 "-1,234,567.89"을 반환.

        auto str3 = mcs::to_string<int>(12345678);
        auto sep3 = mcs::add_separator(str3, 4, ',');
        TEST_CHECK(sep3 == "1234,5678"); // "12345678"을 4자리마다 ','로 구분하여 "1234,5678"을 반환.

        bool include_comma_1 = false; // 숫자 크기 구분 시, 콤마(,)를 사용하지 않음.
        TEST_CHECK(mcs::to_human_readable_korean("10000", include_comma_1) == "1만"); // "10000"을 한국식 숫자 단위로 변환하여 "1만"을 반환. (UTF-8 문자열)

        bool include_comma_2 = true; // 숫자 크기 구분 시, 콤마(,)를 사용함.
        TEST_CHECK(mcs::to_human_readable_korean("123456789", include_comma_2) == "1억 2,345만 6,789"); // "123456789"을 한국식 숫자 단위로 변환하여 "1억 2,345만 6,789"을 반환.

        bool include_comma_3 = false;
        TEST_CHECK(mcs::to_human_readable_korean("-50000000", include_comma_3) == "-5000만");
    }
}

// =========================================================================
// 2. Test tokenizer.hpp
// =========================================================================
void test_tokenizer() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("tokenizer");

    auto toks = mcs::tokenize_string("alpha,beta\ngamma\rdelta", ",\n\r"); // 문자열 "alpha,beta\ngamma\rdelta"를 ",", "\n", "\r" 중 하나를 기준으로 분리하여 토큰화.
    TEST_CHECK(toks.size() == 4); // 분리된 토큰의 개수가 4개인지 확인.
    TEST_CHECK(toks[0] == "alpha" && // 첫 번째 토큰이 "alpha"인지 확인.
        toks[1] == "beta" && // 두 번째 토큰이 "beta"인지 확인.
        toks[2] == "gamma" && // 세 번째 토큰이 "gamma"인지 확인.
        toks[3] == "delta"); // 네 번째 토큰이 "delta"인지 확인.

    auto empty_tok = mcs::tokenize_string("", ","); // 빈 문자열을 토큰화하면, 하나의 빈 토큰이 생성되어야 함.
    TEST_CHECK(empty_tok.size() == 1 && // 분리된 토큰의 개수가 1개인지 확인.
        empty_tok[0].empty()); // 첫 번째 토큰이 빈 문자열인지 확인.
}

// =========================================================================
// 3. Test to_string.hpp
// =========================================================================
void test_to_string() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("to_string & is_equal");

    double val1 = 3.1415926535;
    double val2 = 3.1415926536;

    std::string s_fixed = mcs::to_string<double>(val1, 4); // 소수점 4자리까지 고정하여 문자열로 변환. 결과는 "3.1416"이어야 함.
    TEST_CHECK(s_fixed == "3.1416");

    std::string s_default = mcs::to_string<double>(val1); // 기본 정밀도로 문자열로 변환. 결과는 3.14159265350000005 
    TEST_CHECK(!s_default.empty());

    TEST_CHECK(mcs::is_equal<double>(val1, val2, 2)); // 소수점 2자리까지 비교하면 두 값이 같으므로 true 반환.
    TEST_CHECK(!mcs::is_equal<double>(val1, val2, 10)); // 소수점 10자리까지 비교하면 두 값이 다르므로 false 반환.

    float f1 = 1.0f;
    float f2 = 1.0f;
    TEST_CHECK(mcs::is_equal<float>(f1, f2)); // 두 float 값이 같으므로 true 반환.
}

// =========================================================================
// 4. Test mutex_string.hpp
// =========================================================================
void test_mutex_string() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("mutex_string");

    // Constructors & Assignment
    mcs::mutex_string ms1;
    TEST_CHECK(ms1.empty() && ms1.size() == 0);

    mcs::mutex_string ms2("initial_str");
    TEST_CHECK(ms2 == "initial_str");
    TEST_CHECK(ms2.length() == 11);

    mcs::mutex_string ms3 = ms2;
    TEST_CHECK(ms3.str() == ms2.str());

    mcs::mutex_string ms4 = std::move(ms3); // Move constructor. m3는 비워지고, m4는 m3의 내용을 가져옴.
    TEST_CHECK(ms4 == "initial_str" && ms3.empty());

    ms1 = "assigned";
    TEST_CHECK(ms1 == "assigned");

    ms1 = std::string("assigned_std"); // Assign from std::string
    TEST_CHECK(ms1 == "assigned_std");

    // Comparisons
    TEST_CHECK(ms1.str() != ms2.str());
    TEST_CHECK(ms1 != "other");
    TEST_CHECK("assigned_std" == ms1);
    TEST_CHECK(std::string("assigned_std") == ms1);
    TEST_CHECK("other" != ms1);
    TEST_CHECK(std::string("other") != ms1);

    // Explicit std::string cast
    std::string snap = static_cast<std::string>(ms1); // Explicitly cast mutex_string to std::string
    TEST_CHECK(snap == "assigned_std");
    TEST_CHECK(ms1.str() == "assigned_std");

    // Capacity & state
    ms1.reserve(64); // 64 글자로 크기 예약. (크기는 영문 기준으로 64글자 이상 확보)
    TEST_CHECK(ms1.capacity() >= 64);
    TEST_CHECK(ms1.max_size() > 0);
    ms1.shrink_to_fit();

    // Element access and setters
    ms1 = "hello";
    TEST_CHECK(ms1.at(0) == 'h'); // at() 메서드로 첫 번째 글자 접근
    TEST_CHECK(ms1[1] == 'e'); // operator[]로 두 번째 글자 접근
    TEST_CHECK(ms1.front() == 'h'); // 첫 번째 글자 접근
    TEST_CHECK(ms1.back() == 'o'); // 마지막 글자 접근

    ms1.set(0, 'H'); // 첫 번째 글자를 'H'로 변경
    ms1.front('J'); // 첫 번째 글자를 'J'로 변경
    ms1.back('!'); // 마지막 글자를 '!'로 변경
    TEST_CHECK(ms1 == "Jell!");

    // Modifiers
    ms1.clear();
    TEST_CHECK(ms1.empty());

    ms1.push_back('A'); // 'A'
    ms1.push_back('B'); // 'AB'
    ms1.pop_back(); // 'A'
    TEST_CHECK(ms1 == "A");

    ms1.assign("Base"); // C-style 문자열로 할당
    TEST_CHECK(ms1 == "Base");

    ms1.assign(std::string("NewBase")); // std::string으로 할당
    TEST_CHECK(ms1 == "NewBase");

    ms1.assign(3, 'X'); // 'X'를 3번 반복하여 "XXX"로 설정
    TEST_CHECK(ms1 == "XXX");

    ms1.append("123"); // "XXX" + "123" => "XXX123"
    ms1.append(std::string("456")); // "XXX123" + "456" => "XXX123456"
    ms1.append(2, '7'); // "XXX123456" + "77" => "XXX12345677"
    TEST_CHECK(ms1 == "XXX12345677");

    ms1 += "_"; // "XXX12345677" + "_" => "XXX12345677_"
    ms1 += std::string("end"); // "XXX12345677_" + "end" => "XXX12345677_end"
    ms1 += '!'; // "XXX12345677_end" + "!" => "XXX12345677_end!"
    TEST_CHECK(ms1.str().find("end!") != std::string::npos); // "end!"가 포함되어 있는지 확인

    ms1 = "ABCDEF";
    ms1.insert(3, "_INS_"); // "ABC" + "_INS_" + "DEF" => "ABC_INS_DEF"
    TEST_CHECK(ms1 == "ABC_INS_DEF");

    ms1.insert(0, std::string("[")); // "[" + "ABC_INS_DEF" => "[ABC_INS_DEF"
    ms1.insert(ms1.size(), 1, ']'); // "[ABC_INS_DEF" + "]" => "[ABC_INS_DEF]"
    TEST_CHECK(ms1 == "[ABC_INS_DEF]");

    ms1 = "0123456789";
    ms1.erase(3, 4); // 3번째 인덱스부터 4글자 제거 => "012____789" => "012789"
    TEST_CHECK(ms1 == "012789");

    ms1.replace(1, 2, "XX"); // 1번째 인덱스부터 2글자를 "XX"로 교체 => "0XX789"
    TEST_CHECK(ms1 == "0XX789");

    ms1.replace(0, 1, std::string("ZZ")); // 0번째 인덱스부터 1글자를 "ZZ"로 교체 => "ZZXX789"
    TEST_CHECK(ms1 == "ZZXX789");

    ms1.replace(0, 2, 3, 'Q'); // 0번째 인덱스부터 2글자를 'Q'를 3번 반복하여 교체 => "QQQXX789"
    TEST_CHECK(ms1 == "QQQXX789");

    ms1.resize(5); // "QQQXX789"을 5글자로 줄이기 => "QQQXX"
    TEST_CHECK(ms1.size() == 5);

    ms1.resize(8, '#'); // "QQQXX"을 8글자로 늘리고, 부족한 부분은 '#'로 채우기 => "QQQXX###"
    TEST_CHECK(ms1 == "QQQXX###");

    // String operations
    TEST_CHECK(ms1.substr(0, 3) == "QQQ"); // 0번째 인덱스부터 3글자를 추출하여 "QQQ" 반환

    char c_buf[10] = { 0 };
    ms1.copy(c_buf, 3, 0); // 0번째 인덱스부터 3글자를 c_buf에 복사 => c_buf = "QQQ"
    TEST_CHECK(std::string(c_buf) == "QQQ");

    // 현재 ms1은 QQQXX###
    TEST_CHECK(ms1.compare("QQQXX###") == 0); // "QQQXX###"와 비교하여 같으면 0 반환
    TEST_CHECK(ms1.compare(0, 3, "QQQ") == 0); // 0번째 인덱스부터 3글자를 "QQQ"와 비교하여 같으면 0 반환

    ms1 = "banana split";
    TEST_CHECK(ms1.find("na") == 2); // "banana split"에서 "na"의 첫 번째 위치는 2
    TEST_CHECK(ms1.find(std::string("na"), 3) == 4); // 3번째 인덱스 이후에서 "na"의 첫 번째 위치는 4
    TEST_CHECK(ms1.find('s') == 7); // "banana split"에서 's'의 위치는 7

    TEST_CHECK(ms1.rfind("na") == 4); // "banana split"에서 "na"의 마지막(역방향) 위치는 4
    TEST_CHECK(ms1.rfind(std::string("na")) == 4); // "banana split"에서 "na"의 마지막(역방향) 위치는 4
    TEST_CHECK(ms1.rfind('a') == 5); // "banana split"에서 'a'의 마지막(역방향) 위치는 5

    TEST_CHECK(ms1.find_first_of("aeiou") == 1); // "banana split"에서 모음 중 첫 번째 위치는 1
    TEST_CHECK(ms1.find_first_of(std::string("xyzs")) == 7); // "banana split"에서 "xyzs" 중 첫 번째 위치는 7
    TEST_CHECK(ms1.find_first_of('p') == 8); // "banana split"에서 'p'의 첫 번째 위치는 8

    TEST_CHECK(ms1.find_last_of("aeiou") == 10); // "banana split"에서 모음 중 마지막 위치는 10
    TEST_CHECK(ms1.find_last_of(std::string("aeiou")) == 10); // "banana split"에서 모음 중 마지막 위치는 10
    TEST_CHECK(ms1.find_last_of('b') == 0); // "banana split"에서 'b'의 마지막 위치는 0

    TEST_CHECK(ms1.find_first_not_of("abn ") == 7); // "banana split"에서 "abn "에 속하지 않는 첫 번째 위치는 7 ('s')
    TEST_CHECK(ms1.find_first_not_of(std::string("abn ")) == 7); // "banana split"에서 "abn "에 속하지 않는 첫 번째 위치는 7 ('s')
    TEST_CHECK(ms1.find_first_not_of('b') == 1); // "banana split"에서 'b'에 속하지 않는 첫 번째 위치는 1 ('a')

    TEST_CHECK(ms1.find_last_not_of("it") == 9); // "banana split"에서 "it"에 속하지 않는 마지막 위치는 9 ('l')
    TEST_CHECK(ms1.find_last_not_of(std::string("it")) == 9); // "banana split"에서 "it"에 속하지 않는 마지막 위치는 9 ('l')
    TEST_CHECK(ms1.find_last_not_of('t') == 10); // "banana split"에서 't'에 속하지 않는 마지막 위치는 10 ('t' 제외한 마지막 글자 'l')

    // Swap
    mcs::mutex_string sw1("AAA"), sw2("BBB");

    sw1.swap(sw2); // sw1과 sw2의 내용을 서로 교환
    TEST_CHECK(sw1.str() == "BBB" && sw2.str() == "AAA");

    mcs::swap(sw1, sw2); // mcs::swap을 사용하여 sw1과 sw2의 내용을 서로 교환
    TEST_CHECK(sw1.str() == "AAA" && sw2.str() == "BBB");

    std::string str_std = "CCC";
    sw1.swap(str_std); // sw1과 str_std의 내용을 서로 교환. sw1은 "CCC"가 되고, str_std는 "AAA"가 됨.
    TEST_CHECK(sw1.str() == "CCC" && str_std == "AAA");

    // with / with_lock / synchronize / guard
    ms1 = "thread_safe_data";
    ms1.with([](std::string& s) { // with() 메서드를 사용하여 ms1의 내부 문자열에 접근하고 수정
        s += "_modified";
    });
    TEST_CHECK(ms1 == "thread_safe_data_modified");

    const mcs::mutex_string& const_ms = ms1; // const 참조를 사용하여 const_ms를 선언. const_ms는 ms1의 내부 문자열을 읽기 전용으로 접근 가능

    bool checked = const_ms.with_lock([](const std::string& s) { // with_lock() 메서드를 사용하여 const_ms의 내부 문자열에 접근하고 검사
        return s.size() > 0;
    });
    TEST_CHECK(checked);

    {
        std::string expected = ms1.str(); // 가드 진입 전에 스냅샷을 뜨거나
        auto locked = ms1.synchronize(); // synchronize() 메서드를 사용하여 ms1의 내부 문자열에 대한 잠금을 획득하고, locked는 std::unique_lock<std::mutex>를 반환

        TEST_CHECK(locked.owns_lock()); // 잠금이 성공적으로 획득되었는지 확인
        TEST_CHECK(locked->size() == expected.size()); // 잠금이 걸린 상태에서 내부 문자열의 크기가 예상과 일치하는지 확인
        TEST_CHECK(*locked == expected); // 잠금이 걸린 상태에서 내부 문자열이 예상과 일치하는지 확인
        TEST_CHECK(locked->find("thread") != std::string::npos); // 잠금이 걸린 상태에서 내부 문자열에 "thread"가 포함되어 있는지 확인
    }

    {
        const auto guard = const_ms.guard(); // guard() 메서드를 사용하여 const_ms의 내부 문자열에 대한 잠금을 획득하고, guard는 std::unique_lock<std::mutex>를 반환

        TEST_CHECK(guard.owns_lock()); // 잠금이 성공적으로 획득되었는지 확인
        TEST_CHECK(guard->find("thread") != std::string::npos); // 잠금이 걸린 상태에서 내부 문자열에 "thread"가 포함되어 있는지 확인
    }
}

// =========================================================================
// 5. Test u8string.hpp & u8str
// =========================================================================
void test_u8str() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("u8string & u8str");

    // Global conversions
    std::string test_utf8 = "한글 및 English 123! 😀";
    auto u8_str_val = mcsu8::to_u8_string(test_utf8); // std::string을 UTF-8로 변환하여 u8_str_val에 저장
    TEST_CHECK(mcsu8::to_std_string(u8_str_val) == test_utf8);

    std::wstring ws = mcsu8::to_wstring(u8_str_val); // UTF-8을 std::wstring으로 변환하여 ws에 저장
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(ws)) == test_utf8);

    std::u16string u16 = mcsu8::to_u16string(u8_str_val); // UTF-8을 std::u16string으로 변환하여 u16에 저장
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(u16)) == test_utf8);

    std::u32string u32 = mcsu8::to_u32string(u8_str_val); // UTF-8을 std::u32string으로 변환하여 u32에 저장
    TEST_CHECK(mcsu8::to_std_string(mcsu8::to_u8_string(u32)) == test_utf8);

    // u8str class
    mcsu8::u8str ustr1;
    TEST_CHECK(ustr1.empty() && ustr1.size() == 0); // ustr1이 비어있고 크기가 0인지 확인

    mcsu8::u8str ustr2("Hello 유니코드");
    TEST_CHECK(!ustr2.empty()); // ustr2가 비어있지 않은지 확인
    TEST_CHECK(ustr2.to_std_string() == "Hello 유니코드"); // ustr2의 내용을 std::string으로 변환하여 확인
    mcsu8::u8str ustr3(ustr2); // Copy constructor. ustr3는 ustr2의 내용을 복사
    TEST_CHECK(ustr3 == ustr2);

    mcsu8::u8str ustr4(std::wstring(L"WideString 한글")); // std::wstring을 u8str로 변환하여 ustr4에 저장
    TEST_CHECK(!ustr4.empty());

    mcsu8::u8str ustr5(u16); // std::u16string을 u8str로 변환하여 ustr5에 저장
    mcsu8::u8str ustr6(u32); // std::u32string을 u8str로 변환하여 ustr6에 저장
    TEST_CHECK(ustr5 == ustr6);

    // Setters & value access

    ustr1.from_std_string("Test std string"); // std::string을 u8str로 변환하여 ustr1에 저장
    TEST_CHECK(ustr1.to_std_string() == "Test std string");

    ustr1.from_cstr("Test C-string"); // C-string을 u8str로 변환하여 ustr1에 저장
    TEST_CHECK(ustr1.to_std_string() == "Test C-string");

    ustr1.from_wstring(L"From WString"); // std::wstring을 u8str로 변환하여 ustr1에 저장
    TEST_CHECK(!ustr1.empty());

    ustr1.from_u16string(u16); // std::u16string을 u8str로 변환하여 ustr1에 저장
    ustr1.from_u32string(u32); // std::u32string을 u8str로 변환하여 ustr1에 저장
    TEST_CHECK(ustr1.data() != nullptr);

    ustr1.clear(); // ustr1의 내용을 비우기
    TEST_CHECK(ustr1.empty());

    // Parsers & Setters
    ustr1.from_int64(9876543210LL); // int64_t 숫자를 u8str로 변환하여 ustr1에 저장
    int64_t parsed_i64 = 0;
    TEST_CHECK(ustr1.to_int64(parsed_i64) && parsed_i64 == 9876543210LL);

    ustr1.from_double(123.456); // double 숫자를 u8str로 변환하여 ustr1에 저장. ustr1 문자열은 123.456에 근사한 값의 문자열이 설정됨.
    double parsed_dbl = 0.0;
    TEST_CHECK(ustr1.to_double(parsed_dbl) && std::abs(parsed_dbl - 123.456) < 1e-4); // parsed_dbl이 123.456과 거의 같은지 확인

    ustr1.from_bool(true); // bool 값을 u8str로 변환하여 ustr1에 저장
    bool parsed_b = false;
    TEST_CHECK(ustr1.to_bool(parsed_b) && parsed_b == true); // parsed_b가 true인지 확인

    // Search and checks
    mcsu8::u8str search_target("가나다라_ABCD_가나다라"); // u8str를 생성하여 search_target에 저장

    TEST_CHECK(search_target.starts_with("가나다")); // search_target이 "가나다"로 시작하는지 확인
    TEST_CHECK(search_target.starts_with(mcsu8::u8str("가나다"))); // search_target이 u8str("가나다")로 시작하는지 확인
    TEST_CHECK(search_target.ends_with("다라")); // search_target이 "다라"로 끝나는지 확인
    TEST_CHECK(search_target.ends_with(mcsu8::u8str("다라"))); // search_target이 u8str("다라")로 끝나는지 확인
    TEST_CHECK(search_target.contains("ABCD")); // search_target이 "ABCD"를 포함하는지 확인
    TEST_CHECK(search_target.contains(mcsu8::u8str("ABCD"))); // search_target이 u8str("ABCD")를 포함하는지 확인

    TEST_CHECK(search_target.index_of("ABCD") >= 0); // search_target에서 "ABCD"의 첫 번째 위치가 0 이상인지 확인
    TEST_CHECK(search_target.index_of(mcsu8::u8str("ABCD")) >= 0); // search_target에서 u8str("ABCD")의 첫 번째 위치가 0 이상인지 확인
    TEST_CHECK(search_target.last_index_of("가나다라") > 0); // search_target에서 "가나다라"의 마지막 위치가 0 초과인지 확인
    TEST_CHECK(search_target.last_index_of(mcsu8::u8str("가나다라")) > 0); // search_target에서 u8str("가나다라")의 마지막 위치가 0 초과인지 확인

    // Trim
    mcsu8::u8str trim_u("   \t가나다   \n");
    trim_u.trim(); // 앞뒤 공백 제거
    TEST_CHECK(trim_u.to_std_string() == "가나다");

    // Split & Replace
    mcsu8::u8str split_target("사과/오렌지/바나나/포도");
    auto split_res = split_target.split("/"); // "/"를 기준으로 문자열을 분리하여 split_res에 저장
    TEST_CHECK(split_res.size() == 4 &&
        split_res[0].to_std_string() == "사과" &&
        split_res[1].to_std_string() == "오렌지"&&
        split_res[2].to_std_string() == "바나나"&&
        split_res[3].to_std_string() == "포도"
    );

    mcsu8::u8str rep_target("foo bar foo baz");
    rep_target.replace("foo", "qux"); // "foo"를 "qux"로 교체. 첫 번째 "foo"만 교체됨.
    TEST_CHECK(rep_target.to_std_string() == "qux bar foo baz");

    rep_target.replace_all(mcsu8::u8str("foo"), mcsu8::u8str("qux")); // "foo"를 모두 "qux"로 교체. 두 번째 "foo"도 교체됨.
    TEST_CHECK(rep_target.to_std_string() == "qux bar qux baz");

    // UTF-8 Codepoint based Left/Right/Mid/Reverse/Pad
    mcsu8::u8str cp_str("안녕하세요반갑습니다");
    // [왼쪽부터 인덱스]{오른쪽부터 인덱스}
    // [0]{9}안 [1]{8}녕 [2]{7}하 [3]{6}세 [4]{5}요 [5]{4}반 [6]{3}갑 [7]{2}습 [8]{1}니 [9]{0}다
    TEST_CHECK(cp_str.left(2).to_std_string() == "안녕"); // [0]안 [1]녕
    TEST_CHECK(cp_str.right(4).to_std_string() == "갑습니다"); // [6]갑 [7]습 [8]니 [9]다
    TEST_CHECK(cp_str.mid(2, 3).to_std_string() == "하세요"); // [2]하 [3]세 [4]요
    TEST_CHECK(cp_str.substr_utf8(2, 3).to_std_string() == "하세요"); // [2]하 [3]세 [4]요

    mcsu8::u8str rev_str("123가나다");
    rev_str.reverse_utf8(); // UTF8 문자열 뒤집기
    TEST_CHECK(rev_str.to_std_string() == "다나가321");

    mcsu8::u8str pad_str("한글");
    pad_str.pad_left(5, '-'); // UTF-8 문자열 "한글"의 길이는 2이므로, 왼쪽에 '-'를 5 - 2 = 3개 추가하여 "---한글"이 됨.
    TEST_CHECK(pad_str.to_std_string() == "---한글");

    pad_str.pad_right(7, '+'); // UTF-8 문자열 "---한글"의 길이는 5이므로, 오른쪽에 '+'를 7 - 5 = 2개 추가하여 "---한글++"이 됨.
    TEST_CHECK(pad_str.to_std_string() == "---한글++");

    // Strip prefix/suffix & Case
    mcsu8::u8str strip_str("[TAG]Content[/TAG]");

    TEST_CHECK(strip_str.strip_prefix("[TAG]")); // "[TAG]Content[/TAG]"에서 "[TAG]"를 제거하여 "Content[/TAG]"이 됨.
    TEST_CHECK(strip_str.strip_suffix("[/TAG]")); // "Content[/TAG]"에서 "[/TAG]"를 제거하여 "Content"이 됨.
    TEST_CHECK(strip_str.to_std_string() == "Content"); // "Content"이 맞는지 확인

    mcsu8::u8str case_u("Hello World");
    TEST_CHECK(case_u.equals_ignore_case_ascii("hELLO wORLD")); // "Hello World"와 "hELLO wORLD"를 대소문자 구분 없이 비교하여 같으면 true 반환.
    TEST_CHECK(case_u.equals_ignore_case_ascii(mcsu8::u8str("hELLO wORLD"))); // u8str("Hello World")와 u8str("hELLO wORLD")를 대소문자 구분 없이 비교하여 같으면 true 반환.

    TEST_CHECK(case_u.to_upper_copy().to_std_string() == "HELLO WORLD"); // 대문자로 변환한 복사본을 반환하여 "HELLO WORLD"인지 확인
    TEST_CHECK(case_u.to_lower_copy().to_std_string() == "hello world"); // 소문자로 변환한 복사본을 반환하여 "hello world"인지 확인

    case_u.to_lower(); // 원본을 소문자로 변환
    TEST_CHECK(case_u.to_std_string() == "hello world");

    case_u.to_upper(); // 원본을 대문자로 변환
    TEST_CHECK(case_u.to_std_string() == "HELLO WORLD");

    // Operators
    mcsu8::u8str op1("안녕");
    mcsu8::u8str op2("하세요");
    mcsu8::u8str op3 = op1 + op2; // "안녕" + "하세요" => "안녕하세요"
    TEST_CHECK(op3.to_std_string() == "안녕하세요");

    op3 += "!"; // "안녕하세요" + "!" => "안녕하세요!"
    TEST_CHECK(op3.to_std_string() == "안녕하세요!");
    TEST_CHECK(op3 != op1);
    TEST_CHECK(op3 == (op1 + op2 + "!"));
}

// =========================================================================
// 6. Test encoding_function.hpp & to_console_encoding.hpp
// =========================================================================
void test_encodings() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;

    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    TEST_SECTION("encoding_function & console encoding");

    std::string utf8_sample = "한글 테스트 ABC 123";

    // UTF-8 <-> UTF-16
    std::u16string u16_out;
    TEST_CHECK(mcs::utf8_to_utf16(utf8_sample, u16_out)); // UTF-8 문자열을 UTF-16으로 변환하여 u16_out에 저장

    std::string utf8_back;
    TEST_CHECK(mcs::utf16_to_utf8(u16_out, utf8_back)); // UTF-16 문자열을 다시 UTF-8로 변환하여 utf8_back에 저장
    TEST_CHECK(utf8_back == utf8_sample); // 원래 UTF-8 문자열과 변환 후 문자열이 같은지 확인

    // UTF-8 <-> wstring
    std::wstring w_out = mcs::utf8_to_utf16(utf8_sample); // UTF-8 문자열을 wstring으로 변환하여 w_out에 저장
    TEST_CHECK(!w_out.empty());
    TEST_CHECK(mcs::utf16_to_utf8(w_out) == utf8_sample); // wstring을 다시 UTF-8로 변환하여 원래 문자열과 같은지 확인

    // UTF-8 <-> UTF-32
    std::u32string u32_out;
    TEST_CHECK(mcs::utf8_to_utf32(utf8_sample, u32_out)); // UTF-8 문자열을 UTF-32로 변환하여 u32_out에 저장

    std::string utf8_from_u32;
    TEST_CHECK(mcs::utf32_to_utf8(u32_out, utf8_from_u32)); // UTF-32 문자열을 다시 UTF-8로 변환하여 utf8_from_u32에 저장
    TEST_CHECK(utf8_from_u32 == utf8_sample); // 원래 UTF-8 문자열과 변환 후 문자열이 같은지 확인

    // UTF-16 <-> UTF-32
    std::u32string u32_from_u16;
    TEST_CHECK(mcs::utf16_to_utf32(u16_out, u32_from_u16)); // UTF-16 문자열을 UTF-32로 변환하여 u32_from_u16에 저장

    std::u16string u16_from_u32;
    TEST_CHECK(mcs::utf32_to_utf16(u32_from_u16, u16_from_u32)); // UTF-32 문자열을 다시 UTF-16으로 변환하여 u16_from_u32에 저장
    TEST_CHECK(u16_from_u32 == u16_out); // 원래 UTF-16 문자열과 변환 후 문자열이 같은지 확인

    // CP949 / EUC-KR
    std::string cp949_out;
    if (mcs::utf8_to_cp949(utf8_sample, cp949_out)) { // UTF-8 문자열을 CP949로 변환하여 cp949_out에 저장
        std::string utf8_from_cp949;
        TEST_CHECK(mcs::cp949_to_utf8(cp949_out, utf8_from_cp949)); // CP949 문자열을 다시 UTF-8로 변환하여 utf8_from_cp949에 저장
        TEST_CHECK(utf8_from_cp949 == utf8_sample); // 원래 UTF-8 문자열과 변환 후 문자열이 같은지 확인

        std::string cp949_from_u16;
        TEST_CHECK(mcs::utf16_to_cp949(u16_out, cp949_from_u16)); // UTF-16 문자열을 CP949로 변환하여 cp949_from_u16에 저장

        std::u16string u16_from_cp949;
        TEST_CHECK(mcs::cp949_to_utf16(cp949_from_u16, u16_from_cp949)); // CP949 문자열을 다시 UTF-16으로 변환하여 u16_from_cp949에 저장
        TEST_CHECK(u16_from_cp949 == u16_out); // 원래 UTF-16 문자열과 변환 후 문자열이 같은지 확인
    }

    // ISO-2022-KR, JOHAB, MacKorean (환경에 따라 지원 여부 체크)
    std::string iso_out;
    if (mcs::utf8_to_iso2022kr(utf8_sample, iso_out)) { // UTF-8 문자열을 ISO-2022-KR로 변환하여 iso_out에 저장
        std::string utf8_from_iso;
        TEST_CHECK(mcs::iso2022kr_to_utf8(iso_out, utf8_from_iso)); // ISO-2022-KR 문자열을 다시 UTF-8로 변환하여 utf8_from_iso에 저장
        TEST_CHECK(!utf8_from_iso.empty()); 
    }

    std::string johab_out;
    if (mcs::utf8_to_johab(utf8_sample, johab_out)) { // UTF-8 문자열을 JOHAB로 변환하여 johab_out에 저장
        std::string utf8_from_johab;
        TEST_CHECK(mcs::johab_to_utf8(johab_out, utf8_from_johab)); // JOHAB 문자열을 다시 UTF-8로 변환하여 utf8_from_johab에 저장
        TEST_CHECK(!utf8_from_johab.empty());
    }

    std::string mac_out;
    if (mcs::utf8_to_mackorean(utf8_sample, mac_out)) { // UTF-8 문자열을 MacKorean으로 변환하여 mac_out에 저장
        std::string utf8_from_mac;
        TEST_CHECK(mcs::mackorean_to_utf8(mac_out, utf8_from_mac)); // MacKorean 문자열을 다시 UTF-8로 변환하여 utf8_from_mac에 저장
        TEST_CHECK(!utf8_from_mac.empty());
    }

    try {
        // *_or_throw helpers
        TEST_CHECK(mcs::utf8_to_utf16_or_throw(utf8_sample) == u16_out); // UTF-8 문자열을 UTF-16으로 변환하여 u16_out과 같은지 확인
        TEST_CHECK(mcs::utf16_to_utf8_or_throw(u16_out) == utf8_sample); // UTF-16 문자열을 다시 UTF-8로 변환하여 원래 문자열과 같은지 확인
        TEST_CHECK(mcs::utf8_to_utf32_or_throw(utf8_sample) == u32_out); // UTF-8 문자열을 UTF-32으로 변환하여 u32_out과 같은지 확인
        TEST_CHECK(mcs::utf32_to_utf8_or_throw(u32_out) == utf8_sample); // UTF-32 문자열을 다시 UTF-8로 변환하여 원래 문자열과 같은지 확인
        TEST_CHECK(mcs::utf16_to_utf32_or_throw(u16_out) == u32_out); // UTF-16 문자열을 UTF-32으로 변환하여 u32_out과 같은지 확인
        TEST_CHECK(mcs::utf32_to_utf16_or_throw(u32_out) == u16_out); // UTF-32 문자열을 다시 UTF-16으로 변환하여 원래 문자열과 같은지 확인
    } catch (const std::exception& ex) {
        eprint(endl, tce("[EXCEPTION ERROR] "), ex.what());
        TEST_CHECK(false); // 예외 발생 시 테스트 실패
    }

    // Console encoding
    std::string console_bytes = mcs::to_console_encoding(utf8_sample); // UTF-8 문자열을 콘솔 인코딩으로 변환하여 console_bytes에 저장
    TEST_CHECK(!console_bytes.empty());

    std::string from_console = mcs::from_console_encoding(console_bytes); // 콘솔 인코딩 문자열을 다시 UTF-8로 변환하여 from_console에 저장
    TEST_CHECK(!from_console.empty());
}

// =========================================================================
// Main Entry
// =========================================================================
int main() {
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
    } catch (const std::exception& ex) {
        eprint(endl, tce("[EXCEPTION ERROR] "), ex.what());
        return 1;
    }

    return 0;
}
