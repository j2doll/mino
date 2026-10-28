#include "catch_amalgamated.hpp"
#include "mino/core/config/config.hpp"

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

using namespace mino::core::config;
namespace fs = std::filesystem;

namespace {
    // 테스트 종료 시 임시 파일을 자동으로 정리하는 RAII 헬퍼
    struct temp_file_guard {
        fs::path path;

        explicit temp_file_guard(const std::string& filename, const std::string& content = "")
            : path(fs::temp_directory_path() / filename) {
            std::ofstream ofs(path);
            ofs << content;
        }

        ~temp_file_guard() {
            std::error_code ec;
            fs::remove(path, ec);
        }

        std::string string() const {
            return path.string();
        }
    };
}

TEST_CASE("config_manager 파일 로드 및 타입별 파싱 테스트", "[config][config_manager]") {
    const std::string sample_content =
        "# 서버 설정 파일\n"
        "\n"
        "host = 127.0.0.1\n"
        "port = 8080\n"
        "enabled = true\n"
        "ratio = 3.1415\n"
        "invalid_int = not_a_number\n";

    temp_file_guard temp_file("test_config_load.ini", sample_content);
    auto& mgr = config_manager::get_instance();

    REQUIRE(mgr.load(temp_file.string()));

    SECTION("문자열(string) 파싱 및 get_string_or") {
        auto host = mgr.get_string("host");
        REQUIRE(host.has_value());
        CHECK(*host == "127.0.0.1");

        CHECK(mgr.get_string_or("host", "default") == "127.0.0.1");
        CHECK(mgr.get_string_or("non_existent", "default_val") == "default_val");
        CHECK_FALSE(mgr.get_string("non_existent").has_value());
    }

    SECTION("정수(int) 파싱 및 기본값 확인") {
        auto port = mgr.get_int("port");
        REQUIRE(port.has_value());
        CHECK(*port == 8080);

        CHECK(mgr.get_int("port", 0) == 8080);
        CHECK(mgr.get_int_or("port", 0) == 8080);

        // 존재하지 않는 키 및 잘못된 형식
        CHECK_FALSE(mgr.get_int("non_existent").has_value());
        CHECK(mgr.get_int("non_existent", 9999) == 9999);
        CHECK_FALSE(mgr.get_int("invalid_int").has_value());
        CHECK(mgr.get_int_or("invalid_int", 5000) == 5000);
    }

    SECTION("불리언(bool) 다양한 포맷 파싱") {
        CHECK(mgr.get_bool("enabled") == true);
        CHECK(mgr.get_bool_or("enabled", false) == true);
        CHECK_FALSE(mgr.get_bool("non_existent").has_value());
        CHECK(mgr.get_bool("non_existent", false) == false);
    }

    SECTION("실수(double) 파싱 및 get_double_or") {
        auto ratio = mgr.get_double("ratio");
        REQUIRE(ratio.has_value());
        CHECK(Catch::Approx(*ratio).epsilon(0.0001) == 3.1415);

        CHECK(Catch::Approx(mgr.get_double_or("ratio", 0.0)).epsilon(0.0001) == 3.1415);
        CHECK(mgr.get_double_or("non_existent", 1.23) == 1.23);
    }
}

TEST_CASE("불리언 문자열 표현식 평가 테스트", "[config][config_manager]") {
    const std::string bool_content =
        "t1 = 1\n"
        "t2 = yes\n"
        "t3 = on\n"
        "f1 = false\n"
        "f2 = 0\n"
        "f3 = no\n"
        "f4 = off\n"
        "invalid = random_text\n";

    temp_file_guard temp_file("test_config_bool.ini", bool_content);
    auto& mgr = config_manager::get_instance();
    REQUIRE(mgr.load(temp_file.string()));

    CHECK(mgr.get_bool("t1") == true);
    CHECK(mgr.get_bool("t2") == true);
    CHECK(mgr.get_bool("t3") == true);

    CHECK(mgr.get_bool("f1") == false);
    CHECK(mgr.get_bool("f2") == false);
    CHECK(mgr.get_bool("f3") == false);
    CHECK(mgr.get_bool("f4") == false);

    CHECK_FALSE(mgr.get_bool("invalid").has_value());
}

TEST_CASE("따옴표 및 이스케이프(\", \\) 파싱 테스트", "[config][config_manager]") {
    const std::string escape_content =
        "quoted_str = \"Hello, World!\"\n"
        "escaped_quote = \"He said \\\"Hello\\\"\"\n"
        "escaped_slash = \"Path\\\\to\\\\file\"\n"
        "no_quote = Just Plain Text\n";

    temp_file_guard temp_file("test_config_escape.ini", escape_content);
    auto& mgr = config_manager::get_instance();
    REQUIRE(mgr.load(temp_file.string()));

    CHECK(mgr.get_string("quoted_str") == "Hello, World!");
    CHECK(mgr.get_string("escaped_quote") == "He said \"Hello\"");
    CHECK(mgr.get_string("escaped_slash") == "Path\\to\\file");
    CHECK(mgr.get_string("no_quote") == "Just Plain Text");
}

TEST_CASE("설정 값 변경(set) 및 주석 보존 저장(save) 테스트", "[config][config_manager]") {
    const std::string original_content =
        "# Header Comment\n"
        "app_name = MinoApp\n"
        "\n"
        "# Network Section\n"
        "port = 8080\n";

    temp_file_guard src_file("test_config_src.ini", original_content);
    temp_file_guard dst_file("test_config_dst.ini");

    auto& mgr = config_manager::get_instance();
    REQUIRE(mgr.load(src_file.string()));

    SECTION("기존 값 수정 및 신규 값 추가 후 저장") {
        // 기존 키 값 갱신
        mgr.set("port", "9090");
        // 새 키 추가
        mgr.set("max_connections", "100");

        CHECK(mgr.get_int("port") == 9090);
        CHECK(mgr.get_int("max_connections") == 100);

        REQUIRE(mgr.save(dst_file.string()));

        // 저장된 파일 다시 읽어서 주석 및 변경 내용 보존 여부 확인
        auto& reload_mgr = config_manager::get_instance();
        REQUIRE(reload_mgr.load(dst_file.string()));

        CHECK(reload_mgr.get_string("app_name") == "MinoApp");
        CHECK(reload_mgr.get_int("port") == 9090);
        CHECK(reload_mgr.get_int("max_connections") == 100);

        const auto all_lines = reload_mgr.get_all();
        // # Header Comment, app_name, 빈 줄, # Network Section, port, max_connections 총 6줄
        REQUIRE(all_lines.size() == 6);
        CHECK_FALSE(all_lines[0].is_data); // # Header Comment
        CHECK(all_lines[0].raw_text == "# Header Comment");
        CHECK(all_lines[1].is_data);       // app_name
        CHECK_FALSE(all_lines[2].is_data); // 빈 줄
        CHECK_FALSE(all_lines[3].is_data); // # Network Section
        CHECK(all_lines[4].is_data);       // port
        CHECK(all_lines[5].is_data);       // max_connections
    }
}

TEST_CASE("기본 파일 경로(set_config_file) 기반 load 및 save 테스트", "[config][config_manager]") {
    const std::string content = "env = production\n";
    temp_file_guard default_file("test_default_config.ini", content);

    auto& mgr = config_manager::get_instance();
    mgr.set_config_file(default_file.string());

    // 인자 없는 load()
    REQUIRE(mgr.load());
    CHECK(mgr.get_string("env") == "production");

    mgr.set("env", "staging");

    // 인자 없는 save()
    REQUIRE(mgr.save());

    // 재확인
    REQUIRE(mgr.load());
    CHECK(mgr.get_string("env") == "staging");
}

TEST_CASE("존재하지 않는 파일 로드 에러 처리", "[config][config_manager]") {
    auto& mgr = config_manager::get_instance();
    CHECK_FALSE(mgr.load("non_existent_path_to_file_12345.ini"));
}
