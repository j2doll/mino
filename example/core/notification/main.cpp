#include <iostream>
#include <string>
#include <memory>

#include "mino/core/notification/notification.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 1. 테스트용 커스텀 메시지 클래스 정의
// ==========================================

// 사용자 로그인 알림 메시지
class user_login_message : public mino::core::notification::message {
public:
    std::string user_name;
    int user_id;

    user_login_message(std::string name, int id)
        : user_name(std::move(name)), user_id(id) {
    }
};

// 시스템 경고 알림 메시지
class system_alert_message : public mino::core::notification::message {
public:
    std::string alert_text;
    int severity_level;

    system_alert_message(std::string text, int level)
        : alert_text(std::move(text)), severity_level(level) {
    }
};

// ==========================================
// 2. UserLogger 클래스 정의 (std::weak_ptr 사용)
// ==========================================
class UserLogger : public std::enable_shared_from_this<UserLogger> {
public:
    UserLogger() = default;

    ~UserLogger() {
        // 객체 파괴(reset()) 시, 등록했던 옵저버 해제
        if (m_obs_id != 0) {
            mino::core::notification::center::default_center().remove_observer("USER_LOGIN", m_obs_id);
        }
    }

    // 옵저버 등록 함수. 
    // NOTE: register_observer()는 std::make_shared<UserLogger>()로 객체를 생성한 이후,
    //  개발자가 코드에서 logger->register_observer()를 명시적으로 호출하는 시점에 알림 센터에 등록됨.
    // NOTICE: 본 함수를 생성자에서 호출되지 않도록 해야함.
    //  객체의 생성자(UserLogger())가 실행 중인 시점에는
    //  아직 해당 객체를 관리할 std::shared_ptr의 제어 블록이 완전히 만들어지지 않았습니다.
    //  따라서 생성자 내부에서 weak_from_this()나 shared_from_this()를 호출하면
    //  유효한 shared_ptr를 찾을 수 없어 std::bad_weak_ptr 예외가 발생하며 프로그램이 크래시 발생됨.
    void register_observer() {
        auto& center = mino::core::notification::center::default_center();
        using message = mino::core::notification::message;

        // weak_from_this()를 사용하여 weak_ptr 캡처
        std::weak_ptr<UserLogger> weak_self = weak_from_this();

        // USER_LOGIN 이벤트에 대한 옵저버 등록
        m_obs_id = center.add_observer("USER_LOGIN", [weak_self](const std::shared_ptr<message>& msg) {
            // 콜백 실행 시점에 객체가 살아있는지 검사
            if (auto self = weak_self.lock()) {
                self->on_user_login(msg); // 객체가 살아있으면 실제 처리 함수 호출
            }
            else {
                auto to_console_encoding = mino::core::string::to_console_encoding;
                std::cout
                    << to_console_encoding("[UserLogger] 객체가 파괴되어 알림을 처리하지 않고 무시합니다.")
                    << std::endl;
            }
        });
    }

    void on_user_login(const std::shared_ptr<mino::core::notification::message>& msg) {
        if (auto login_msg = std::dynamic_pointer_cast<user_login_message>(msg)) { // user_login_message 메시지이면,
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout
                << to_console_encoding("[UserLogger Class] 로그인 처리 완료: ")
                << to_console_encoding(login_msg->user_name)
                << std::endl;
        }
    }

private:
    mino::core::notification::center::observer_id_t m_obs_id = 0;
};


// ==========================================
// 3. 메인 테스트 함수
// ==========================================
int main() {
    using message = mino::core::notification::message;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    // 싱글톤 객체 참조 가져오기
    auto& center = mino::core::notification::center::default_center();

    std::cout
        << "=========================================="
        << std::endl;
    std::cout
        << to_console_encoding("  Notification Center 테스트 시작") << std::endl;
    std::cout
        << "=========================================="
        << std::endl;

    // ------------------------------------------
    // A. 옵저버 등록 (USER_LOGIN 이벤트)
    // ------------------------------------------
    auto obs1_id = center.add_observer("USER_LOGIN", [](const std::shared_ptr<message>& msg) {
        if (auto login_msg = std::dynamic_pointer_cast<user_login_message>(msg)) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout
                << to_console_encoding("[옵저버 1 - UI] 환영합니다, ")
                << to_console_encoding(login_msg->user_name)
                << to_console_encoding("님! (ID: ")
                << login_msg->user_id
                << to_console_encoding(")")
                << std::endl;
        }
    });

    auto obs2_id = center.add_observer("USER_LOGIN", [](const std::shared_ptr<message>& msg) {
        if (auto login_msg = std::dynamic_pointer_cast<user_login_message>(msg)) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout
                << to_console_encoding("[옵저버 2 - Logger] 로그인 로깅 기록 완료 (User: ")
                << to_console_encoding(login_msg->user_name)
                << to_console_encoding(")")
                << std::endl;
        }
    });

    // ------------------------------------------
    // B. 옵저버 등록 (SYSTEM_ALERT 이벤트)
    // ------------------------------------------
    center.add_observer("SYSTEM_ALERT", [](const std::shared_ptr<message>& msg) {
        if (auto alert_msg = std::dynamic_pointer_cast<system_alert_message>(msg)) {
            auto to_console_encoding = mino::core::string::to_console_encoding;
            std::cout
                << to_console_encoding("[경고 옵저버 - System] 경고 발생! 레벨: ")
                << alert_msg->severity_level
                << to_console_encoding(" / 내용: ")
                << to_console_encoding(alert_msg->alert_text)
                << std::endl;
        }
    });

    // ------------------------------------------
    // C. 알림 발송 테스트 1
    // ------------------------------------------
    std::cout
        << to_console_encoding("\n[테스트 1] 'USER_LOGIN' 알림 발송 (Alice)")
        << std::endl;
    auto login_msg1 = std::make_shared<user_login_message>("Alice", 1001);
    center.post_notification("USER_LOGIN", login_msg1);
    // 
    // [테스트 1] 'USER_LOGIN' 알림 발송 (Alice)
    // [옵저버 1 - UI] 환영합니다, Alice님! (ID: 1001)
    // [옵저버 2 - Logger] 로그인 로깅 기록 완료 (User: Alice)

    std::cout
        << to_console_encoding("\n[테스트 2] 'SYSTEM_ALERT' 알림 발송")
        << std::endl;
    auto alert_msg = std::make_shared<system_alert_message>("메모리 사용량 90% 초과", 3);
    center.post_notification("SYSTEM_ALERT", alert_msg);
    // 
    // [테스트 2] 'SYSTEM_ALERT' 알림 발송
    // [경고 옵저버 - System] 경고 발생! 레벨: 3 / 내용: 메모리 사용량 90% 초과

    // ------------------------------------------
    // D. 옵저버 해제 및 발송 테스트 2
    // ------------------------------------------
    std::cout
        << to_console_encoding("\n[테스트 3] 옵저버 1(UI) 해제 후 'USER_LOGIN' 알림 발송 (Bob)")
        << std::endl;
    center.remove_observer("USER_LOGIN", obs1_id);

    auto login_msg2 = std::make_shared<user_login_message>("Bob", 1002);
    center.post_notification("USER_LOGIN", login_msg2);
    // 
    // [테스트 3] 옵저버 1(UI)해제 후 'USER_LOGIN' 알림 발송(Bob)
    // [옵저버 2 - Logger] 로그인 로깅 기록 완료(User: Bob)

    // ------------------------------------------
    // E. UserLogger 클래스 활용 테스트 (weak_ptr 생명주기 검증)
    // ------------------------------------------
    std::cout
        << to_console_encoding("\n[테스트 4] UserLogger 클래스 생성 및 옵저버 등록 후 발송 (Charlie)")
        << std::endl;

    // UserLogger 객체 생성을 위해 shared_ptr 필수 (enable_shared_from_this 동작 조건)
    auto logger = std::make_shared<UserLogger>();
    logger->register_observer(); // 옵저버 등록 (생성자 이후에 호출)

    auto login_msg3 = std::make_shared<user_login_message>("Charlie", 1003);
    center.post_notification("USER_LOGIN", login_msg3);
    // 
    // [테스트 4] UserLogger 클래스 생성 및 옵저버 등록 후 발송 (Charlie)
    // [옵저버 2 - Logger] 로그인 로깅 기록 완료 (User: Charlie)
    // [UserLogger Class] 로그인 처리 완료: Charlie

    std::cout
        << to_console_encoding("\n[테스트 5] UserLogger 객체 파괴(reset) 후 'USER_LOGIN' 알림 발송 (David)")
        << std::endl;

    // UserLogger 객체 파괴 (소멸자에서 remove_observer가 자동 호출됨)
    logger.reset();

    auto login_msg4 = std::make_shared<user_login_message>("David", 1004);
    center.post_notification("USER_LOGIN", login_msg4);
    //
    // [테스트 5] UserLogger 객체 파괴(reset) 후 'USER_LOGIN' 알림 발송 (David)
    // [옵저버 2 - Logger] 로그인 로깅 기록 완료 (User: David)

    std::cout
        << to_console_encoding("\n==========================================")
        << std::endl;
    std::cout
        << to_console_encoding("  Notification Center 테스트 완료")
        << std::endl;
    std::cout
        << to_console_encoding("==========================================")
        << std::endl;

    return 0;
}
