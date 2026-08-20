#include <iostream>
#include <memory>
#include <utility>
#include <cassert>

#include "mino/core/string/to_console_encoding.hpp"

// mino external log adapter
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "mino/external/log/adapter/adapter.hpp"

// 전역 유틸리티 람다들 정의
// print: 여러 인자를 받아 std::cout에 순차적으로 출력 후 개행
// eprint: 여러 인자를 받아 std::cerr에 순차적으로 출력 후 개행
// endl: std::endl 포인터 (명시적 타입 지정으로 가독성 향상)
// tce: 콘솔 출력에 사용될 문자열 인코딩 변환 함수(네임스페이스 별칭)
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

void test_logger_adapter();
void test_alias_and_helpers();
void test_logger_adapter_wrapper();

int main(int argc, char* argv[]) {
    try {
        // 각 테스트 함수들을 순차적으로 실행하여 전체 공개 인터페이스 검증

        // NOTE: logger_adapter의 용도는 전역 로거 또는 소유 로거에 대한 안전한 접근과 버퍼링을 제공
        test_logger_adapter();

        // NOTE: alias_fun은 전역 또는 소유 로거에 바인딩된 호출 가능한 객체로, 편리한 로깅 API 제공
        test_alias_and_helpers();

        // NOTE: logger_adapter_wrapper는 이름 기반 접근이나 문자열 기반 로그 API를 제공하는 래퍼 클래스
        test_logger_adapter_wrapper();

        // 모든 테스트가 정상적으로 완료되었음을 알림
        print(tce("\n모든 퍼블릭 멤버 및 기능 테스트가 성공적으로 완료되었습니다."));
    }
    catch (const std::exception& ex) {
        // 예외 발생 시 에러 메시지를 표준 오류로 출력하고 비정상 종료 코드 반환
        eprint(tce("테스트 실행 중 예외 발생: "), tce(ex.what()));
        return 1;
    }
    return 0;
}

// 1. logger_adapter의 모든 public 멤버 테스트
// 이 함수는 logger_adapter의 생성자, set_logger, log 등 공개된 인터페이스를 순차적으로 검증한다.
void test_logger_adapter() {
    // 네임스페이스 별칭으로 코드 길이 축약
    namespace log_adapter = mino::external::log::adapter;
    using logger_adapter = log_adapter::logger_adapter;
    using level = log_adapter::level;

    // 화면에 구분선 및 테스트 섹션 제목 출력
    print(tce("\n========================================"));
    print(tce("[Test 1] logger_adapter 모든 Public 멤버 테스트"));
    print(tce("========================================"));

    // spdlog에서 컬러 콘솔 싱크로 로거 2개 생성
    auto test_logger_1 = spdlog::stdout_color_mt("test_logger_1");
    auto test_logger_2 = spdlog::stdout_color_mt("test_logger_2");

    // 로깅 레벨을 최상위(trace)로 설정하여 모든 로그 출력 허용
    test_logger_1->set_level(spdlog::level::trace);
    test_logger_2->set_level(spdlog::level::trace);

    // 1-1. 기본 생성자: logger_adapter()
    // 기본 생성자로 생성한 adapter는 내부에 등록된 로거가 없으므로 로그를 버퍼링할 수 있음
    logger_adapter adapter_default;
    print(tce("-> [기본 생성자] 로거 미등록 상태에서 로그 레벨별 버퍼링:"));

    // 로거 미등록 상태에서 호출된 로그는 내부 버퍼에 저장될 수 있음 (구현에 따라)
    adapter_default.log(level::trace, "Buffered TRACE: {}", 1);
    adapter_default.log(level::debug, "Buffered DEBUG: {}", 2);
    adapter_default.log(level::info, "Buffered INFO: {}", 3);

    // 1-2. set_logger()를 통한 로거 주입 및 버퍼 플러시
    // set_logger에 의해 로거가 등록되면 이전에 버퍼링된 메시지들이 플러시되어 출력되어야 함
    print(tce("-> [set_logger] test_logger_1 등록 (버퍼 플러시):"));
    adapter_default.set_logger(test_logger_1);

    // 1-3. explicit 생성자: logger_adapter(std::shared_ptr<::spdlog::logger>)
    // 생성 시점에 로거를 전달하면 즉시 출력 가능 상태로 초기화됨
    print(tce("-> [explicit 생성자] test_logger_2로 초기화된 어댑터:"));
    logger_adapter adapter_with_logger(test_logger_2);

    // 1-4. log() 템플릿 멤버 함수 (모든 level 열거형 테스트)
    // 각 로그 레벨에 대해 포맷 문자열과 값을 전달하며 출력 동작 확인
    adapter_with_logger.log(level::trace, "Direct TRACE: val={}", 10);
    adapter_with_logger.log(level::debug, "Direct DEBUG: val={}", 20);
    adapter_with_logger.log(level::info, "Direct INFO: val={}", 30);
    adapter_with_logger.log(level::warn, "Direct WARN: val={}", 40);
    adapter_with_logger.log(level::error, "Direct ERROR: val={}", 50);
    adapter_with_logger.log(level::critical, "Direct CRITICAL: val={}", 60);

    // 1-5. global_logger_adapter() 참조 접근자
    // 전역 어댑터에 대한 레퍼런스를 얻어 전역 로거 설정 및 호출을 테스트
    print(tce("-> [global_logger_adapter] 전역 어댑터 직접 참조 호출:"));
    logger_adapter& global_ref = log_adapter::global_logger_adapter();
    global_ref.set_logger(test_logger_1);
    global_ref.log(level::info, tce("global_logger_adapter() 직접 사용 로그"));
}

// 2. alias_fun 및 헬퍼 함수, 매크로 테스트
// alias_fun은 특정 레벨에 바인딩된 호출 가능한 객체(함수 객체)로, 편리한 로깅 API 제공
void test_alias_and_helpers() {
    namespace log_adapter = mino::external::log::adapter;
    using level = log_adapter::level;
    using alias_fun = log_adapter::alias_fun;

    print(tce("\n========================================"));
    print(tce("[Test 2] alias_fun 및 전역 헬퍼 함수 테스트"));
    print(tce("========================================"));

    // 전역 로거 생성 및 레벨 설정
    auto global_logger = spdlog::stdout_color_mt("global_logger");
    global_logger->set_level(spdlog::level::trace);

    // 2-1. set_global_logger() 및 set_global_logger_by_name()
    // 전역 어댑터 또는 이름으로 전역 로거를 설정하는 헬퍼 함수들 테스트
    log_adapter::set_global_logger(global_logger);
    log_adapter::set_global_logger_by_name("global_logger");

    // 2-2. alias_fun 생성자 1: explicit alias_fun(level lvl) - 전역 어댑터 바인딩
    // 전역 어댑터와 바인딩된 alias를 통해 전역 로거로 로그 전송
    alias_fun global_info_alias(level::info);
    global_info_alias(tce("alias_fun(level) 생성자 테스트: 전역 로거로 출력"));

    // 2-3. alias_fun 생성자 2: alias_fun(level lvl, std::shared_ptr<::spdlog::logger>) - 소유 어댑터 바인딩
    // 특정 로거를 직접 전달하여 해당 로거로 로그를 전송하는 alias 생성
    auto dedicated_logger = spdlog::stdout_color_mt("dedicated_logger");
    alias_fun owned_warn_alias(level::warn, dedicated_logger);
    owned_warn_alias(tce("alias_fun(level, logger) 생성자 테스트: 전용 로거로 출력"));

    // 2-4. 팩토리 헬퍼: make_alias()
    // 레벨만으로 alias_fun을 만드는 팩토리 함수 사용 테스트
    auto factory_debug = log_adapter::make_alias(level::debug);
    factory_debug(tce("make_alias() 팩토리 헬퍼 호출: debug_val={}"), 999);

    // 2-5. 팩토리 헬퍼: make_alias_for_logger()
    // 특정 로거와 레벨로 alias를 생성하는 헬퍼 테스트
    auto factory_custom_error = log_adapter::make_alias_for_logger(level::error, dedicated_logger);
    factory_custom_error(tce("make_alias_for_logger() 팩토리 헬퍼 호출: error code={}"), 404);

    // 2-6. 매크로 테스트 (MINO_DEFINE_LOG_ALIAS, MINO_DEFINE_LOG_ALIAS_FOR_LOGGER)
    // 매크로로 정의된 방식과 팩토리 방식의 동작을 비교 확인
    // (실제 매크로 사용부는 여기에 없으므로 명시적으로 alias_fun을 생성하여 대체)
    alias_fun log_macro_info{ level::info };
    log_macro_info(tce("매크로 정의 방식 전역 alias 호출"));

    alias_fun log_macro_dedicated = log_adapter::make_alias_for_logger(level::critical, dedicated_logger);
    log_macro_dedicated(tce("매크로 정의 방식 전용 로거 alias 호출"));
}

// 3. logger_adapter_wrapper의 모든 public 멤버 테스트
// logger_adapter_wrapper는 이름 기반 접근이나 문자열 기반 로그 API를 제공하는 래퍼 클래스임을 가정
void test_logger_adapter_wrapper() {
    namespace log_adapter = mino::external::log::adapter;
    using logger_adapter_wrapper = log_adapter::logger_adapter_wrapper;
    using log_level = log_adapter::log_level;

    print(tce("\n========================================"));
    print(tce("[Test 3] logger_adapter_wrapper 모든 Public 멤버 테스트"));
    print(tce("========================================"));

    // 3-1. 정적 메서드: set_global_logger_by_name()
    // wrapper의 정적 헬퍼를 통해 전역 로거 이름 설정(이미 존재하는 로거 이름을 사용)
    logger_adapter_wrapper::set_global_logger_by_name("global_logger");

    // 3-2. 기본 생성자: logger_adapter_wrapper()
    // 기본 생성자로 생성하면 내부적으로 전역 또는 디폴트 로거와 연결될 수 있음
    logger_adapter_wrapper default_wrapper;
    default_wrapper.log_string(log_level::info, "기본 생성자로 생성된 Wrapper: log_string 호출");

    // 3-3. explicit 생성자: logger_adapter_wrapper(const std::string& logger_name)
    // 이름으로 초기화하면 해당 이름의 로거와 연결된 wrapper가 생성됨
    logger_adapter_wrapper named_wrapper("global_logger");

    // 3-4. log_string() 멤버 함수 (모든 log_level 열거형 테스트)
    // 문자열 전용 API를 호출하여 각 레벨별 출력 동작을 확인
    named_wrapper.log_string(log_level::trace, "Wrapper log_string [TRACE]");
    named_wrapper.log_string(log_level::debug, "Wrapper log_string [DEBUG]");
    named_wrapper.log_string(log_level::info, "Wrapper log_string [INFO]");
    named_wrapper.log_string(log_level::warn, "Wrapper log_string [WARN]");
    named_wrapper.log_string(log_level::error, "Wrapper log_string [ERROR]");
    named_wrapper.log_string(log_level::critical, "Wrapper log_string [CRITICAL]");

    // 3-5. 이동 생성자: logger_adapter_wrapper(logger_adapter_wrapper&&) noexcept
    // 이동 생성 시 내부 리소스 소유권 이전을 통해 이전 인스턴스는 비가동 상태가 될 수 있음
    logger_adapter_wrapper move_constructed_wrapper(std::move(named_wrapper));
    move_constructed_wrapper.log_string(log_level::info, tce("이동 생성자로 전달된 인스턴스에서 호출"));

    // 3-6. 이동 대입 연산자: operator=(logger_adapter_wrapper&&) noexcept
    // 이동 대입으로 다른 인스턴스에 리소스 이전 후 호출 테스트
    logger_adapter_wrapper move_assigned_wrapper;
    move_assigned_wrapper = std::move(move_constructed_wrapper);
    move_assigned_wrapper.log_string(log_level::info, tce("이동 대입 연산자로 전달된 인스턴스에서 호출"));
}


