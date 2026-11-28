#include "test_common.hpp"

void test_string_similarity_fuzzy_search() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // =========================================================================
    // 4. 문자열 유사도 & 퍼지 검색
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("4. 문자열 유사도 & 퍼지 검색 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    std::string str_a = "홍길동";
    std::string str_b = "홍길순";
    std::size_t dist = mcs::levenshtein_distance(str_a, str_b); // 레벤슈타인 거리 계산
    assert(dist == 1);

    double sim = mcs::similarity_ratio(str_a, str_b); // 유사도 계산 (0.0 ~ 1.0), sim:0.666..
    assert(std::abs(sim - (2.0 / 3.0)) < 1e-6);
    mcsp::println("[PASS] 레벤슈타인 거리: {}, 유사도: {}%", dist, sim * 100.0);

    assert(mcs::fuzzy_match("app", "apple_banana") == true); // "app"은 "apple_banana"의 서브시퀀스
    assert(mcs::fuzzy_match("xyz", "apple_banana") == false); // "xyz"는 "apple_banana"의 서브시퀀스 아님
    mcsp::println("[PASS] 서브시퀀스 퍼지 매칭 검증 완료");
}

