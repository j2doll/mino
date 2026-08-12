#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>

#include "mino/core/ini/ini_parser.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    namespace mini = mino::core::ini;
    using ini_parser = mini::ini_parser;
    using value_kind = mini::value_kind;
    using entry = mini::entry;
    using section = mini::section;
    using value_kind = mini::value_kind;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    ini_parser parser;
    const std::string test_file = "config_test.ini";

    std::cout
        << to_console_encoding("=== 1. 퍼블릭 세터 함수 및 구조체 테스트 ===")
        << std::endl;

    // 1-1. set_string (Raw 모드: 이스케이프 문자 포함)
    parser.set_string("Server", "host", "127.0.0.1");
    parser.set_string("Server", "path_raw", "C:\\Program Files\\App\nLine2"); // \n 이스케이프 포함

    // 1-2. set_string_literal (Literal 모드: 큰따옴표 이스케이프)
    parser.set_string_literal("Server", "path_literal", "C:\\Program Files\\App\"Quote\"");

    // 1-3. set_bool, set_int, set_double
    parser.set_bool("Server", "enabled", true);
    parser.set_int("Server", "port", 8080);
    parser.set_double("Server", "timeout", 15.5);

    parser.set_string("Database", "db_name", "test_db");
    parser.set_int("Database", "max_connections", 100);
    parser.set_bool("Database", "use_ssl", false);

    // 1-4. save 호출
    std::error_code ec;
    std::filesystem::remove(test_file, ec); // 기존 파일 삭제
    if (ec) {
        std::cerr
            << to_console_encoding("파일 삭제 실패: ")
            << ec.message() << std::endl;
        return 1;
    }
    bool saved = parser.save(test_file);
    std::cout
        << to_console_encoding("save(): ")
        << to_console_encoding((saved ? "성공" : "실패"))
        << std::endl;
    assert(saved);

    std::cout
        << to_console_encoding("\n=== 2. 퍼블릭 조회 및 검증 함수 테스트 ===")
        << std::endl;

    // 2-1. load 호출
    ini_parser loaded_parser;
    bool loaded = loaded_parser.load(test_file);
    std::cout
        << to_console_encoding("load(): ")
        << to_console_encoding((loaded ? "성공" : "실패"))
        << std::endl;
    assert(loaded);

    // 2-1-1. 모든 섹션과 엔트리 출력
    std::cout
        << std::endl
        << to_console_encoding("모든 섹션과 엔트리 출력")
        << std::endl;
    auto all = loaded_parser.all_sections();
    for (const auto& sec_pair : all) {
        const std::string& section_name = sec_pair.first;
        const auto& entries = sec_pair.second;

        std::cout << "[" << to_console_encoding(section_name) << "]\n";
        for (const auto& e : entries) {
            std::cout
                << "'" << to_console_encoding(e.key) << "' = '"
                << to_console_encoding(e.value) << "'";

            std::cout << "  (kind=";
            switch (e.kind) {
                case value_kind::String: std::cout << "String"; break;
                case value_kind::Bool:   std::cout << "Bool";   break;
                case value_kind::Int:    std::cout << "Int";    break;
                case value_kind::Double: std::cout << "Double"; break;
            }
            std::cout << (e.string_literal ? ", literal" : ", raw") << ")" << std::endl;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // 2-2. section_names 호출
    std::vector<std::string> sections = loaded_parser.section_names();
    std::cout << to_console_encoding("섹션 목록:") << std::endl;
    for (const auto& sec_name : sections) {
        std::cout << "  [" << to_console_encoding(sec_name) << "]" << std::endl;
    }

    // 2-3. has_section, has 호출
    std::cout
        << to_console_encoding("\n존재 여부 확인:") << std::endl;
    std::cout
        << to_console_encoding("has_section('Server'): ")
        << std::boolalpha << loaded_parser.has_section("Server") << std::endl;
    std::cout
        << to_console_encoding("has_section('Client'): ")
        << std::boolalpha << loaded_parser.has_section("Client") << std::endl;
    std::cout
        << to_console_encoding("has('Server', 'port'): ")
        << std::boolalpha << loaded_parser.has("Server", "port") << std::endl;
    std::cout
        << to_console_encoding("has('Server', 'unknown'): ")
        << std::boolalpha << loaded_parser.has("Server", "unknown") << std::endl;

    // 2-4. get_string, get_bool, get_int, get_double 호출
    std::cout
        << to_console_encoding("\n게터 함수 확인:")
        << std::endl;
    if (auto host = loaded_parser.get_string("Server", "host")) {
        std::cout
            << to_console_encoding("Server.host: ")
            << to_console_encoding(*host) << std::endl;
    }
    if (auto raw_path = loaded_parser.get_string("Server", "path_raw")) {
        std::cout
            << to_console_encoding("Server.path_raw: ")
            << to_console_encoding(*raw_path)
            << std::endl;
    }
    if (auto lit_path = loaded_parser.get_string("Server", "path_literal")) {
        std::cout
            << to_console_encoding("Server.path_literal: ")
            << to_console_encoding(*lit_path)
            << std::endl;
    }
    if (auto enabled = loaded_parser.get_bool("Server", "enabled")) {
        std::cout
            << to_console_encoding("Server.enabled: ")
            << std::boolalpha << *enabled
            << std::endl;
    }
    if (auto port = loaded_parser.get_int("Server", "port")) {
        std::cout
            << to_console_encoding("Server.port: ")
            << *port
            << std::endl;
    }
    if (auto timeout = loaded_parser.get_double("Server", "timeout")) {
        std::cout
            << to_console_encoding("Server.timeout: ")
            << *timeout
            << std::endl;
    }

    std::cout << to_console_encoding("\n=== 3. 공개 타입(value_kind, entry, section) 직접 접근/사용 ===") << std::endl;

    // value_kind 열거형 사용 예시
    value_kind kind_str = value_kind::String;
    value_kind kind_bool = value_kind::Bool;
    value_kind kind_int = value_kind::Int;
    value_kind kind_dbl = value_kind::Double;

    // entry 및 section 구조체 직접 생성 및 검증
    entry sample_entry{ "my_key", "my_val", kind_str, true };
    section sample_section;
    sample_section.entries.push_back(sample_entry);
    sample_section.index["my_key"] = 0;

    std::cout
        << to_console_encoding("직접 생성한 entry key: ")
        << to_console_encoding(sample_section.entries[0].key)
        << to_console_encoding(", value: ")
        << to_console_encoding(sample_section.entries[0].value)
        << to_console_encoding(", literal: ")
        << std::boolalpha << sample_section.entries[0].string_literal
        << std::endl;

    std::cout << to_console_encoding("\n=== 4. 삭제 함수 (remove, remove_section) 테스트 ===") << std::endl;
    // 4-1. remove 호출 (단일 키 삭제)
    bool removed_key = loaded_parser.remove("Server", "timeout");
    std::cout
        << to_console_encoding("remove('Server', 'timeout'): ")
        << to_console_encoding((removed_key ? "성공" : "실패"))
        << std::endl;
    std::cout
        << to_console_encoding("삭제 후 has('Server', 'timeout'): ")
        << std::boolalpha
        << loaded_parser.has("Server", "timeout")
        << std::endl;

    // 4-2. remove_section 호출 (섹션 전체 삭제)
    bool removed_sec = loaded_parser.remove_section("Database");
    std::cout
        << to_console_encoding("remove_section('Database'): ")
        << to_console_encoding((removed_sec ? "성공" : "실패"))
        << std::endl;
    std::cout
        << to_console_encoding("삭제 후 has_section('Database'): ")
        << std::boolalpha
        << loaded_parser.has_section("Database") 
        << std::endl;

    // 최종 상태 저장
    loaded_parser.save("config_test_modified.ini");

    std::cout
        << to_console_encoding("\n모든 퍼블릭 멤버 기능 테스트 완료.")
        << std::endl;
    return 0;
}
