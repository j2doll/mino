#include "catch_amalgamated.hpp"
#include "mino/core/cli/arg_parser.hpp"

#include <vector>
#include <string>
#include <cmath>
#include <limits>

using namespace mino::core::cli;

// std::vector<std::string>을 받아 C 스타일의 argc, argv로 변환하여 parse를 실행해주는 헬퍼 함수
static bool parse_args(arg_parser& parser, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    return parser.parse(static_cast<int>(argv.size()), argv.data());
}

TEST_CASE("기본 플래그 파싱 및 도움말 옵션 테스트", "[cli][arg_parser]") {
    arg_parser parser("CLI Unit Test Program");

    parser.add_option("-v", "--verbose", "상세 출력 활성화", false);
    parser.add_option("-d", "--debug", "디버그 모드", false);

    SECTION("기본 내장된 --help 및 -h 플래그 동작 확인") {
        std::vector<std::string> args = { "app", "-h" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.has_flag("--help"));
    }

    SECTION("개별 플래그 및 다중 플래그 확인") {
        std::vector<std::string> args = { "app", "--verbose", "-d" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.has_flag("--verbose"));
        CHECK(parser.has_flag("--debug"));
        CHECK_FALSE(parser.has_flag("--help"));
    }

    SECTION("단축 옵션 결합 플래그 (예: -vd) 확인") {
        std::vector<std::string> args = { "app", "-vd" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.has_flag("--verbose"));
        CHECK(parser.has_flag("--debug"));
    }
}

TEST_CASE("값을 가지는 옵션 파싱 형식 테스트", "[cli][arg_parser]") {
    arg_parser parser;
    parser.add_option("-p", "--port", "포트 번호", true, "8080");
    parser.add_option("-c", "--config", "설정 파일 경로", true);

    SECTION("공백으로 분리된 옵션 값 (--port 9000)") {
        std::vector<std::string> args = { "app", "--port", "9000" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_int("--port") == 9000);
    }

    SECTION("등호(=)로 연결된 옵션 값 (--port=9000)") {
        std::vector<std::string> args = { "app", "--port=9000" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_int("--port") == 9000);
    }

    SECTION("단축 옵션 공백 분리 (-c app.conf)") {
        std::vector<std::string> args = { "app", "-c", "app.conf" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_string("--config") == "app.conf");
    }

    SECTION("단축 옵션 붙여쓰기 (-capp.conf)") {
        std::vector<std::string> args = { "app", "-capp.conf" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_string("--config") == "app.conf");
    }

    SECTION("기본값(default value) 동작 확인") {
        std::vector<std::string> args = { "app" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_int("--port") == 8080);
        CHECK_FALSE(parser.get_string("--config").has_value());
    }
}

TEST_CASE("위치 인자(Positional arguments) 및 '--' 구분자 테스트", "[cli][arg_parser]") {
    arg_parser parser;
    parser.add_option("-o", "--output", "출력 경로", true);

    SECTION("옵션과 위치 인자 혼용") {
        std::vector<std::string> args = { "app", "input1.txt", "-o", "out.bin", "input2.txt" };
        REQUIRE(parse_args(parser, args));

        CHECK(parser.get_string("--output") == "out.bin");
        const auto& pos = parser.get_positional();
        REQUIRE(pos.size() == 2);
        CHECK(pos[0] == "input1.txt");
        CHECK(pos[1] == "input2.txt");
    }

    SECTION("'--' 구분자 이후의 인자는 옵션 형태라도 위치 인자로 처리") {
        std::vector<std::string> args = { "app", "-o", "out.bin", "--", "-o", "--raw-arg" };
        REQUIRE(parse_args(parser, args));

        CHECK(parser.get_string("--output") == "out.bin");
        const auto& pos = parser.get_positional();
        REQUIRE(pos.size() == 2);
        CHECK(pos[0] == "-o");
        CHECK(pos[1] == "--raw-arg");
    }
}

TEST_CASE("다양한 데이터 타입 변환 테스트", "[cli][arg_parser]") {
    arg_parser parser;
    parser.add_option("", "--flag-bool", "불리언 값", true);
    parser.add_option("", "--val-int", "정수 값", true);
    parser.add_option("", "--val-int64", "64비트 정수 값", true);
    parser.add_option("", "--val-str", "문자열 값", true);

    SECTION("불리언 값 변환 (true, false, yes, no, 1, 0, on, off)") {
        std::vector<std::string> args = { "app", "--flag-bool=yes" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_bool("--flag-bool") == true);

        args = { "app", "--flag-bool=false" };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_bool("--flag-bool") == false);

        args = { "app", "--flag-bool=invalid" };
        REQUIRE(parse_args(parser, args));
        CHECK_FALSE(parser.get_bool("--flag-bool").has_value());
    }

    SECTION("정수 및 64비트 정수 범위 테스트") {
        std::vector<std::string> args = {
            "app",
            "--val-int=-12345",
            "--val-int64=9223372036854775807"
        };
        REQUIRE(parse_args(parser, args));
        CHECK(parser.get_int("--val-int") == -12345);
        CHECK(parser.get_int64("--val-int64") == 9223372036854775807LL);
    }
}

TEST_CASE("부동소수점(float, double) 및 옵션 속성 테스트", "[cli][arg_parser]") {
    arg_parser parser;

    // 소수점 2자리 반올림 및 inf/nan 제한 옵션
    parser.add_option("", "--ratio", "비율", true)
        .precision(2);

    parser.add_option("", "--strict-num", "특수값 금지 실수", true)
        .allow_inf(false)
        .allow_nan(false);

    SECTION("precision에 따른 반올림 확인") {
        std::vector<std::string> args = { "app", "--ratio=3.14159" };
        REQUIRE(parse_args(parser, args));

        auto f = parser.get_float("--ratio");
        REQUIRE(f.has_value());
        CHECK(Catch::Approx(*f).epsilon(0.001) == 3.14f);

        auto d = parser.get_double("--ratio");
        REQUIRE(d.has_value());
        CHECK(Catch::Approx(*d).epsilon(0.001) == 3.14);
    }

    SECTION("inf 및 nan 파싱 처리") {
        std::vector<std::string> args = { "app", "--ratio=inf" };
        REQUIRE(parse_args(parser, args));
        auto f = parser.get_float("--ratio");
        REQUIRE(f.has_value());
        CHECK(std::isinf(*f));

        args = { "app", "--ratio=-nan" };
        REQUIRE(parse_args(parser, args));
        auto d = parser.get_double("--ratio");
        REQUIRE(d.has_value());
        CHECK(std::isnan(*d));
    }

    SECTION("allow_inf/allow_nan 비활성화 시 파싱 거부 확인") {
        std::vector<std::string> args = { "app", "--strict-num=inf" };
        REQUIRE(parse_args(parser, args));
        CHECK_FALSE(parser.get_double("--strict-num").has_value());

        args = { "app", "--strict-num=nan" };
        REQUIRE(parse_args(parser, args));
        CHECK_FALSE(parser.get_double("--strict-num").has_value());
    }
}

TEST_CASE("문자열 대소문자 옵션 테스트", "[cli][arg_parser]") {
    arg_parser parser;
    parser.add_option("-m", "--mode", "실행 모드", true)
        .case_sensitive(false);

    std::vector<std::string> args = { "app", "--mode=ReLeAsE" };
    REQUIRE(parse_args(parser, args));

    // case_sensitive가 false이므로 소문자로 정규화되어 반환되어야 함
    CHECK(parser.get_string("--mode") == "release");
}

TEST_CASE("파싱 에러 처리 테스트", "[cli][arg_parser]") {
    arg_parser parser;
    parser.add_option("-f", "--file", "파일명", true);

    SECTION("정의되지 않은 전체 옵션 전달 시 실패") {
        std::vector<std::string> args = { "app", "--undefined-opt" };
        CHECK_FALSE(parse_args(parser, args));
        CHECK(parser.get_error().find("Unknown option") != std::string::npos);
    }

    SECTION("정의되지 않은 단축 옵션 전달 시 실패") {
        std::vector<std::string> args = { "app", "-x" };
        CHECK_FALSE(parse_args(parser, args));
        CHECK(parser.get_error().find("Unknown option") != std::string::npos);
    }

    SECTION("값을 요구하는 옵션에 값이 누락된 경우 실패") {
        std::vector<std::string> args = { "app", "--file" };
        CHECK_FALSE(parse_args(parser, args));
        CHECK(parser.get_error().find("Option requires an argument") != std::string::npos);
    }

    SECTION("값을 요구하는 단축 옵션 바로 뒤에 다음 옵션(-)이 온 경우 실패") {
        std::vector<std::string> args = { "app", "-f", "-h" };
        CHECK_FALSE(parse_args(parser, args));
        CHECK(parser.get_error().find("Option requires an argument") != std::string::npos);
    }
}
