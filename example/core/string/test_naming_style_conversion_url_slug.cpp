#include "test_common.hpp"

void test_naming_style_conversion_url_slug() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    // =========================================================================
    // 3. 네이밍 스타일 변환 & URL 슬러그
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("3. 케이스 스타일 변환 & URL 슬러그 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    std::string ident = "created_user_id";
    assert(mcs::to_camel_case(ident) == "createdUserId");
    assert(mcs::to_pascal_case(ident) == "CreatedUserId");
    assert(mcs::to_kebab_case(ident) == "created-user-id");
    assert(mcs::to_snake_case("createdUserId") == "created_user_id");
    mcsp::println("[PASS] 식별자 케이스 상호 변환 (camel, pascal, kebab, snake)");

    std::string raw_title = "2026년 최신 C++17 아키텍처 가이드!!";
    std::string slug = mcs::to_slug(raw_title);
    assert(slug == "2026년-최신-c-17-아키텍처-가이드");
    mcsp::println("[PASS] URL Slug 생성: {}", slug);
}
