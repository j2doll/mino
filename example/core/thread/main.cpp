#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <string>

#include "mino/core/thread/dynamic_thread.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 1. thread_task 인터페이스 구현 클래스 (performTask, ~thread_task 가상 소멸자 검증용)
class TestTask : public mino::core::thread::thread_task {
public:
    explicit TestTask(std::string name) : name_(std::move(name)) {
        print(tce("[TestTask 생성] " + name_));
    }

    ~TestTask() override {
        print(tce("[TestTask 소멸] " + name_));
    }

    void performTask() override {
        print(tce("[TestTask - " + name_ + "] performTask 실행 (Count: " + std::to_string(++count_) + ")"));
    }

    int getCount() const { return count_; }

private:
    std::string name_;
    int count_{ 0 };
};



int main() {
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
        print(tce("초기 isRunning 상태: " + std::string(th.isRunning() ? "true" : "false")));

        // 1-2. set_logger() 등록
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
        auto logger = std::make_shared<mclt::logger>("server_logger");
        logger->add_sink(std::make_shared<mclt::console_sink>("console", ccfg));
        th.set_logger(logger);
        print(tce("로거 등록 완료"));

        // 1-3. setInterval() 설정
        th.setInterval(80ms);
        print(tce("주기 설정: 80ms"));

        // 1-4. start() - 가변 인자를 가진 일반 함수 바인딩
        print(tce("start(freeFunctionWithArgs, \"TestParam\", 100) 호출"));

        // 쓰레드 시작
        th.start([](const std::string& prefix, int value) {
                print(tce("[freeFunction] " + prefix + " / Value: " + std::to_string(value)));
            },
            "TestParam", 100);

        print(tce("실행 중 isRunning 상태: "), std::string(th.isRunning() ? "true" : "false"));

        // 1-5. 중복 start() 호출 시도 -> 등록된 로거(s_logger_)를 통한 경고 출력 확인
        print(tce("중복 start() 호출 시도 (로거 경고 발생 확인용)..."));
        th.start([]() { print(tce("실행되지 않아야 하는 람다")); });

        std::this_thread::sleep_for(250ms);

        // 1-6. stop() 호출
        print(tce("stop() 호출"));
        th.stop();
        print(tce("stop 후 isRunning 상태: " + std::string(th.isRunning() ? "true" : "false")));

        // 1-7. set_logger(nullptr) 등록 해제 검증
        th.set_logger(nullptr);
        print(tce("로거 등록 해제 완료 (nullptr)"));
    }
    print(endl);

    // ====================================================
    // Test 2: start(Callable) 람다 및 setInterval 변경 테스트
    // ====================================================
    print(tce("--- [Test 2] 람다 start(), setInterval() 주기 변경 동작 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th;
        th.setInterval(50ms);

        int counter = 0;
        print(tce("50ms 주기로 람다 start()"));
        th.start([&counter]() {
            print(tce("[람다 작업] Counter: " + std::to_string(++counter)));
        });

        std::this_thread::sleep_for(160ms);

        print(tce("실행 중 setInterval(150ms)로 주기 변경"));
        th.setInterval(150ms);

        std::this_thread::sleep_for(320ms);
        th.stop();
    }
    print(endl);

    // ====================================================
    // Test 3: start(thread_task&) 참조 오버로드 테스트
    // ====================================================
    print(tce("--- [Test 3] start(thread_task&) 참조 전달 오버로드 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th;
        th.setInterval(100ms);

        TestTask taskByRef("Ref_Instance");

        print(tce("start(taskByRef) 참조 전달 호출"));
        th.start(taskByRef);

        std::this_thread::sleep_for(250ms);
        th.stop();
        print(tce("참조 객체 작업 완료 (최종 실행 횟수: " + std::to_string(taskByRef.getCount()) + ")"));
    }
    print(endl);

    // ====================================================
    // Test 4: start(std::shared_ptr<thread_task>) 스마트 포인터 수명 유지 및 ~dynamic_thread() 소멸자 검증
    // ====================================================
    print(tce("--- [Test 4] start(shared_ptr<thread_task>) 및 소멸자(~dynamic_thread) 자동 stop 검증 ---"));
    {
        using dynamic_thread = mino::core::thread::dynamic_thread;

        dynamic_thread th;
        th.setInterval(100ms);

        {
            // 내부 스코프에서 생성된 shared_ptr
            auto spTask = std::make_shared<TestTask>("Shared_Instance");
            print(tce("start(spTask) 전달 후 로컬 shared_ptr 스코프 종료 진입"));
            th.start(spTask);
            // 여기서 spTask 변수는 파괴되지만, th 내부 task_obj_에 의해 객체 수명이 유지되어야 함
        }
        print(tce("로컬 spTask 스코프 벗어남 -> 객체는 소멸되지 않고 스레드에서 계속 실행되어야 함"));

        std::this_thread::sleep_for(250ms);

        print(tce("dynamic_thread 스코프 종료 -> 소멸자에서 stop() 자동 호출 및 task_obj_ 해제 유도"));
    }
    print(endl);

    print(tce("========================================="));
    print(tce("        모든 퍼블릭 멤버 테스트 완료        "));
    print(tce("========================================="));

    return 0;
}
