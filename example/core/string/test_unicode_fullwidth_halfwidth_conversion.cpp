#include "test_common.hpp"

void test_unicode_fullwidth_halfwidth_conversion() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // =========================================================================
    // 2. 유니코드 및 전각/반각 변환
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("2. 유니코드 및 전각/반각 변환 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    std::string ascii_str = "ABC 123 !?";
    std::string full_str = mcs::to_full_width(ascii_str); // 전각 "ＡＢＣ　１２３　！？"
    std::string half_str = mcs::to_half_width(full_str); // 반각 "ABC 123 !?"
    assert(half_str == ascii_str);
    mcsp::println("[PASS] 전각 변환 및 반각 복원: [{}] -> [{}]", full_str, half_str);

    // Grapheme 글자 수 및 안전 자르기 검증
    assert(mcs::utf8_grapheme_count("대한민국") == 4);

    std::string korean_sentence = "대한민국 만세! 무궁화 삼천리";
    std::string ellipsized = mcs::ellipsize_grapheme_safe(korean_sentence, 6);
    assert(ellipsized == "대한민국 만...");
    mcsp::println("[PASS] Grapheme 6글자 안전 자르기: {}", ellipsized);


}
