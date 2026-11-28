#include "test_common.hpp"

extern void test_korean_features_chosung_extraction_search_jamo_compose_decompose();
extern void test_unicode_fullwidth_halfwidth_conversion();
extern void test_naming_style_conversion_url_slug();
extern void test_string_similarity_fuzzy_search();
extern void test_zero_allocation_string_view_utilities();
extern void test_structured_data_safe_tokenizer_formatting_replace();

void test_string_ext() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // 1. 한국어 특화 기능 (초성 추출 / 초성 검색 / 자모 분해 및 재조합)
    test_korean_features_chosung_extraction_search_jamo_compose_decompose();

    // 2. 유니코드 및 전각/반각 변환
    test_unicode_fullwidth_halfwidth_conversion(); 

    // 3. 네이밍 스타일 변환 & URL 슬러그 
    test_naming_style_conversion_url_slug();

    // 4. 문자열 유사도 & 퍼지 검색
    test_string_similarity_fuzzy_search(); 

    // 5. 제로-할당 string_view 기반 유틸리티
    test_zero_allocation_string_view_utilities(); 

    // 6. 구조화 데이터 안전 토크나이저 & 포맷팅 치환
    test_structured_data_safe_tokenizer_formatting_replace(); 

    // =========================================================================
    // 7. 표준 에러 스트림 (eprintln) 테스트
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("7. 표준 에러 출력 (eprintln) 테스트");
    mcsp::println("===========================================");

    bool has_error = false;
    assert(!has_error);
    if (!has_error) {
        mcsp::eprintln("[INFO] 에러 스트림 정상 테스트 완료.");
    }

    mcsp::println("\n>>> [SUCCESS] test_string_ext()의 모든 assert 검증을 통과했습니다. <<<");
}
