#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>

namespace mino::core::string {

    // =========================================================================
    // 1. 한국어 특화 기능 (Korean Specialized Features)
    // =========================================================================

    // UTF-8 문자열에서 한글 음절의 초성(호환용 자모: ㄱ, ㄴ, ㄷ ...)을 추출합니다.
    // 한글 음절이 아닌 문자(영숫자, 공백 등)는 원본 그대로 보존됩니다.
    // 예: extract_chosung("홍길동") -> "ㅎㄱㄷ"
    std::string extract_chosung(
        std::string_view utf8_text); // UTF-8 문자열

    // 초성 검색 패턴 일치 여부를 검사합니다.
    // 패턴이 완성형 음절이면 글자 자체를 비교하고, 초성 자모이면 대상 음절의 초성과 비교합니다.
    // 예: matches_chosung("홍길동", "ㅎㄱㄷ") -> true
    // 예: matches_chosung("홍길동", "홍ㄱㄷ") -> true
    bool matches_chosung(
        std::string_view utf8_target, // 대상 문자열 (UTF-8)     
        std::string_view chosung_pattern); // 초성 패턴 문자열 (UTF-8)

    // 한글 음절(가~힣)을 자모 단위(초성, 중성, 종성)로 분해합니다.
    // 예) "강" -> "ㄱㅏㅇ", "한글" -> "ㅎㅏㄴㄱㅡㄹ"
    // 활용 분야: Mac OS 에서는 파일명을 자모가 분리된 한글로 저장하는 경우가 있음.
    std::string decompose_hangul(
        std::string_view utf8_text, // 원본 UTF-8 문자열
        bool use_compat_jamo = true); // true: 호환용 자모(ㄱ, ㄴ, ㄷ ...) 사용, false: 현대 한글 자모(ᄀ, ᄂ, ᄃ ...) 사용

    // 분해된 한글 자모(초성+중성[+종성])를 완성형 음절로 재조합합니다.
    // 예) "ㄱㅏㅇ" -> "강", "ㅎㅏㄴㄱㅡㄹ" -> "한글"
    std::string compose_hangul(
        std::string_view utf8_text); // 원본 UTF-8 문자열 (자모 단위로 분해된 한글)

    // =========================================================================
    // 2. 유니코드 및 전각/반각 변환 (Advanced Unicode Utilities)
    // =========================================================================

    // 반각 ASCII 문자(! ~ ~) 및 공백을 전각 문자(！ ~ ～, 전각공백)로 변환합니다.
    // 에) to_full_width("Hello, World!") -> "Ｈｅｌｌｏ，　Ｗｏｒｌｄ！"
    std::string to_full_width(
        std::string_view utf8_text); // 원본 UTF-8 문자열

    // 전각 문자(！ ~ ～, 전각공백)를 반각 ASCII 문자로 변환합니다.
    // 에) to_half_width("Ｈｅｌｌｏ，　Ｗｏｒｌｄ！") -> "Hello, World!"
    std::string to_half_width(
        std::string_view utf8_text); // 원본 UTF-8 문자열

    // 결합 자모 및 다이아크리틱 결합 기호를 제외한 시각적 글자 수(Grapheme 단위)를 계산합니다.
    // 예) utf8_grapheme_count("한글") -> 2, utf8_grapheme_count("á") -> 1
    std::size_t utf8_grapheme_count(
        std::string_view utf8_text); // 원본 UTF-8 문자열

    // 시각적 글자 수(Grapheme) 단위로 안전하게 잘라내어 말줄임표를 추가합니다.
    // 예) ellipsize_grapheme_safe("한글 테스트", 3) -> "한글 ..." 
    std::string ellipsize_grapheme_safe(
        std::string_view utf8_text, // 원본 UTF-8 문자열
        std::size_t max_graphemes, // 최대 시각적 글자 수
        std::string_view ellipsis = "..."); // 말줄임표 (기본: "...")


    // =========================================================================
    // 3. 네이밍 스타일 및 텍스트 변환 (Case Styles & Slugs)
    // =========================================================================

    // 소스 코드 문자열을 스네이크 케이스(snake_case)로 변환합니다.
    std::string to_snake_case(std::string_view s);

    // 소스 코드 문자열을 카멜 케이스(camelCase)로 변환합니다.
    std::string to_camel_case(std::string_view s);

    // 소스 코드 문자열을 파스칼 케이스(PascalCase)로 변환합니다.
    std::string to_pascal_case(std::string_view s);

    // 소스 코드 문자열을 케밥 케이스(kebab-case)로 변환합니다.
    std::string to_kebab_case(std::string_view s);

    // 영숫자 및 한글 외의 문자를 구분자(기본 '-')로 치환하고 소문자화한 URL 슬러그를 생성합니다.
    // 예) to_slug("Hello, World!") -> "hello-world"
    std::string to_slug(
        std::string_view s, // 원본 문자열
        char separator = '-'); // 구분자 (기본: '-')

    // =========================================================================
    // 4. 문자열 유사도 및 퍼지 검색 (Fuzzy Matching & Metrics)
    // =========================================================================

    // UTF-8 문자(코드포인트) 단위 레벤슈타인 편집 거리를 계산합니다.
    // 예) levenshtein_distance("kitten", "sitting") -> 3
    // 예) levenshtein_distance("한글", "한글자모") -> 2
    std::size_t levenshtein_distance(
        std::string_view src, // 원본 문자열
        std::string_view cmp); // 비교 대상 문자열

    // 레벤슈타인 거리를 기반으로 한 두 문자열의 유사도 비율(0.0 ~ 1.0)을 반환합니다.
    // 0.0은 완전히 다름, 1.0은 완전히 동일함을 의미합니다.
    // 예) similarity_ratio("kitten", "sitting") -> 0.5714
    // 예) similarity_ratio("한글", "한글자모") -> 0.6667
    double similarity_ratio(
        std::string_view src, // 원본 문자열
        std::string_view cmp); // 비교 대상 문자열

    // 서브시퀀스 기반 퍼지 검색 (pattern의 문자들이 순서대로 target에 존재하는지 확인)
    // 예) fuzzy_match("hld", "Hello, World!") -> true
    // 예) fuzzy_match("hld", "Hello, World!", false) -> false (대소문자 구분)
    bool fuzzy_match(
        std::string_view pattern, // 검색 패턴 문자열
        std::string_view target, // 대상 문자열
        bool ignore_case = true); // 대소문자 무시 여부 (기본: true)

    // =========================================================================
    // 5. 제로-할당 뷰 기반 유틸리티 (Zero-Allocation Views)
    // =========================================================================

    // 문자열 왼쪽의 공백 문자(스페이스, 탭, 개행 등)를 제거한 std::string_view를 반환합니다.
    std::string_view ltrim_view(
        std::string_view sv); // 원본 문자열 (UTF-8)

    // 문자열 오른쪽의 공백 문자(스페이스, 탭, 개행 등)를 제거한 std::string_view를 반환합니다.
    std::string_view rtrim_view(
        std::string_view sv); // 원본 문자열 (UTF-8)

    // 문자열 양쪽의 공백 문자(스페이스, 탭, 개행 등)를 제거한 std::string_view를 반환합니다.
    std::string_view trim_view(
        std::string_view sv); // 원본 문자열 (UTF-8)

    // 복사/동적 할당 없이 단일 문자 구분자로 나눈 std::string_view 목록을 반환합니다.
    std::vector<std::string_view> split_view(
        std::string_view s, // 원본 문자열 (UTF-8)
        char delimiter, // 단일 문자 구분자
        bool keep_empty = false); // 빈 토큰 유지 여부 (true: 빈 토큰 포함, false: 빈 토큰 제거)

    // 복사/동적 할당 없이 문자열 구분자로 나눈 std::string_view 목록을 반환합니다.
    std::vector<std::string_view> split_view(
        std::string_view s, // 원본 문자열 (UTF-8)
        std::string_view delimiter, // 문자열 구분자
        bool keep_empty = false); // 빈 토큰 유지 여부 (true: 빈 토큰 포함, false: 빈 토큰 제거)

    // =========================================================================
    // 6. 구조화 데이터 안전 토크나이저 (CSV & Template Substitution)
    // =========================================================================

    // RFC 4180 규격의 CSV 라인을 파싱합니다.
    // 따옴표 내부의 구분자(쉼표) 및 연속 따옴표 이스케이프("")를 정확하게 처리합니다.
    std::vector<std::string> split_csv(
        std::string_view line, // CSV 라인 문자열
        char delim = ',', // CSV 구분자 (기본: 쉼표)
        char quote = '"'); // CSV 인용부호 (기본: 큰따옴표)

    // 템플릿 문자열 내의 변수 플레이스홀더를 맵(Key-Value)의 값으로 치환합니다.
    std::string format_map(
        std::string_view tmpl, // 템플릿 문자열 (예: "Hello, {name}!")
        const std::unordered_map<std::string, std::string>& vars, // 치환할 변수 맵. 예: {"name": "Alice"}
        std::string_view prefix = "{", // 플레이스홀더 접두사 (기본: "{")
        std::string_view suffix = "}"); // 플레이스홀더 접미사 (기본: "}")

} // namespace mino::core::string
