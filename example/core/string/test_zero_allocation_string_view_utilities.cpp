#include "test_common.hpp"

void test_zero_allocation_string_view_utilities() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // =========================================================================
    // 5. 제로-할당 string_view 기반 유틸리티
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("5. 제로-할당 string_view 기반 뷰 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    std::string_view raw_csv = "   apple , banana , orange , melon   ";
    assert(mcs::trim_view(raw_csv) == "apple , banana , orange , melon");

    auto tokens = mcs::split_view(raw_csv, ','); // ','를 기준으로 분할
    assert(tokens.size() == 4);
    assert(mcs::trim_view(tokens[0]) == "apple");
    assert(mcs::trim_view(tokens[1]) == "banana");
    assert(mcs::trim_view(tokens[2]) == "orange");
    assert(mcs::trim_view(tokens[3]) == "melon");
    mcsp::println("[PASS] split_view 및 trim_view 분할/공백제거 토큰 4개 검증 완료");

}
