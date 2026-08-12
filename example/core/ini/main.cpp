#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cassert>

#include "mino/core/ini/ini_parser.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    namespace mini = mino::core::ini;
    using ini_parser = mini::ini_parser;
    using value_kind = mini::value_kind;
    using entry = mini::entry;
    using section = mini::section;

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

    // --- 1.1 주석 보존 예제 파일 생성 (leading + inline comments) ---
    const std::string comment_file = "config_with_comments.ini";
    {
        std::ofstream ofs(comment_file, std::ios::trunc); 
        ofs << "; Global comment: configuration for demo\n"; // 주석
        ofs << "# Another global comment line\n"; // 주석
        ofs << "\n";
        ofs << "[Server]\n";
        ofs << "; Server section leading comment\n"; // 주석
        ofs << "host='127.0.0.1' ; inline comment for host\n"; // 엔트리와 인라인 주석
        ofs << "path=\"C:\\\\Program Files\\\\App\\\"Quote\\\"\" # inline with # delimiter\n"; // 엔트리와 인라인 주석
        ofs << "enabled=true\n";
        ofs << "\n";
        ofs << "[Database]\n";
        ofs << "; DB leading comment\n"; // 주석
        ofs << "db_name='test_db' ; important DB comment\n"; // 엔트리와 인라인 주석
        ofs.close();
    }

    std::cout
        << to_console_encoding("\n=== 1.1 주석 보존(load/save) 테스트 ===")
        << std::endl;

    // load comment file and print preserved comments
    ini_parser comment_parser;
    bool loaded_comments = comment_parser.load(comment_file);
    std::cout
        << to_console_encoding("load(config_with_comments.ini): ")
        << to_console_encoding((loaded_comments ? "성공" : "실패"))
        << std::endl;
    assert(loaded_comments);

    auto secs = comment_parser.all_sections();
    for (const auto& sec_pair : secs) {
        const std::string& sec_name = sec_pair.first;
        const auto& entries = sec_pair.second;

        // print section leading comments if any
        std::cout << to_console_encoding("\n[Section] ") << to_console_encoding(sec_name) << std::endl;
        // find section object to access leading_comments (use parser.section_names + entries for printed data)
        // we already have entries vector; print any leading comments attached to first entry or section via API:
        // (entry.leading_comments and section.leading_comments were set by modified parser on load)
        // print leading comments stored at section (via re-fetching section by name)
        const section* sec_obj = nullptr;
        // Because all_sections returns copies, use comment_parser.section_names() + comment_parser.entries(...)
        // to access section leading comments we need to fetch entries and check entry.leading_comments.
        // Print section-level leading comments by checking entries of this section and first entry's leading_comments if present.
        if (!entries.empty() && !entries.front().leading_comments.empty()) {
            for (const auto& c : entries.front().leading_comments) {
                std::cout << to_console_encoding("  [leading comment] ") << to_console_encoding(c) << std::endl;
            }
        }

        for (const auto& e : entries) {
            std::cout
                << to_console_encoding("  key: ") << to_console_encoding(e.key)
                << to_console_encoding("  value: ") << to_console_encoding(e.value)
                << std::endl;
            // print entry-level leading comments
            for (const auto& lc : e.leading_comments) {
                std::cout << to_console_encoding("    entry leading comment: ") << to_console_encoding(lc) << std::endl;
            }
            // print inline comment if present
            if (!e.inline_comment.empty()) {
                std::cout << to_console_encoding("    inline comment: ") << to_console_encoding(e.inline_comment) << std::endl;
            }
        }
    }

    // Save round-trip to verify comments are written back
    const std::string comment_out = "config_with_comments_out.ini";
    bool saved_comments = comment_parser.save(comment_out);
    std::cout
        << to_console_encoding("\nsave(config_with_comments_out.ini): ")
        << to_console_encoding((saved_comments ? "성공" : "실패"))
        << std::endl;
    assert(saved_comments);

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
    auto all = loaded_parser.all_sections(); // 모든 섹션과 엔트리 복사 반환
    for (const auto& sec_pair : all) {
        const std::string& section_name = sec_pair.first;
        const auto& entries = sec_pair.second;

        // 섹션 출력
        std::cout << "[" << to_console_encoding(section_name) << "]\n";

        for (const auto& e : entries) {
            // 엔트리 출력
            std::cout
                << "'" << to_console_encoding(e.key) << "' = '"
                << to_console_encoding(e.value) << "'";

            // 엔트리 종류 및 문자열 모드 출력
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

    // === 5. sample.ini 읽기 테스트 (모든 섹션과 엔트리 출력) ===
    std::cout << to_console_encoding("\n=== 5. sample.ini 로드 및 전체 출력 테스트 ===") << std::endl;
    const std::string sample_file = "sample.ini";
    ini_parser sample_parser;
    bool sample_loaded = sample_parser.load(sample_file);
    std::cout
        << to_console_encoding("load(sample.ini): ")
        << to_console_encoding((sample_loaded ? "성공" : "실패"))
        << std::endl;

    if (sample_loaded) {
        auto sample_all = sample_parser.all_sections();
        for (const auto& sec_pair : sample_all) {
            const std::string& section_name = sec_pair.first;
            const auto& entries = sec_pair.second;

            // print section header
            std::cout << std::endl << "[" << to_console_encoding(section_name) << "]" << std::endl;

            for (const auto& e : entries) {
                std::cout
                    << "  '" << to_console_encoding(e.key) << "' = '"
                    << to_console_encoding(e.value) << "'";

                std::cout << "  (kind=";
                switch (e.kind) {
                    case value_kind::String: std::cout << "String"; break;
                    case value_kind::Bool:   std::cout << "Bool";   break;
                    case value_kind::Int:    std::cout << "Int";    break;
                    case value_kind::Double: std::cout << "Double"; break;
                }
                std::cout << (e.string_literal ? ", literal" : ", raw") << ")";

                if (!e.inline_comment.empty()) {
                    std::cout << "  ; " << to_console_encoding(e.inline_comment);
                }
                std::cout << std::endl;

                // print leading comments associated with this entry (if any)
                if (!e.leading_comments.empty()) {
                    for (const auto& lc : e.leading_comments) {
                        std::cout << to_console_encoding("    [leading comment] ") << to_console_encoding(lc) << std::endl;
                    }
                }
            }
        }
    }

    std::cout
        << to_console_encoding("\n모든 퍼블릭 멤버 기능 테스트 완료.")
        << std::endl;
    return 0;
}
