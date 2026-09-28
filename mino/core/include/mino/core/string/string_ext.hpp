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

    /**
     * @brief UTF-8 문자열에서 한글 음절의 초성(호환용 자모: ㄱ, ㄴ, ㄷ ...)을 추출합니다.
     *        한글 음절이 아닌 문자(영숫자, 공백 등)는 원본 그대로 보존됩니다.
     *        예: extract_chosung("홍길동") -> "ㅎㄱㄷ"
     */
    std::string extract_chosung(std::string_view utf8_text);

    /**
     * @brief 초성 검색 패턴 일치 여부를 검사합니다.
     *        패턴이 완성형 음절이면 글자 자체를 비교하고, 초성 자모이면 대상 음절의 초성과 비교합니다.
     *        예: matches_chosung("홍길동", "ㅎㄱㄷ") -> true
     *        예: matches_chosung("홍길동", "홍ㄱㄷ") -> true
     */
    bool matches_chosung(std::string_view target, std::string_view chosung_pattern);

    /**
     * @brief 한글 음절(가~힣)을 자모 단위(초성, 중성, 종성)로 분해합니다.
     * @param use_compat_jamo true: 호환용 자모(0x3131~: ㄱ, ㅏ), false: 표준 자모(0x1100~)
     */
    std::string decompose_hangul(std::string_view utf8_text, bool use_compat_jamo = true);

    /**
     * @brief 분해된 한글 자모(초성+중성[+종성])를 완성형 음절로 재조합합니다.
     */
    std::string compose_hangul(std::string_view utf8_text);


    // =========================================================================
    // 2. 유니코드 및 전각/반각 변환 (Advanced Unicode Utilities)
    // =========================================================================

    /**
     * @brief 반각 ASCII 문자(! ~ ~) 및 공백을 전각 문자(！ ~ ～, 전각공백)로 변환합니다.
     */
    std::string to_full_width(std::string_view utf8_text);

    /**
     * @brief 전각 문자(！ ~ ～, 전각공백)를 반각 ASCII 문자로 변환합니다.
     */
    std::string to_half_width(std::string_view utf8_text);

    /**
     * @brief 결합 자모 및 다이아크리틱 결합 기호를 제외한 시각적 글자 수(Grapheme 단위)를 계산합니다.
     */
    std::size_t utf8_grapheme_count(std::string_view utf8_text);

    /**
     * @brief 시각적 글자 수(Grapheme) 단위로 안전하게 잘라내어 말줄임표를 추가합니다.
     */
    std::string ellipsize_grapheme_safe(std::string_view utf8_text, std::size_t max_graphemes, std::string_view ellipsis = "...");


    // =========================================================================
    // 3. 네이밍 스타일 및 텍스트 변환 (Case Styles & Slugs)
    // =========================================================================

    std::string to_snake_case(std::string_view s);
    std::string to_camel_case(std::string_view s);
    std::string to_pascal_case(std::string_view s);
    std::string to_kebab_case(std::string_view s);

    /**
     * @brief 영숫자 및 한글 외의 문자를 구분자(기본 '-')로 치환하고 소문자화한 URL 슬러그를 생성합니다.
     */
    std::string to_slug(std::string_view s, char separator = '-');


    // =========================================================================
    // 4. 문자열 유사도 및 퍼지 검색 (Fuzzy Matching & Metrics)
    // =========================================================================

    /**
     * @brief UTF-8 문자(코드포인트) 단위 레벤슈타인 편집 거리를 계산합니다.
     */
    std::size_t levenshtein_distance(std::string_view a, std::string_view b);

    /**
     * @brief 레벤슈타인 거리를 기반으로 한 두 문자열의 유사도 비율(0.0 ~ 1.0)을 반환합니다.
     */
    double similarity_ratio(std::string_view a, std::string_view b);

    /**
     * @brief 서브시퀀스 기반 퍼지 검색 (pattern의 문자들이 순서대로 target에 존재하는지 확인)
     */
    bool fuzzy_match(std::string_view pattern, std::string_view target, bool ignore_case = true);


    // =========================================================================
    // 5. 제로-할당 뷰 기반 유틸리티 (Zero-Allocation Views)
    // =========================================================================

    std::string_view ltrim_view(std::string_view sv);
    std::string_view rtrim_view(std::string_view sv);
    std::string_view trim_view(std::string_view sv);

    /**
     * @brief 복사/동적 할당 없이 단일 문자 구분자로 나눈 std::string_view 목록을 반환합니다.
     */
    std::vector<std::string_view> split_view(std::string_view s, char delimiter, bool keep_empty = false);

    /**
     * @brief 복사/동적 할당 없이 문자열 구분자로 나눈 std::string_view 목록을 반환합니다.
     */
    std::vector<std::string_view> split_view(std::string_view s, std::string_view delimiter, bool keep_empty = false);


    // =========================================================================
    // 6. 구조화 데이터 안전 토크나이저 (CSV & Template Substitution)
    // =========================================================================

    /**
     * @brief RFC 4180 규격의 CSV 라인을 파싱합니다.
     *        따옴표 내부의 구분자(쉼표) 및 연속 따옴표 이스케이프("")를 정확하게 처리합니다.
     */
    std::vector<std::string> split_csv(std::string_view line, char delim = ',', char quote = '"');

    /**
     * @brief 템플릿 문자열 내의 변수 플레이스홀더를 맵(Key-Value)의 값으로 치환합니다.
     */
    std::string format_map(
        std::string_view tmpl,
        const std::unordered_map<std::string, std::string>& vars,
        std::string_view prefix = "{",
        std::string_view suffix = "}");

} // namespace mino::core::string
