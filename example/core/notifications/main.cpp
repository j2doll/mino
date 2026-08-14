#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include <stdexcept>

#include "mino/core/notifications/notifications_event.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 1. 기본 구독 및 알림 테스트
void test_basic_subscription() {
    namespace notifications = mino::core::notifications;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test 1] 기본 구독 및 알림 테스트 시작...\n");

    // 이벤트 객체 생성. int와 std::string을 인자로 받는 이벤트.
    notifications::event<int, const std::string&> on_user_action;

    int received_id = 0;
    std::string received_msg;

    // 구독
    on_user_action.subscribe([&](int id, const std::string& msg) {
        received_id = id;
        received_msg = msg;

        std::cout
            << to_console_encoding("  -> 이벤트 수신 (ID: ")
            << received_id
            << to_console_encoding(", 메시지: ")
            << to_console_encoding(received_msg)
            << ")" << std::endl;
    });

    // 이벤트 발생
    on_user_action.notify(101, "Hello World");

    // 수신된 값 확인
    assert(received_id == 101);
    assert(received_msg == "Hello World");

    std::cout
        << to_console_encoding("  -> 성공 (수신 ID: ")
        << received_id
        << to_console_encoding(", 메시지: ")
        << to_console_encoding(received_msg)
        << ")"
        << std::endl << std::endl;
}

// 2. 수동 구독 해제(unsubscribe) 테스트
void test_unsubscribe() {
    namespace notifications = mino::core::notifications;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test 2] 수동 구독 해제 테스트 시작...\n");

    // 이벤트 객체 생성. int를 인자로 받는 이벤트.
    notifications::event<int> on_score_changed;

    int call_count = 0;

    // 두 개의 콜백 구독 (id1, id2)
    auto id1 = on_score_changed.subscribe([&](int) { call_count += 1; });
    auto id2 = on_score_changed.subscribe([&](int) { call_count += 10; });

    on_score_changed.notify(5); // 이벤트 발생.
    // NOTE: 등록된 두 개의 콜백이 모두 호출되어야 함.

    assert(call_count == 11); // 0+1=1, 1+10=11

    // id1 구독 해제
    bool unsubscribed = on_score_changed.unsubscribe(id1);
    assert(unsubscribed == true);

    // 재발생 시 id2만 호출되어야 함
    on_score_changed.notify(5); // 이벤트 발생. 
    assert(call_count == 21); // 11 + 10 = 21

    // NOTE: 객체가 파괴되지 않는다면, 서로 다른 쓰레드에서 notify()와
    //  unsubscribe()가 동시에 호출되도 문제 없음. 

    std::cout
        << to_console_encoding("  -> 성공 (구독 해제 후 콜백 실행 횟수 정상 확인)\n\n");
}

// 3. RAII 기반 Scoped Connection 테스트
void test_scoped_connection() {
    namespace notifications = mino::core::notifications;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test 3] RAII Scoped Connection 테스트 시작...\n");

    // 이벤트 객체 생성. std::string을 인자로 받는 이벤트.
    notifications::event<std::string> on_message;

    int call_count = 0;
    {
        // 스코프 블록 내부에서 구독 등록 (subscribe_scoped)
        auto conn = on_message.subscribe_scoped([&](const std::string& msg) {
            call_count++;

            std::cout
                << to_console_encoding("  -> 이벤트 수신 (call_count: ")
                << call_count
                << to_console_encoding(", 메시지: ")
                << to_console_encoding(msg)
                << to_console_encoding(")\n");
        });

        assert(conn.connected() == true); // 구독이 활성화되어 있는가? 

        on_message.notify("첫 번째 알림"); // 이벤트 발생
        assert(call_count == 1); 
    }
    // conn 객체가 소멸하면서 자동으로 unsubscribe 처리됨

    on_message.notify("두 번째 알림"); // 이벤트 발생
    assert(call_count == 1); // 스코프 탈출 후에는 호출되지 않아야 함.

    std::cout
        << to_console_encoding("  -> 성공 (스코프 이탈 시 자동 구독 해제 확인)\n\n");
}

// 4. 예외 처리 테스트 (콜백 내부 예외 발생 시 다른 콜백에 영향을 주지 않는지)
void test_exception_handling() {
    namespace notifications = mino::core::notifications;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test 4] 예외 격리 처리 테스트 시작...\n");

    // 이벤트 객체 생성. 인자가 없는 이벤트.
    notifications::event<> on_error_test;

    bool second_listener_executed = false;

    // 첫 번째 콜백에서 예외 던짐
    on_error_test.subscribe([]() {
        throw std::runtime_error("의도된 예외 발생!");
    });

    // 두 번째 콜백
    on_error_test.subscribe([&]() {
        second_listener_executed = true;
    });

    // notify 실행
    try {
        on_error_test.notify();
    }
    catch (const std::exception& e) {
        // 처리 안됨
        std::cout
            << to_console_encoding("notify 호출부에서 캡처: ")
            << to_console_encoding(e.what())
            << std::endl;
    }
    // 첫 번째 콜백에서 예외가 발생했지만, 두 번째 콜백은 정상적으로 실행되어야 함.
    // catch(...) 안의 코드는 notify 내부에서 처리되고 예외가 전파되지 않음.

    assert(second_listener_executed == true);

    std::cout
        << to_console_encoding("  -> 성공 (콜백 예외 발생 시에도 다음 콜백 정상 수행 확인)\n\n");
}

// 5. 멀티스레드 동시성 테스트
void test_multithreading() {
    namespace notifications = mino::core::notifications;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test 5] 멀티스레드 동시성 테스트 시작...\n");

    // 이벤트 객체 생성. int를 인자로 받는 이벤트.
    notifications::event<int> concurrent_event;

    std::atomic<int> total_sum{ 0 };

    // 5개의 리스너 등록
    for (int i = 0; i < 5; ++i) {
        concurrent_event.subscribe([&total_sum](int value) {
            total_sum += value;
        });
    }

    // 10개의 스레드에서 동시에 notify 호출
    std::vector<std::thread> threads;
    threads.reserve(10); // 10개의 스레드 생성
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&concurrent_event]() { // 각 스레드에서 100회 notify 호출
            for (int j = 0; j < 100; ++j) {
                concurrent_event.notify(1);
            }
        });
    }

    // 모든 스레드가 종료될 때까지 대기
    for (auto& t : threads) {
        t.join();
    }

    // 예상 값: 10개 스레드 * 100회 호출 * 5개 리스너 * 값 1씩 증가 = 5,000
    assert(total_sum == 5000);

    std::cout
        << to_console_encoding("  -> 성공 (합계: ")
        << total_sum.load()
        << to_console_encoding(" / Expected: 5000)\n\n");
}

int main(int argc, char* argv[]) {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("========================================\n");
    std::cout << to_console_encoding(" mino::core::notifications::event 검증\n");
    std::cout << to_console_encoding("========================================\n\n");

    try {
        test_basic_subscription();
        test_unsubscribe();
        test_scoped_connection();
        test_exception_handling();
        test_multithreading();
    }
    catch (const std::exception& e) {
        // 예외 발생 시 메시지 출력 후 종료
        std::cerr
            << to_console_encoding("테스트 중 예외 발생: ")
            << to_console_encoding(e.what())
            << std::endl;
        return 1;
    }

    std::cout << to_console_encoding("========================================\n");
    std::cout
        << to_console_encoding("  모든 테스트를 성공적으로 통과했습니다!\n");
    std::cout << to_console_encoding("========================================\n");

    return 0;
}
