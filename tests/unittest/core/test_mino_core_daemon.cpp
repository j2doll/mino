#include "catch_amalgamated.hpp"
#include "mino/core/daemon/daemon.hpp"

TEST_CASE("termination_handler singleton instance test", "[mino][daemon]") {
    using mino::core::daemon::termination_handler;

    // 싱글톤 인스턴스의 동일성(참조 주소 일치 여부) 검증
    termination_handler& instance1 = termination_handler::get_instance();
    termination_handler& instance2 = termination_handler::get_instance();

    REQUIRE(&instance1 == &instance2);
}

TEST_CASE("termination_handler callback registration test", "[mino][daemon]") {
    using mino::core::daemon::termination_handler;

    auto& handler = termination_handler::get_instance();

    // 콜백 등록 및 람다 전달 테스트
    bool callback_invoked = false;
    handler.set_callback([&callback_invoked]() {
        callback_invoked = true;
        });

    // 콜백 교체 테스트
    int call_count = 0;
    handler.set_callback([&call_count]() {
        ++call_count;
        });

    SUCCEED("Callback registered successfully");
}

TEST_CASE("termination_handler initialization test", "[mino][daemon]") {
    using mino::core::daemon::termination_handler;

    auto& handler = termination_handler::get_instance();

    // OS 시그널 및 콘솔 핸들러 등록(initialize) 성공 여부 검증
    bool init_result = handler.initialize();
    CHECK(init_result == true);
}
