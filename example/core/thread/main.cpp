#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <string>

#include "mino/core/thread/dynamic_thread.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/datetime/util/util.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 현재 시간(시분초.밀리초) 문자열 얻기
std::string t_str() {
    namespace dtutil = mino::core::datetime::util;
    using time_zone_mode = mino::core::datetime::util::time_zone_mode;
    auto current_tz = time_zone_mode::local_time;
    auto ret = dtutil::current_time_string(current_tz, "|hh:mm:ss.SSS| ");
    return ret;
}

// 1. thread_task 인터페이스 구현 클래스 (performTask, ~thread_task 가상 소멸자 검증용)
class TestTask : public mino::core::thread::thread_task {
public:
    explicit TestTask(std::string name) : name_(std::move(name)) { // 생성자
        /*LOG*/ print(t_str(), tce("[TestTask 생성] "), name_);
    }

    ~TestTask() override { // 소멸자
        /*LOG*/ print(t_str(), tce("[TestTask 소멸] "), name_);
    }

    void performTask() override { // 쓰레드의 업무 수행
        /*LOG*/ print(t_str(), tce("[TestTask - "), name_, tce("] performTask 실행 (Count: "),
            std::to_string(++count_), ")" );
    }

    int getCount() const {
        return count_;
    }

private:
    std::string name_;
    int count_{ 0 };
};

int main(int argc, char* argv[]) {
    using namespace std::chrono_literals;

    print(tce("========================================="));
    print(tce("   dynamic_thread 전체 퍼블릭 API 테스트   "));
    print(tce("========================================="), endl);

    // ====================================================
    // Test 1: 기본 생성자, set_logger(), start(Callable, Args...), isRunning(), stop()
    // ====================================================
    print(tce("--- [Test 1] 로거 등록, 템플릿 start(Callable, Args...), isRunning, stop ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th; // 동적 쓰레드 생성자 

        // 1-1. 초기 상태 검증 (isRunning)
        /*LOG*/ print(t_str(), tce("초기 isRunning 상태: " + std::string(th.isRunning() ? "true" : "false")));

        // 1-2. 로거 등록
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
        auto logger = std::make_shared<mclt::logger>("thread_logger");
        logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));
        th.set_logger(logger);
        /*LOG*/ print(t_str(), tce("로거 등록 완료"));

        // 1-3. setInterval() 설정
        auto interval = 1000;
        th.setInterval(std::chrono::milliseconds(interval)); // 동적 쓰레드 주기 설정.
        // NOTE: 주기는 등록된 함수를 실행 후 sleep하는 시간을 의미함.
        /*LOG*/ print(t_str(), tce("주기 설정: "), interval, tce(" 밀리초"));

        // 1-4. start() - 가변 인자를 가진 일반 함수 바인딩
        /*LOG*/ print(t_str(), tce("start([](prefix, value), \"TestParam\", 100) 호출"));
        th.start( // 동적 쓰레드 시작
            [](const std::string& prefix, int value) { // 람다 함수 바인딩
                print(t_str(), tce("[] prefix: "), prefix, tce(" / value: "), std::to_string(value));
            },
            "TestParam", 100); // 람다 함수 인자 전달 

        /*LOG*/ print(t_str(), tce("실행 중 isRunning 상태: "), std::string(th.isRunning() ? "true" : "false"));

        // 1-5. 중복 start() 호출 시도 -> 등록된 로거(s_logger_)를 통한 경고 출력 확인
        /*LOG*/ print(t_str(), tce("중복 start() 호출 시도 (로거 경고 발생 확인용)..."));
        th.start([]() { print(tce("실행되지 않아야 하는 람다")); });

        auto wait_time = interval * 3 + 100; // 3회 실행 후 stop() 호출
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

        // 1-6. stop() 호출
        /*LOG*/ print(t_str(), tce("stop() 호출"));
        th.stop(); // 동적 쓰레드 종료
        /*LOG*/ print(t_str(), tce("stop 후 isRunning 상태: "), std::string(th.isRunning() ? "true" : "false"));

        // 1-7. set_logger(nullptr) 등록 해제 검증
        th.set_logger(nullptr);
        /*LOG*/ print(t_str(), tce("로거 등록 해제 완료 (nullptr)"));
    }
    print(endl);

    // ====================================================
    // Test 2: start(Callable) 람다 및 setInterval 변경 테스트
    // ====================================================
    print(tce("--- [Test 2] 람다 start(), setInterval() 주기 변경 동작 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th; // 동적 쓰레드 인스턴스

        auto interval = 1000;
        th.setInterval(std::chrono::milliseconds(interval)); // 초기 주기

        int counter = 0;
        /*LOG*/ print(t_str(), interval, tce(" ms 주기로 람다 start()"));

        // 쓰레드 시작 
        th.start(
            [&counter]() { // 람다 함수
            /*LOG*/ print(t_str(), tce("[람다 작업] Counter: "), std::to_string(++counter));
        });

        auto wait_time = interval * 3 + 20; // 3회 실행 후 주기 변경
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

        auto new_interval = interval + 50;
        /*LOG*/ print(t_str(), tce("실행 중 setInterval("), new_interval, tce("ms)로 주기 변경"));
        th.setInterval(std::chrono::milliseconds(new_interval)); // 주기 변경

        auto wait_stop_time = new_interval * 3 + 20; // 3회 실행 후 종료
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_stop_time));
        th.stop(); // 동적 쓰레드 종료
     }
    print(endl);

    // ====================================================
    // Test 3: start(thread_task&) 참조 오버로드 테스트
    // ====================================================
    print(tce("--- [Test 3] start(thread_task&) 참조 전달 오버로드 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th; // 동적 쓰레드 인스턴스

        auto interval = 1000;
        th.setInterval(std::chrono::milliseconds(interval)); // 초기 주기
        /*LOG*/ print(t_str(), interval, tce(" ms 주기 설정"));

        TestTask taskByRef("Ref_Instance"); // thread_task 참조 전달용 객체 생성

        /*LOG*/ print(t_str(), tce("start(taskByRef) 참조 전달 호출"));
        th.start(taskByRef); // 참조 전달 오버로드 호출

        auto wait_time = interval * 3 + 20; // 3회 실행 후 stop() 호출
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        th.stop(); // 동적 쓰레드 종료
        /*LOG*/ print(t_str(), tce("참조 객체 작업 완료 (최종 실행 횟수: " + std::to_string(taskByRef.getCount()) + ")"));
    }
    print(endl);

    // ====================================================
    // Test 4: start(std::shared_ptr<thread_task>) 스마트 포인터 수명 유지 및 ~dynamic_thread() 소멸자 검증
    // ====================================================
    print(tce("--- [Test 4] start(shared_ptr<thread_task>) 및 소멸자(~dynamic_thread) 자동 stop 검증 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th; // 동적 쓰레드 인스턴스

        auto interval = 1000;
        th.setInterval(std::chrono::milliseconds(interval)); // 초기 주기 설정
        /*LOG*/ print(t_str(), interval, tce(" ms 주기 설정")); 

        {
            /*LOG*/ print(t_str(), tce("start(spTask) 전달 후 로컬 shared_ptr 스코프 종료 진입"));

            // 내부 스코프에서 생성된 shared_ptr
            auto spTask = std::make_shared<TestTask>("Shared_Instance");
            th.start(spTask);
            // NOTE: spTask 변수는 scope를 벗어나서 파괴되지만, th의 내부 task_obj_에 의해 객체 수명이 유지됨.
        }
        /*LOG*/ print(t_str(), tce("로컬 spTask 스코프 벗어남 -> 객체는 소멸되지 않고 스레드에서 계속 실행되어야 함"));

        auto wait_time = interval * 5 + 20; // 5회 실행 후 스코프 종료
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

        /*LOG*/ print(t_str(), tce("dynamic_thread 스코프 종료 -> 소멸자에서 stop() 자동 호출 및 task_obj_ 해제 유도"));
    }
    print(endl);

    print(tce("========================================="));
    print(tce("        모든 퍼블릭 멤버 테스트 완료        "));
    print(tce("========================================="));

    return 0;
}
