#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/cli/cli.hpp"

int main(int argc, char* argv[]) {
    namespace mcc = mino::core::cli;
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    using arg_parser = mcc::arg_parser;
    using string_encoding = mcc::string_encoding;

    arg_parser parser("Mino Server Option Runner (Non-template Getter)");

    // 1. 문자열 (기본 UTF-8 및 대소문자 무시)
    parser.add_option("-e", "--env", U8("실행 환경 (dev/stage/prod)"), true, "production")
        .case_sensitive(false); // 대소문자 구분 없이 처리
    // --env=DEV, --env=dev, --env=Dev 모두 동일하게 "dev"로 처리됨

#ifdef _WIN32
    // 2. 문자열 (인코딩 변환 옵션: CP949, Console)
    parser.add_option("-t", "--title", U8("CP949 문자열"), true)
        .encoding(string_encoding::cp949);
#else
    parser.add_option("-t", "--title", U8("UTF-8 문자열"), true)
        .encoding(string_encoding::utf8);
#endif

    parser.add_option("-m", "--msg-console", U8("OS 콘솔 인코딩 문자열"), true)
        .encoding(string_encoding::console); // Windows 인 경우 CP949, Linux/MacOS 경우 UTF-8로 변환됨

    // 3. 정수형 옵션 (int, int64_t)
    parser.add_option("-p", "--port", U8("서버 포트 번호 (int)"), true, "8080");

    parser.add_option("-l", "--max-memory", U8("최대 메모리 바이트 (int64_t)"), true, "8589934592");

    // 4. 불리언형 옵션 (bool)
    parser.add_option("-a", "--auto-reload", U8("자동 갱신 플래그 (true/false)"), true, "true");

    // 5. 단정밀도 실수형 (float)
    parser.add_option("-s", "--scale", U8("스케일 계수 (float, 소수점 2자리)"), true, "1.4142")
        .precision(2) // 소수점 2자리까지 반올림
        .allow_inf(false) // 무한대 허용하지 않음
        .allow_nan(false); // NaN(Not a Number) 허용하지 않음

    // 6. 배정밀도 실수형 (double)
    parser.add_option("-w", "--weight", U8("가중치 (double, inf/nan 허용)"), true, "inf")
        .allow_inf(true) // 무한대 허용
        .allow_nan(true); // NaN(Not a Number) 허용

    // 7. 단순 플래그
    parser.add_option("-d", "--daemon", U8("백그라운드 데몬 구동 여부"), false);

    parser.add_option("-v", "--verbose", U8("상세 로그 출력"), false);

    // 파싱 실행
    if (!parser.parse(argc, argv)) {
        mcs::print::eprintln("파싱 에러: {}", parser.get_error());
        parser.print_help();
        return 1;
    }

    // 도움말 출력
    if (parser.has_flag("--help")) {
        parser.print_help();
        return 0;
    }

    // 전용 Getter 메서드 호출
    std::optional<std::string> env = parser.get_string("--env").value_or("unknown");
    assert(env);
    std::optional<std::string> title = parser.get_string("--title").value_or("N/A");
    assert(title);
    std::optional<std::string> msg_console = parser.get_string("--msg-console").value_or("N/A");
    assert(msg_console);

    std::optional<int> port = parser.get_int("--port").value_or(8080);
    assert(port);
    std::optional<int64_t> max_memory = parser.get_int64("--max-memory").value_or(0);
    assert(max_memory);

    std::optional<bool> auto_reload = parser.get_bool("--auto-reload").value_or(false);
    assert(auto_reload);

    std::optional<float> scale = parser.get_float("--scale");
    assert(scale);
    std::optional<double> weight = parser.get_double("--weight");
    assert(weight);

    bool is_daemon = parser.has_flag("--daemon");
    bool is_verbose = parser.has_flag("--verbose");

    const auto& positional_args = parser.get_positional();

    // 결과 출력
    mcsp::println("[파서 실행 결과]");
    mcsp::println(" - get_string (env)         : {}", *env);
    mcsp::println(" - get_string (title_cp949) : {}", *title);
    mcsp::println(" - get_string (msg_console) : {}", *msg_console);
    mcsp::println(" - get_int (port)           : {}", *port);
    mcsp::println(" - get_int64 (max_memory)   : {}", *max_memory);
    mcsp::println(" - get_bool (auto_reload)   : {}", *auto_reload ? "true" : "false");
    mcsp::println(" - has_flag (daemon)        : {}", is_daemon ? "true" : "false");
    mcsp::println(" - has_flag (verbose)       : {}", is_verbose ? "true" : "false");

    if (scale) {
        mcsp::println(" - get_float (scale, prec 2): {}", *scale);
    }
    else {
        mcsp::println(" - get_float (scale)        : 유효하지 않은 float 값");
    }

    if (weight) {
        mcsp::println(" - get_double (weight)      : {}", *weight);
        if (std::isinf(*weight))
            mcsp::println("   * 무한대(inf) 인식됨");
        if (std::isnan(*weight))
            mcsp::println("   * NaN 인식됨");
    }

    if (!positional_args.empty()) {
        mcsp::println(" - get_positional (총 {}개):", positional_args.size());
        for (size_t i = 0; i < positional_args.size(); ++i) {
            mcsp::println("   [{}] {}", i, positional_args[i]);
        }
    }

    return 0;
}
