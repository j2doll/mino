#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cassert>
#include <cmath>

#include "mino/core/string/string.hpp"

void test_korean_features_chosung_extraction_search_jamo_compose_decompose() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // =========================================================================
    // 1. 한국어 특화 기능 (초성 추출 / 초성 검색 / 자모 분해 및 재조합)
    // =========================================================================
    mcsp::println("===========================================");
    mcsp::println("1. 한국어 특화 기능 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    // (1) "홍길동" -> "ㅎㄱㄷ" 초성 추출 검증
    std::string name = "홍길동";
    std::string chosung = mcs::extract_chosung(name);
    assert(chosung == "ㅎㄱㄷ");
    mcsp::println("[PASS] 초성 추출: {} -> {}", name, chosung);

    // 혼합 문자열 초성 추출 검증 (영문/숫자/공백 유지)
    std::string mixed = "C++17 한글 엔진 v2.0";
    std::string mixed_chosung = mcs::extract_chosung(mixed);
    assert(mixed_chosung == "C++17 ㅎㄱ ㅇㅈ v2.0");
    mcsp::println("[PASS] 혼합 초성 추출: {}", mixed_chosung);

    // 초성 검색 매칭 검증
    std::string target_doc = "동해물과 백두산이 마르고 닳도록";
    assert(mcs::matches_chosung(target_doc, "ㅂㄷㅅ") == true); // "백두산"에 매칭
    assert(mcs::matches_chosung(target_doc, "백ㄷㅅ") == true); // "백두산"에 매칭
    assert(mcs::matches_chosung(target_doc, "ㅎㄹㅅ") == false); // ㅎㄹㅅ는 없음
    mcsp::println("[PASS] 초성 패턴 매칭 (ㅂㄷㅅ, 백ㄷㅅ, ㅎㄹㅅ)");

    // 자모 분해 및 재조합 검증
    std::string word = "한글";
    std::string decomposed = mcs::decompose_hangul(word); // "한글" => "ㅎㅏㄴㄱㅡㄹ" 자모 분해
    assert(decomposed == "ㅎㅏㄴㄱㅡㄹ"); // MacOS 및 Linux 환경의 파일명에 호환되는 자모 분해
    // std::filesystem::path p(decomposed); // 파일명으로 사용 가능 여부 확인
    // std::filesystem::create_directory(p); // 실제로 디렉토리 생성 가능 여부 확인

    std::string composed = mcs::compose_hangul(decomposed); // "ㅎㅏㄴㄱㅡㄹ" => "한글" 재조합
    assert(composed == word);
    mcsp::println("[PASS] 자모 분해({}) -> 재조합({})", decomposed, composed);

}


