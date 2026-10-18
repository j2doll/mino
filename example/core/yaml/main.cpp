#include <filesystem>
#include <string>
#include <string_view>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/yaml/yaml.hpp"

int main(int argc, char* argv[]) {
    namespace mcy = mino::core::yaml;
    namespace fs = std::filesystem;
    namespace mcsp = mino::core::string::print;
    auto tce = mino::core::string::to_console_encoding;

    fs::path config_dir = fs::current_path() / "config_test";
    fs::path input_file = config_dir / "app_settings.yaml";
    fs::path output_file = config_dir / "app_settings_updated.yaml";

    // 0. 테스트 디렉토리 생성
    std::error_code ec;
    fs::create_directories(config_dir, ec);
    if (ec) {
        mcsp::println("Failed to create dir: {}\n", ec.message());
        return 1;
    }

    // 테스트용 샘플 파일 생성
    {
        std::string_view sample_content = R"(
app:
  name: "FileSystem App UTF8한글"
  version: "1.0.1"
  active: true
storage:
  path: "/var/data"
  max_capacity_gb: 500.5
maintainers:
  - "alice@example.com"
  - "bob@example.com"
)";

        mcy::node init_doc = mcy::parse_yaml(sample_content);
        if (!mcy::dump_yaml_file(init_doc, input_file)) {
            auto input_yaml_string = input_file.string();
            mcsp::println("초기 파일 저장 실패: {}\n", input_yaml_string);
            return 1;
        }
        auto input_yaml_string = input_file.string();
        mcsp::println("[파일 생성 완료]: {}\n\n", input_yaml_string);
    }

    // ---------------------------------------------------------
    // 1. std::filesystem::path 로 파일 읽기
    // ---------------------------------------------------------
    mcsp::println("========================================\n");
    mcsp::println(" 1. std::filesystem::path 로 파일 읽기\n");
    mcsp::println("========================================\n");

    auto opt_doc = mcy::parse_yaml_file(input_file);
    if (opt_doc.has_value()) {
        mcy::node& doc = *opt_doc;

        auto app_name_node = doc["app"]["name"];
        assert(app_name_node.is_string());
        auto app_name_string = app_name_node.as<std::string>();
        mcsp::println("App Name     : {}\n", app_name_string);

        auto version_node = doc["app"]["version"];
        assert(version_node.is_string());
        auto version_value = version_node.as<std::string>();
        mcsp::println("Version      : {}\n", version_value);

        auto active_node = doc["app"]["active"];
        auto active_bool = active_node.as<bool>();
        mcsp::println("Active       : {}\n", active_bool ? "true" : "false");

        auto storage_path_node = doc["storage"]["path"];
        auto storage_path_string = storage_path_node.as<std::string>();
        mcsp::println("Storage Path : {}\n", storage_path_string);
        auto max_capacity_node = doc["storage"]["max_capacity_gb"];
        auto max_capacity_double = max_capacity_node.as<double>();
        mcsp::println("Max Capacity : {} GB\n", max_capacity_double);

        auto maintainers_node = doc["maintainers"];
        auto first_maintainer = maintainers_node[0].as<std::string>();
        mcsp::println("Maintainer 0 : {}\n", first_maintainer);
        // ---------------------------------------------------------
        // 2. 데이터 수정 및 파일 쓰기
        // ---------------------------------------------------------
        mcsp::println("\n========================================\n");
        mcsp::println(" 2. 데이터 수정 후 새로운 파일로 저장\n");
        mcsp::println("========================================\n");

        doc["app"]["version"] = mcy::node{ "2.0.1" };
        doc["maintainers"].push_back(mcy::node{ std::string("charlie@example.com") });

        if (mcy::dump_yaml_file(doc, output_file)) {
            mcsp::println("[저장 완료]: {}\n", output_file.string());
        }

        // 저장된 파일 다시 읽어서 확인
        auto opt_reloaded_doc = mcy::parse_yaml_file(output_file);
        if (opt_reloaded_doc.has_value()) {
            mcy::node& reloaded_doc = *opt_reloaded_doc;
            mcsp::println("[새 파일에서 읽은 버전]: {}\n", reloaded_doc["app"]["version"].as<std::string>());
            mcsp::println("[새 파일의 유지관리자 수]: {}\n", reloaded_doc["maintainers"].size());
        }
    }
    else {
        mcsp::println("파일 읽기 실패: {}\n", input_file.string());
    }

    // ---------------------------------------------------------
    // 3. 파일 관련 에러 처리 테스트 (존재하지 않는 파일)
    // ---------------------------------------------------------
    mcsp::println("\n========================================\n");
    mcsp::println(" 3. 존재하지 않는 파일 읽기 시도 예외 처리\n");
    mcsp::println("========================================\n");

    fs::path non_existing_path = config_dir / "does_not_exist.yaml";
    auto missing = mcy::parse_yaml_file(non_existing_path);
    if (!missing.has_value()) {
        mcsp::println("-> 파일이 존재하지 않아 안전하게 nullopt 반환 성공\n");
    }

    // 테스트 생성 파일 정리
    fs::remove_all(config_dir, ec);
    if (ec) {
        mcsp::println("Failed to remove dir: {}\n", ec.message());
        return 1;
    }
    mcsp::println("\n[테스트 임시 디렉토리 정리 완료]\n");

    return 0;
}
