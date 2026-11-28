#include "test_common.hpp"

void test_structured_data_safe_tokenizer_formatting_replace() {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

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

}
