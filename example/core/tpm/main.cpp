#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <any>
#include <cassert>
#include <stdexcept>

#include "mino/core/tpm/tpm.hpp"
#include "mino/core/log/log.hpp"

// =============================================================================
// 1. 사용자 정의 트랜잭션 컨텍스트 클래스
// - mino::core::tpm::transaction_context를 상속받아 구현합니다.
// - request_service 템플릿의 static_assert 제약 조건을 충족합니다.
// =============================================================================
class test_context : public mino::core::tpm::transaction_context {
public:
    // 생성자: 트랜잭션 고유 ID를 기반 클래스에 전달하여 초기화합니다.
    explicit test_context(uint64_t tx_id)
        : mino::core::tpm::transaction_context(tx_id), committed_value(0) {}

    // 서비스 실행 결과를 커밋 액션으로 전달하기 위한 테스트용 멤버 변수
    int committed_value;

    // 서비스 내부에서 실제로 주입된 로거를 확인하기 위한 포인터 보관 변수
    std::shared_ptr<mino::core::log::tinylog::logger> observed_logger{ nullptr };
};

int main() {
    namespace mclt = mino::core::log::tinylog;

    using console_sink_config = mclt::console_sink_config;
    console_sink_config ccfg;
#ifdef _WIN32
    ccfg.encoding = mclt::encoding_type::cp949;
    ccfg.eol = mclt::eol_type::crlf;
#else
    ccfg.encoding = mclt::encoding_type::utf8;
    ccfg.eol = mclt::eol_type::lf;
#endif

    std::cout << "========================================\n";
    std::cout << "  TP Monitor Full Interface Unit Test   \n";
    std::cout << "========================================\n";

    // =========================================================================
    // Unit Test 1: transaction_context 클래스의 모든 퍼블릭 멤버/메서드 검증
    // =========================================================================
    std::cout << "\n[Test 1] transaction_context Public Members\n";
    {
        uint64_t test_id = 42;
        test_context ctx(test_id);

        // 1-1. tx_id가 기본 생성자를 통해 올바르게 설정되었는지 확인
        assert(ctx.id == test_id);

        // 1-2. 초기 생성 시 중단 플래그(is_aborted)가 false인지 확인
        assert(ctx.is_aborted == false);

        // 1-3. 초기 생성 시 로거가 nullptr인지 확인 (get_logger() 호출)
        assert(ctx.get_logger() == nullptr);

        // 1-4. abort() 메서드 호출 시 is_aborted 플래그가 true로 바뀌는지 확인
        ctx.abort();
        assert(ctx.is_aborted == true);

        // 1-5. set_logger() 및 get_logger() 인터페이스 정상 동작 확인
        auto dummy_logger = std::make_shared<mclt::logger>("test_logger");
        dummy_logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));

        ctx.set_logger(dummy_logger);
        assert(ctx.get_logger() == dummy_logger);

        std::cout << " -> transaction_context: PASSED\n";
    }

    // =========================================================================
    // Unit Test 2: tp_monitor의 기본 설정 및 워커 스레드 인터페이스 검증
    // =========================================================================
    std::cout << "\n[Test 2] tp_monitor Logger & Worker Management\n";
    using tp_monitor = mino::core::tpm::tp_monitor;

    tp_monitor monitor; // 인스턴스 생성

    // 2-1. tp_monitor::set_logger() 및 tp_monitor::get_logger() 검증

    auto monitor_logger = std::make_shared<mclt::logger>("monitor_logger");
    monitor_logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));

    monitor.set_logger(monitor_logger); // set logger 
    assert(monitor.get_logger() == monitor_logger);

    // 2-2. tp_monitor::start_workers() 워커 스레드 4개 기동
    assert(monitor.start_workers(4));

    // 2-3. start_workers() 중복 호출 시 방어 로직이 정상 작동하는지 확인
    assert(!monitor.start_workers(2));

    auto worker_count = monitor.worker_count(); // 현재 워커 수 확인 (4개여야 함)
    assert(worker_count == 4);

    std::cout << " -> Worker initialization: PASSED\n";

    // =========================================================================
    // Unit Test 3: 서비스 등록(register_service) 및 전용 로거 설정(set_ctx_logger)
    // =========================================================================
    std::cout << "\n[Test 3] Service Registration & Context Logger Routing\n";

    auto dedicated_service_logger = std::make_shared<mclt::logger>("calc_service_logger");
    dedicated_service_logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));
 
    // 3-1. 정상 계산 서비스 등록: 가변 인자 전달 및 연산 결과 세팅
    monitor.register_service(
        "CalcService", // 서비스 이름
        [](mino::core::tpm::transaction_context& ctx, const std::vector<std::any>& args) -> bool
        {
            if (args.size() < 3)
                return false;

            // any_cast를 통해 가변 인자 풀기
            int a, b;
            std::string op;
            try {
                a = std::any_cast<int>(args[0]);
                b = std::any_cast<int>(args[1]);
                op = std::any_cast<std::string>(args[2]);
            } catch (const std::bad_any_cast&) {
                return false; // 타입 변환 실패 시 false 반환
            }

            auto* tctx = dynamic_cast<test_context*>(&ctx);
            if (tctx == nullptr) {
                return false; // 컨텍스트 타입이 맞지 않으면 false 반환
            }

            // 컨텍스트에 주입된 로거를 기록
            tctx->observed_logger = ctx.get_logger();

            // 연산 수행
            if (op == "+")
                tctx->committed_value = a + b; // 더하기 
            else if (op == "*")
                tctx->committed_value = a * b; // 곱하기
            else
                return false; // 지원하지 않는 연산자

            return true; // 성공 반환
        }
    );

    // 3-2. "CalcService" 전용 컨텍스트 로거 등록 (set_ctx_logger 검증)
    monitor.set_ctx_logger("CalcService", dedicated_service_logger);

    // 3-3. 공통 로거를 상속받는지 확인하기 위한 서비스 등록
    monitor.register_service(
        "CommonLogService", // 서비스 이름
        [](mino::core::tpm::transaction_context& ctx, const std::vector<std::any>&) -> bool
        { 
            auto* tctx = dynamic_cast<test_context*>(&ctx);
            if (tctx == nullptr) {
                return false; // 컨텍스트 타입이 맞지 않으면 false 반환
            }

            tctx->observed_logger = ctx.get_logger();

            return true;
        }
    );

    // 3-4. abort()를 호출하는 서비스 등록 (명시적 롤백 케이스)
    monitor.register_service(
        "AbortService",
        [](mino::core::tpm::transaction_context& ctx, const std::vector<std::any>&) -> bool
        {
            ctx.abort(); // 트랜잭션 중단 플래그 설정
            return true; // 서비스가 true를 반환해도 is_aborted가 true면 커밋되지 않아야 함
        }
    );

    // 3-5. false를 반환하는 서비스 등록 (비즈니스 실패 케이스)
    monitor.register_service(
        "FailService",
        [](mino::core::tpm::transaction_context&, const std::vector<std::any>&) -> bool
        {
            return false;
        }
    );

    // 3-6. std::exception 예외를 발생시키는 서비스 등록
    monitor.register_service(
        "StdExceptionService",
        [](mino::core::tpm::transaction_context&, const std::vector<std::any>&) -> bool
        {
            throw std::runtime_error("std::runtime_error occurred!");
        }
    );

    // 3-7. catch(...)로 처리되는 비표준 예외를 발생시키는 서비스 등록
    monitor.register_service(
        "UnknownExceptionService",
        [](mino::core::tpm::transaction_context&, const std::vector<std::any>&) -> bool
        {
            throw 500; // int 타입 예외
        }
    );

    std::cout << " -> Service & Context logger setup: PASSED\n";

    // =========================================================================
    // Unit Test 4: request_service 템플릿 호출 및 비동기 처리 시나리오 검증
    // =========================================================================
    std::cout << "\n[Test 4] request_service Scenarios\n";

    // 공통 팩토리 람다 정의 (tx_id를 받아 unique_ptr로 test_context 생성)
    auto factory = [](uint64_t tx_id) {
        return std::make_unique<test_context>(tx_id);
    };

    // 시나리오 A: 정상 연산 및 전용 로거 우선 주입 검증 (가변 인자: 20, 30, "+")
    bool commit_called_a = false;
    auto fut_a = monitor.request_service<test_context>(
        "CalcService",
        factory,
        [&commit_called_a, dedicated_service_logger](test_context& ctx) {
            commit_called_a = true;

            // 20 + 30 연산 결과 확인
            assert(ctx.committed_value == 50);

            // 모니터 공통 로거가 아닌 '전용 로거'가 주입되었는지 확인
            assert(ctx.observed_logger == dedicated_service_logger);
        },
        20, 30, std::string("+")
    );

    // 시나리오 B: 공통 로거 폴백 주입 검증 (전용 로거가 없을 때 monitor logger 주입)
    bool commit_called_b = false;
    auto fut_b = monitor.request_service<test_context>(
        "CommonLogService",
        factory,
        [&commit_called_b, monitor_logger](test_context& ctx) {
            commit_called_b = true;

            // 전용 로거가 없으므로 모니터 공통 로거가 주입되었는지 확인
            assert(ctx.observed_logger == monitor_logger);
        }
    );

    // 시나리오 C: ctx.abort() 호출 시 커밋이 실행되지 않고 실패(false)하는지 검증
    bool commit_called_c = false;
    auto fut_c = monitor.request_service<test_context>(
        "AbortService",
        factory,
        [&commit_called_c](test_context&) {
            commit_called_c = true; // 호출되면 안 됨
        }
    );

    // 시나리오 D: 서비스 함수가 false를 반환했을 때 커밋되지 않고 false를 반환하는지 검증
    bool commit_called_d = false;
    auto fut_d = monitor.request_service<test_context>(
        "FailService",
        factory,
        [&commit_called_d](test_context&) {
            commit_called_d = true; // 호출되면 안 됨
        }
    );

    // 시나리오 E: std::exception 발생 시 시스템 중단 없이 안전하게 격리/실패하는지 검증
    bool commit_called_e = false;
    auto fut_e = monitor.request_service<test_context>(
        "StdExceptionService",
        factory,
        [&commit_called_e](test_context&) {
            commit_called_e = true; // 호출되면 안 됨
        }
    );

    // 시나리오 F: 알 수 없는 예외(...) 발생 시 격리 및 실패 처리 검증
    bool commit_called_f = false;
    auto fut_f = monitor.request_service<test_context>(
        "UnknownExceptionService",
        factory,
        [&commit_called_f](test_context&) {
            commit_called_f = true; // 호출되면 안 됨
        }
    );

    // 시나리오 G: 등록되지 않은 서비스 이름 라우팅 요청 시 false 반환 검증
    bool commit_called_g = false;
    auto fut_g = monitor.request_service<test_context>(
        "UnregisteredService",
        factory,
        [&commit_called_g](test_context&) {
            commit_called_g = true; // 호출되면 안 됨
        }
    );

    // =========================================================================
    // Future 결과 대기 및 단언(assert) 검증
    // =========================================================================
    assert(fut_a.get() == true && commit_called_a == true);   // 시나리오 A: 성공 및 커밋 수행
    assert(fut_b.get() == true && commit_called_b == true);   // 시나리오 B: 성공 및 커밋 수행
    assert(fut_c.get() == false && commit_called_c == false); // 시나리오 C: 실패 및 커밋 미수행
    assert(fut_d.get() == false && commit_called_d == false); // 시나리오 D: 실패 및 커밋 미수행
    assert(fut_e.get() == false && commit_called_e == false); // 시나리오 E: 실패 및 커밋 미수행
    assert(fut_f.get() == false && commit_called_f == false); // 시나리오 F: 실패 및 커밋 미수행
    assert(fut_g.get() == false && commit_called_g == false); // 시나리오 G: 실패 및 커밋 미수행

    std::cout << " -> All request_service scenarios: PASSED\n";

    std::cout << "\n========================================\n";
    std::cout << " All Public Interface Tests Succeeded! \n";
    std::cout << "========================================\n";

    // main 함수가 종료되면서 monitor의 소멸자(~tp_monitor)가 호출되고,
    // 등록된 워커 스레드들이 정상적으로 join 처리됩니다.
    return 0;
}
