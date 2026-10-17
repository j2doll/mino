#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <utility>
#include <charconv>
#include <cstdint>

#include "mino/core/string/u8/u8.hpp"

namespace mino::core::string
{
    // -------------------------
    // trim 계열
    // -------------------------

    // 앞쪽 공백 제거 (원본 직접 수정)
    // 예) '   123   ' -> '123   '
    void ltrim(std::string& s);

    // 뒤쪽 공백 제거 (원본 직접 수정)
    // 예) '   123   ' -> '   123'
    void rtrim(std::string& s);

    // 양쪽 공백 제거 (원본 직접 수정)
    // 예) '   123   ' -> '123'
    void trim(std::string& s);

    // 앞쪽 공백 제거 (복사본 제공)
    // 예) '   123   ' -> '123   '
    std::string ltrim_copy(std::string s);

    // 뒤쪽 공백 제거 (복사본 제공)
    // 예) '   123   ' -> '   123'
    std::string rtrim_copy(std::string s);

    // 양쪽 공백 제거 (복사본 제공)
    // 예) '   123   ' -> '123'
    std::string trim_copy(std::string s);

    // -------------------------
    // 치환 계열
    // -------------------------

    // 전체 치환(복사본). find_token 이 비어있으면 원문 반환
    // 예) replace("abcabc", "ab", "xy") -> "xycxyc"
    std::string replace(
        const std::string& text, // 원문
        const std::string& find_token, // 찾을 문자열
        const std::string& replace_token); // 치환할 문자열

    // 전체 치환(제자리). find_token 이 비어있으면 아무 작업도 하지 않음
    // 예) replace_inplace(s, "ab", "xy") -> s = "xycxyc"
    void replace_all_inplace(
        std::string& text, // 원문
        const std::string& find_token, // 찾을 문자열
        const std::string& replace_token); // 치환할 문자열

    // 첫 번째만 치환(복사본)
    // 예) replace_first("abcabc", "ab", "xy") -> "xycabc"
    std::string replace_first(
        const std::string& text, // 원문
        const std::string& find_token, // 찾을 문자열
        const std::string& replace_token); // 치환할 문자열

    // 마지막만 치환(복사본)
    // 예) replace_last("abcabc", "ab", "xy") -> "abcxyc"
    std::string replace_last(
        const std::string& text, // 원문
        const std::string& find_token, // 찾을 문자열
        const std::string& replace_token); // 치환할 문자열

    // 여러 (find->replace) 쌍을 순차 적용하여 치환
    // 예) replace_all_map("abcabc", {{"ab", "xy"}, {"c", "z"}}) -> "xyzxyz"
    std::string replace_all_map(
        const std::string& text, // 원문
        const std::vector<std::pair<std::string, std::string>>& replacements); // 치환 쌍 목록

    // -------------------------
    // 대소문자 변환 / 포함 / 접두/접미
    // -------------------------

    // 문자열을 소문자로 변환 (ASCII 기준)
    // 예) to_lower("Hello, World!") -> "hello, world!"
    std::string to_lower(const std::string& s);

    // 문자열을 대문자로 변환 (ASCII 기준)
    // 예) to_upper("Hello, World!") -> "HELLO, WORLD!"
    std::string to_upper(const std::string& s);

    // 포함 여부 (대소문자 구분)
    // 예) contains("Hello, World!", "World") -> true
    bool contains(
        const std::string& s, // 원문
        const std::string& token); // 찾을 문자열

    // 문자열이 특정 접두사로 시작하는지 확인 (C++17 호환)
    // 예) starts_with("Hello, World!", "Hello") -> true
    bool starts_with(
        const std::string& s, // 원문
        const std::string& prefix); // 찾을 접두사

    // 문자열이 특정 접미사로 끝나는지 확인 (C++17 호환)
    // 예) ends_with("Hello, World!", "World!") -> true
    bool ends_with(
        const std::string& s, // 원문
        const std::string& suffix); // 찾을 접미사

    // 대소문자 무시 포함 여부
    // 예) icontains("Hello, World!", "world") -> true
    bool iequals(
        const std::string& a, // 첫 번째 문자열
        const std::string& b); // 두 번째 문자열

    // 대소문자 무시 포함 여부
    // 예) icontains("Hello, World!", "world") -> true
    bool icontains(
        const std::string& s, // 원문
        const std::string& token); // 찾을 문자열

    // 대소문자 무시 접두사 확인
    // 예) istarts_with("Hello, World!", "hello") -> true
    bool istarts_with(
        const std::string& s, // 원문
        const std::string& prefix); // 찾을 접두사

    // 대소문자 무시 접미사 확인
    // 예) iends_with("Hello, World!", "WORLD!") -> true
    bool iends_with(
        const std::string& s, // 원문
        const std::string& suffix); // 찾을 접미사

    // 접두사 집합 매칭
    // 예) starts_with_any("Hello, World!", {"Hi", "Hello"}) -> true
    bool starts_with_any(
        const std::string& s, // 원문
        const std::vector<std::string>& prefixes); // 찾을 접두사 집합

    // 접미사 집합 매칭
    // 예) ends_with_any("Hello, World!", {"WORLD!", "Earth!"}) -> true
    bool ends_with_any(
        const std::string& s, // 원문
        const std::vector<std::string>& suffixes); // 찾을 접미사 집합

    // 여러 문자열의 공통 접두사를 찾음
    // 예) common_prefix({"flower", "flow", "flight"}) -> "fl"
    std::string common_prefix(
        const std::vector<std::string>& prefix_items); // 접두사 집합

    // 여러 문자열의 공통 접미사를 찾음
    // 예) common_suffix({"running", "jogging", "walking"}) -> "ing"
    std::string common_suffix(
        const std::vector<std::string>& suffix_items); // 접미사 집합

    // -------------------------
    // 분할/결합
    // -------------------------

    // 구분자(단일 문자) 기준 분할. keep_empty=true 이면 빈 토큰 유지.
    // 예) split("a,b,,c", ',') -> {"a", "b", "", "c"}
    std::vector<std::string> split(
         const std::string& s, // 원본 문자열
         char delimiter); // 구분자 문자

    // 구분자(문자열) 기준 분할. keep_empty=true 이면 빈 토큰 유지.
    // 예) split("a--b----c", "--") -> {"a", "b", "", "", "c"}
    std::vector<std::string> split(
        const std::string& s, // 원본 문자열
        const std::string& delimiter, // 구분자 문자열
        bool keep_empty = false); // 빈 토큰 유지 여부

    // 구분자 집합 기준 분할. keep_empty=true 이면 빈 토큰 유지
    // 예) tokenize_any_of("a,b;c.d", ",;.") -> {"a", "b", "c", "d"}
    std::vector<std::string> tokenize_any_of(
        const std::string& s, // 원본 문자열
        const std::string& delims, // 구분자 집합
        bool keep_empty = false); // 빈 토큰 유지 여부

    // 구분자 기준 분할 후 각 토큰의 앞뒤 공백 제거. drop_empty=true 이면 빈 토큰 제거.
    // 예) split_trimmed("  a , b , , c  ", ',', true) -> {"a", "b", "c"}
    std::vector<std::string> split_trimmed(
        const std::string& s, // 원본 문자열
        char delimiter, // 구분자 문자
        bool drop_empty = true); // 빈 토큰 제거 여부

    // 개행 기준 분할. keep_empty=true 이면 빈 토큰 유지.
    // 예) split_lines("line1\nline2\n\nline4", true) -> {"line1", "line2", "", "line4"}
    std::vector<std::string> split_lines(
         const std::string& s, // 원본 문자열
         bool keep_empty = false); // 빈 토큰 유지 여부

    // 토큰 목록을 구분자(delimiter)로 결합
    // 예) join({"a", "b", "c"}, ",") -> "a,b,c"
    std::string join(
        const std::vector<std::string>& tokens, // 토큰 목록
        const std::string& delimiter); // 구분자 문자열

    // -------------------------
    // 공백/개행 정규화
    // -------------------------

    // 문자열이 비어있거나 공백만 있는지 확인
    // 예) is_blank("   ") -> true, is_blank("abc") -> false
    bool is_blank(const std::string& s);

    // 연속 공백을 하나의 공백으로 축약하고 앞뒤 공백 제거
    // 예) collapse_spaces("  a   b  c  ") -> "a b c"
    std::string collapse_spaces(const std::string& s);

    // 모든 개행을 nl 로 통일 (\r\n, \r, \n → nl). 기본 nl="\n"
    // 예) normalize_newlines("line1\r\nline2\rline3\n", "\n") -> "line1\nline2\nline3\n"
    std::string normalize_newlines(
        const std::string& s, // 원본 문자열
        const std::string& nl = "\n"); // 통일할 개행 문자열

    // -------------------------
    // 패딩/반복/삭제/감싸기
    // -------------------------

    // 문자열을 지정된 너비로 패딩. fill은 기본 공백
    // 예) pad_left("abc", 5, '*') -> "**abc"
    std::string pad_left(
        const std::string& s, // 원본 문자열
        std::size_t width, // 목표 너비
        char fill = ' '); // 채울 문자 (기본: 공백)

    // 문자열을 지정된 너비로 패딩. fill은 기본 공백
    // 예) pad_right("abc", 5, '*') -> "abc**"
    std::string pad_right(
        const std::string& s, // 원본 문자열
        std::size_t width, // 목표 너비
        char fill = ' '); // 채울 문자 (기본: 공백)

    // 문자열을 지정된 너비로 패딩. fill은 기본 공백
    // 예) pad_center("abc", 5, '*') -> "*abc*"
    std::string pad_center(
        const std::string& s, // 원본 문자열
        std::size_t width, // 목표 너비
        char fill = ' '); // 채울 문자 (기본: 공백)

    // 특정 문자 집합 제거
    // 예) remove_chars("abcde", "bd") -> "ace"
    std::string remove_chars(
        const std::string& s, // 원본 문자열
        const std::string& chars); // 제거할 문자 집합

    // 문자열 반복
    // 예) repeat("abc", 3) -> "abcabcabc"
     std::string repeat(
         const std::string& s, // 원본 문자열
         std::size_t count); // 반복 횟수

     // 문자열이 지정된 따옴표로 감싸져 있으면 제거. 기본 따옴표는 '"' 
     // 예) strip_quotes("\"Hello\"") -> "Hello"
     std::string strip_quotes(
         const std::string& s, // 원본 문자열
         char quote = '"'); // 감쌀 따옴표 문자 (기본: '"')

     // 문자열을 지정된 따옴표로 감싸고, 내부 따옴표는 이스케이프 처리.
     // 기본 따옴표는 '"', escape=true이면 내부 따옴표를 이스케이프 처리
     // 예) quote("Hello") -> "\"Hello\""
     std::string quote(
         const std::string& s, // 원본 문자열
         char quote = '"', // 감쌀 따옴표 문자 (기본: '"')
         bool escape = true); // 내부 따옴표 이스케이프 여부 (기본: true)

     // 문자열이 지정된 접두사/접미사로 감싸져 있으면 제거
     // 예) strip_affixes("HelloWorld", "Hello", "World") -> ""
     std::string surround_if_missing(
         const std::string& s, // 원본 문자열
         const std::string& prefix, // 접두사
         const std::string& suffix); // 접미사

    // 들여쓰기: 각 라인 앞에 prefix 삽입
    // 예) indent_lines("line1\nline2", "  ") -> "  line1\n  line2"
    std::string indent_lines(
        const std::string& s, // 원본 문자열
        const std::string& prefix); // 각 라인 앞에 삽입할 접두사

    // -------------------------
    // 접두/접미 제거
    // -------------------------

    // 접두사 제거 (원본 직접 수정). 접두사가 없으면 false 반환
    // 예) remove_prefix(s, "Hello") -> s="World", return=true
    bool remove_prefix(
        std::string& s, // 원본 문자열
        const std::string& prefix); // 제거할 접두사

    // 접미사 제거 (원본 직접 수정). 접미사가 없으면 false 반환
    // 예) remove_suffix(s, "World") -> s="Hello", return=true
    bool remove_suffix(
        std::string& s, // 원본 문자열
        const std::string& suffix); // 제거할 접미사

    // 접두사 제거 (복사본 반환). 접두사가 없으면 원문 반환
    // 예) removed_prefix("HelloWorld", "Hello") -> "World"
    std::string removed_prefix(
        const std::string& s, // 원본 문자열
        const std::string& prefix); // 제거할 접두사

    // 접미사 제거 (복사본 반환). 접미사가 없으면 원문 반환
    // 예) removed_suffix("HelloWorld", "World") -> "Hello"
    std::string removed_suffix(
        const std::string& s, // 원본 문자열
        const std::string& suffix); // 제거할 접미사

    // -------------------------
    // 안전 부분문자열/말줄임
    // -------------------------

    // 안전한 부분 문자열 추출 (범위 초과 시 빈 문자열 반환)
    // 예) safe_substr("Hello", 1, 3) -> "ell", safe_substr("Hello", 10, 3) -> ""
    std::string safe_substr(
        const std::string& s, // 원본 문자열
        std::size_t pos, // 시작 위치
        std::size_t count); // 추출할 길이

    // 최대 길이를 초과하면 말줄임 부호를 붙여 자르기 (바이트 기준)
    // 예) ellipsize("Hello, World!", 5) -> "He..."
    std::string ellipsize(
        const std::string& s, // 원본 문자열
        std::size_t max_len, // 최대 길이
        const std::string& ellipsis = "..."); // 말줄임 부호 (기본: "...")

    // UTF-8 코드포인트 경계 안전 말줄임(코드포인트 기준; 결합문자/이모지 클러스터는 고려하지 않음)
    // 예) ellipsize_utf8_safe("안녕하세요", 3) -> "안녕..."
    std::string ellipsize_utf8_safe(
        const std::string& s, // 원본 문자열
        std::size_t max_codepoints, // 최대 코드포인트 수
        const std::string& ellipsis = "..."); // 말줄임 부호 (기본: "...")

    // -------------------------
    // 숫자 파싱/검증 (ASCII)
    // -------------------------

    // 문자열이 숫자로만 이루어져 있는지 확인 (부호, 소수점, 지수 등은 허용하지 않음)
    // 예) is_digits("12345") -> true, is_digits("12a45") -> false
    bool is_digits(const std::string& s);

    // 문자열을 int64_t로 변환. 변환 실패 시 false 반환
    // 예) try_parse_int64("12345", out) -> true, out=12345
    bool try_parse_int64(
        const std::string& s, // 원본 문자열
        std::int64_t& out, // 변환된 정수값을 저장할 변수
        int base = 10); // 진수 (기본: 10진수)

    // 문자열을 double로 변환. 변환 실패 시 false 반환
    // 예) try_parse_double("3.14", out) -> true, out=
    bool try_parse_double(
        const std::string& s, // 원본 문자열
        double& out); // 변환된 실수값을 저장할 변수

    // 와일드 카드 패턴 매칭. '*'는 0개 이상의 임의 문자, '?'는 1개의 임의 문자를 의미
    // 예) wildcard_match("hello", "h*o") -> true, wildcard_match
    bool wildcard_match(
        const std::string& text, // 원본 문자열
        const std::string& pattern); // 와일드 카드 패턴 문자열

    // 숫자에 천 단위(3) 또는 만 단위(4) 구분자를 추가합니다.
    // 예) add_separator("1234567") -> "1,234,567"
    std::string add_separator(
        const std::string& str_num,
        int step = 3,
        char separator = ',');

    // 숫자 문자열을 한국어로 읽기 좋게 변환합니다.
    // 예) to_human_readable_korean("1234567", true) -> "123만 4,567"
    // 예) to_human_readable_korean("1234567", false) -> "123만 4567"
    std::string to_human_readable_korean(
        const std::string& str_num, // 숫자 문자열
        bool include_comma = true); // 각 단위 내부(만 단위)에 콤마를 넣을지 여부

}  
