#include <iostream>
#include <cassert>
#include <string>

#include "mino/core/string/string.hpp"

// mino external yaml 
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <yaml-cpp/yaml.h>
#include "mino/external/yaml/yaml.hpp"

// 출력 유틸리티 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

void test_yaml_wrapper_basic();
void test_yaml_wrapper_conversions();
void test_yaml_wrapper_move_semantics();
void test_yaml_handler_direct();  

int main() {
    try {
        test_yaml_wrapper_basic();
        test_yaml_handler_direct(); // 추가 호출
        test_yaml_wrapper_conversions();
        test_yaml_wrapper_move_semantics();

        print(endl, tce("========================================"));
        print(tce(" 모든 테스트를 성공적으로 통과했습니다!"));
        print(tce("========================================"));
    }
    catch (const std::exception& e) {
        eprint(tce("테스트 중 예외 발생: "), e.what());
        return 1;
    }

    return 0;
}

void test_yaml_wrapper_basic() {
    print(tce("========================================"));
    print(tce("[Test 1] Basic YAML Read / Write Test"));
    print(tce("========================================"));

    using yaml_wrapper = mino::external::yml::yaml_wrapper;

    yaml_wrapper yaml;

    // spdlog 로거 설정
    auto console_logger = spdlog::stdout_color_mt("test_logger");
    yaml.set_logger(console_logger);

    // 샘플 YAML 문자열
    std::string sample_yaml = R"(
server:
    host: "127.0.0.1"
    port: 8080
    timeout: 3.5
    enabled: true
)";
     
    // 문자열로부터 파싱
    bool loaded = yaml.load_from_string(sample_yaml); // YAML 문자열 로드
    assert(loaded && "YAML 문자열 로드에 실패했습니다.");
    print(tce(">> load_from_string 성공"));

    // 값 읽기/쓰기 테스트 (기본 타입)
    int port = 0;
    double timeout = 0.0;
    std::string host;
    bool enabled = false;

    // 값 읽기 테스트
    assert(yaml.get_string("server.host", host) && host == "127.0.0.1");
    assert(yaml.get_int("server.port", port) && port == 8080);
    assert(yaml.get_double("server.timeout", timeout) && timeout == 3.5);
    assert(yaml.get_bool("server.enabled", enabled) && enabled == true);

    // 값 쓰기 테스트
    yaml.set_string("service_name", "OrderService"); // 필드 추가 
    yaml.set_int("port", 7070); // 필드 추가
    yaml.set_double("version", 1.5); // 필드 추가
    yaml.set_bool("is_active", true); // 필드 추가

    // --- 디버그 출력: 현재 YAML 내용 확인 ---
    auto serialized_opt = yaml.save_to_string();
    if (serialized_opt.has_value()) {
        print(tce(">> YAML after set_*:\n"), *serialized_opt);
    } else {
        print(tce(">> YAML serialization failed"));
    }

    // --- 디버그 출력: top-level 'port' 과 nested 'server.port' 확인 ---
    int port_top = 0;
    bool ok_top = yaml.get_int("port", port_top);
    print(
        tce(">> get_int(\"port\") returned: "),
        (ok_top ? "true" : "false"),
        (ok_top ? (std::string(" value: ") + std::to_string(port_top)) : std::string(""))
    );

    int port_nested = 0;
    bool ok_nested = yaml.get_int("server.port", port_nested);
    print(
        tce(">> get_int(\"server.port\") returned: "),
        (ok_nested ? "true" : "false"),
        (ok_nested ? (std::string(" value: ") + std::to_string(port_nested)) : std::string(""))
    );

    // 기존 검증 (원래대로 유지: assert 실패 재현 시 위 디버그 출력 참조)
    assert(yaml.get_string("service_name", host) && host == "OrderService");
    assert(yaml.get_int("port", port) && port == 7070);
    assert(yaml.get_double("version", timeout) && timeout == 1.5);
    assert(yaml.get_bool("is_active", enabled) && enabled == true);

    print(tce(">> Key 'service_name': "), host);
    print(tce(">> Key 'port': "), port);
    print(tce(">> Key 'version': "), timeout);
    print(tce(">> Key 'is_active': "), std::boolalpha, enabled);

    // 값 읽기 테스트 (재수행)
    assert(yaml.get_string("server.host", host) && host == "127.0.0.1");
    assert(yaml.get_int("server.port", port) && port == 8080);
    assert(yaml.get_double("server.timeout", timeout) && timeout == 3.5);
    assert(yaml.get_bool("server.enabled", enabled) && enabled == true);


    // 문자열 직렬화 출력
    auto serialized = yaml.save_to_string();
    assert(serialized.has_value());
    print(endl, tce("[직렬화된 YAML 결과]"), endl, *serialized);
    // server:
    //   host: 127.0.0.1
    //   port: 8080
    //   timeout: 3.5
    //   enabled: true
    // service_name: OrderService
    // port: 7070
    // version: 1.5
    // is_active: true

}

void test_yaml_handler_direct() {
    print(tce("========================================"));
    print(tce("[Test - yaml_handler 직접] dotted key 및 존재하지 않는 키 검사"));
    print(tce("========================================"));

    using yaml_handler = mino::external::yml::yaml_handler;

    yaml_handler handler;
    auto logger = spdlog::stdout_color_mt("handler_test_logger");
    handler.set_logger(logger);

    std::string sample_yaml = R"(
server:
    host: "127.0.0.1"
    port: 8080
)";

    bool loaded = handler.load_from_string(sample_yaml);
    assert(loaded && "yaml_handler load_from_string 실패");
    print(tce(">> yaml_handler load_from_string 성공"));

    // dotted-path로 값 가져오기 (예: "server.host")
    auto host_opt = handler.get_value<std::string>("server.host");
    assert(host_opt.has_value() && *host_opt == "127.0.0.1");
    print(tce(">> handler.get_value(\"server.host\") : "), *host_opt);

    auto port_opt = handler.get_value<int>("server.port");
    assert(port_opt.has_value() && *port_opt == 8080);
    print(tce(">> handler.get_value(\"server.port\") : "), *port_opt);

    // 존재하지 않는 키는 nullopt 반환
    auto missing_opt = handler.get_value<std::string>("server.missing");
    assert(!missing_opt.has_value());
    print(tce(">> handler.get_value(\"server.missing\") : nullopt 확인"));
}

void test_yaml_wrapper_conversions() {
    print(tce("========================================"));
    print(tce("[Test 2] JSON / XML 변환 및 블록 스칼라 테스트"));
    print(tce("========================================"));

    using yaml_wrapper = mino::external::yml::yaml_wrapper;

    yaml_wrapper yaml;

    yaml.set_string("title", "Mino Framework");
    yaml.set_int("build_number", 42);
    yaml.set_double("pi", 3.141592);
    yaml.set_bool("production", false);

    // 블록 스칼라(Literal text) 설정
    yaml.set_block_scalar("description", "Line 1: Hello Mino\nLine 2: YAML wrapper test\nLine 3: Finished!", true);

    // 1. JSON 변환 테스트
    auto json_opt = yaml.save_as_json_string();
    assert(json_opt.has_value());
    print(endl, tce("[JSON 변환 출력]:"), endl, *json_opt);

    // 2. XML 변환 테스트
    auto xml_opt = yaml.save_as_xml_string("configuration");
    assert(xml_opt.has_value());
    print(endl, tce("[XML 변환 출력]:"), endl, *xml_opt);

    // 3. 파일 저장 & 로드 테스트
    const std::string test_yaml_file = "test_output.yml";
    const std::string test_json_file = "test_output.json";
    const std::string test_xml_file = "test_output.xml";

    assert(yaml.save_to_file(test_yaml_file));
    assert(yaml.save_as_json_file(test_json_file));
    assert(yaml.save_as_xml_file(test_xml_file, "app_config"));

    print(tce(">> YAML/JSON/XML 파일 저장 완료"));

    // 파일로부터 재로드 검증
    mino::external::yml::yaml_wrapper reloaded_yaml;
    assert(reloaded_yaml.load_from_file(test_yaml_file));

    std::string title_val;
    assert(reloaded_yaml.get_string("title", title_val) && title_val == "Mino Framework");
    print(tce(">> 파일로부터 재로드 및 값 검증 성공: "), title_val);
}

void test_yaml_wrapper_move_semantics() {
    print(endl, tce("========================================"));
    print(tce("[Test 3] Pimpl 이동 시맨틱(Move Semantics) 테스트"));
    print(tce("========================================"));

    using yaml_wrapper = mino::external::yml::yaml_wrapper;

    yaml_wrapper src;
    src.set_string("owner", "Developer");

    // 이동 생성
    yaml_wrapper dest = std::move(src);
    std::string owner_val;
    assert(dest.get_string("owner", owner_val) && owner_val == "Developer");
    print(tce(">> 이동 생성자 검증 성공 (owner: "), owner_val, ")");
}

