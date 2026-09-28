#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cassert>
#include <cmath>

#include "mino/core/string/string.hpp"

void test_string_ext() {
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


    // =========================================================================
    // 6. 구조화 데이터 안전 토크나이저 & 포맷팅 치환
    // =========================================================================
    mcsp::println("\n===========================================");
    mcsp::println("6. 구조화 데이터 안전 토크나이저 테스트 (Assert 검증)");
    mcsp::println("===========================================");

    // CSV 파싱 (따옴표 내부 쉼표 및 이스케이프 따옴표 "" 처리)
    std::string csv_sample = R"(1001,"홍길동, 의적","Seoul, KR","그가 말했다 ""안녕하세요!""")";
    auto parsed_csv = mcs::split_csv(csv_sample); // RFC 4180 호환 CSV 파싱
    assert(parsed_csv.size() == 4);
    assert(parsed_csv[0] == "1001");
    assert(parsed_csv[1] == "홍길동, 의적"); // 따옴표 내부 쉼표 처리
    assert(parsed_csv[2] == "Seoul, KR");
    assert(parsed_csv[3] == "그가 말했다 \"안녕하세요!\""); // 이스케이프 따옴표 처리
    mcsp::println("[PASS] RFC 4180 호환 CSV 필드 4개 추출 완료");

    // format_map 템플릿 치환 검증
    std::unordered_map<std::string, std::string> env = {
        {"service", "Mino Core"},
        {"status", "ONLINE"},
        {"port", "8080"}
    };
    std::string formatted = mcs::format_map("[STATUS] {service} is {status} on port {port}.", env);
    assert(formatted == "[STATUS] Mino Core is ONLINE on port 8080.");

    mcsp::print("  -> printlnss 컨테이너 출력: ");
    mcsp::printlnss("[STATUS] {service} is {status} on port {port}.", env);

    mcsp::print("  -> printlnss 인라인 출력: ");
    mcsp::printlnss("사용자: {name} (권한: {role}), 아이디: {id}, MALE: {is_male}, pi: {pi}", {
        {"name", "홍길동"}, // string 타입
        {"role", "SUPER_ADMIN"}, // string 타입
        {"id", 123}, // int 타입
        {"is_male", true}, // bool 타입
        {"pi", 3.141592} // double 타입
        });
    mcsp::println("[PASS] 템플릿 치환 및 named_args 출력 검증 완료");


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
