#include <iostream>
#include <memory>
#include <utility>

#include <spdlog/spdlog.h>

#include "mino/external/log/factory/factory.hpp"
#include "mino/core/string/to_console_encoding.hpp"

const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

int main(int argc, char* argv[]) {
    namespace log_factory = mino::external::log::factory;
    using logger_registry = log_factory::logger_registry;
    using tiny_logger = log_factory::tiny_logger;

    // ----------------------------------------------------------------
    print(tce("=== [1] logger_registry 멤버 함수 테스트 ==="));

    // logger_registry::instance()
    auto& registry = logger_registry::instance();

    // logger_registry::create()
    registry.create(
        "reg_direct", // 로거 이름
        spdlog::level::info, // 로그 레벨
        true, // 콘솔 출력 활성화
        false, // 파일 출력 비활성화
        "", // 파일 경로 (콘솔만 사용하므로 사용하지 않음)
        0, // 최대 파일 크기 (사용하지 않음)
        7); // 최대 파일 개수 (사용하지 않음)

    // logger_registry::create_by_max_size()
    registry.create_by_max_size(
        "reg_size", // 로거 이름
        spdlog::level::debug, // 로그 레벨
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/reg_size.log", // 파일 경로
        1024 * 1024, // 최대 파일 크기 (1MB)
        3); // 최대 파일 개수 (3개)

    // logger_registry::create_by_retention_days()
    registry.create_by_retention_days(
        "reg_daily", // 로거 이름
        spdlog::level::warn, // 로그 레벨
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/reg_daily.log", // 파일 경로
        7, // 보존 일수 (7일)
        0, // 회전 시간 (0시)
        0); // 회전 분 (0분)

    // logger_registry::get()
    auto reg_logger = registry.get("reg_direct");
    reg_logger << tce(fmt::format("logger_registry::create 성공: {}", "reg_direct"));

    // ----------------------------------------------------------------
    print(tce("\n=== [2] 전역 헬퍼 함수 (create / get) 테스트 ==="));

    // 전역 create (레벨 명시 & 기본 레벨)
    log_factory::create(
        "g_logger_lvl", // 로거 이름
        spdlog::level::trace, // 로그 레벨
        true, // 콘솔 출력 활성화
        false, // 파일 출력 비활성화
        "", // 파일 경로 (콘솔만 사용하므로 사용하지 않음)
        0, // 최대 파일 크기 (사용하지 않음)
        7); // 최대 파일 개수 (사용하지 않음)

    log_factory::create(
        "g_logger_def", // 로거 이름
        true, // 콘솔 출력 활성화
        false, // 파일 출력 비활성화
        "", // 파일 경로 (콘솔만 사용하므로 사용하지 않음)
        0, // 최대 파일 크기 (사용하지 않음)
        7); // 최대 파일 개수 (사용하지 않음)

    // 전역 create_by_max_size (레벨 명시 & 기본 레벨)
    log_factory::create_by_max_size(
        "g_size_lvl", // 로거 이름
        spdlog::level::debug, // 로그 레벨
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/g_size_lvl.log", // 파일 경로
        2 * 1024 * 1024, // 최대 파일 크기 (2MB)
        5); // 최대 파일 개수 (5개)

    log_factory::create_by_max_size(
        "g_size_def", // 로거 이름
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/g_size_def.log", // 파일 경로
        2 * 1024 * 1024, // 최대 파일 크기 (2MB)
        5); // 최대 파일 개수 (5개)

    // 전역 create_by_retention_days (레벨 명시 & 기본 레벨)
    log_factory::create_by_retention_days(
        "g_daily_lvl", // 로거 이름
        spdlog::level::err, // 로그 레벨
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/g_daily_lvl.log", // 파일 경로
        14, // 보존 일수 (14일)
        1, // 회전 시간 (1시)
        30); // 회전 분 (30분)

    log_factory::create_by_retention_days(
        "g_daily_def", // 로거 이름
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/g_daily_def.log", // 파일 경로
        14, // 보존 일수 (14일)
        1, // 회전 시간 (1시)
        30); // 회전 분 (30분)

    // 전역 get()
    auto& g_logger = log_factory::get("g_logger_lvl");
    g_logger << tce(fmt::format("전역 create 및 get 호출 성공: 레벨={}", static_cast<int>(spdlog::level::trace)));

    // ----------------------------------------------------------------
    print(tce("\n=== [3] tiny_logger 독립 생성자 및 멤버 함수 테스트 ==="));

    // tiny_logger 생성자
    tiny_logger custom_logger("custom_standalone");

    // tiny_logger::set_log_level()
    custom_logger.set_log_level(spdlog::level::info);

    // tiny_logger::setup()
    custom_logger.setup(
        true, // 콘솔 출력 활성화
        true, // 파일 출력 활성화
        "logs/custom.log", // 파일 경로
        "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v", // 로그 패턴
        1024 * 512, // 최대 파일 크기 (512KB)
        2, // 최대 파일 개수 (2개)
        0, // 회전 시간 (0시)
        0); // 회전 분 (0분)

    custom_logger << tce(fmt::format("custom_logger format 로깅: 정수={}, 문자열={}", 100, "OK"));

    custom_logger << tce(fmt::format("custom_logger stream 로깅: 값={}", 999.99));

    // ----------------------------------------------------------------
    print(tce("\n=== [4] tiny_logger::stream_proxy 멤버 테스트 (생성, 이동, 스트림, 소멸) ==="));

    auto spd_internal = spdlog::get("custom_standalone");
    if (spd_internal) {
        // stream_proxy 생성자 호출 및 operator<<
        tiny_logger::stream_proxy proxy1(spd_internal, spdlog::level::info);
        proxy1 << tce("stream_proxy 직접 생성 메시지 1");

        // stream_proxy 이동 생성자 (Move Constructor) 테스트
        tiny_logger::stream_proxy proxy2(std::move(proxy1));
        proxy2 << tce(" -> 이동 생성자를 거친 메시지");
        // proxy2 소멸 시점에 spdlog::logger::log() 호출 및 출력

        // stream_proxy 이동 대입 연산자 (Move Assignment) 테스트
        tiny_logger::stream_proxy proxy3(spd_internal, spdlog::level::warn);
        proxy3 << tce("임시 메시지");

        tiny_logger::stream_proxy proxy4(spd_internal, spdlog::level::info);
        proxy4 = std::move(proxy3);
        proxy4 << tce(" -> 이동 대입 연산자를 거쳐 출력");
        // proxy4 소멸 시점에 출력
    }

    // ----------------------------------------------------------------
    print(tce("\n=== [5] 미등록 Fallback 및 eprint 테스트 ==="));

    // Fallback 로거 확인 (spdlog::level::off 상태로 아무것도 출력되지 않아야 정상)
    auto fallback = log_factory::get("non_existing_name");
    fallback(tce("이 메시지는 출력되지 않아야 합니다.").c_str());
    fallback << tce("스트림 메시지도 출력되지 않아야 합니다.");

    eprint(tce("표준 에러 스트림(eprint) 테스트 메시지"), endl);
    print(tce("=== 로거 테스트 완료 ==="));

    return 0;
}
