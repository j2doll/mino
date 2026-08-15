#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <limits>

#include "mino/core/singleton/singleton.hpp"
#include "mino/core/string/to_console_encoding.hpp"

namespace {
    // 테스트용 클래스 1: 상태 변경 및 주소 검증용
    class AppConfig {
    public:
        std::string app_name = "DefaultApp";
        int version = 1;

        void update_version(int new_version) {
            version = new_version;
        }
    };

    // 테스트용 클래스 2: 멀티스레드 안전성 검증용
    class Logger {
    protected:
        std::mutex mtx;
    public:
        int log_count = 0;

        void log(const std::string& msg) {
            auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
            std::ostream& (*endl)(std::ostream&) = std::endl;
            auto tce = mino::core::string::to_console_encoding;

            std::lock_guard<std::mutex> lock(mtx);
            ++log_count; // 로그 횟수 증가. 카운트는 검증용 데이터로 활용.
            if (log_count == std::numeric_limits<int>::max()) { log_count = 0; }
            print(tce("[LOG] " + msg + " (Count: " + std::to_string(log_count) + ")"));
        }
    };
}

void test_basic_and_address() {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    using ConfigSingleton = mino::core::singleton::singleton_wrapper<AppConfig>; 

    print(tce("--- 1. 기본 동작 및 인스턴스 주소 검증 ---"));

    AppConfig& instance1 = ConfigSingleton::get_instance(); // 싱글톤 인스턴스 가져오기
    AppConfig& instance2 = ConfigSingleton::get_instance(); // 싱글톤 인스턴스 가져오기
    // NOTE: 인스턴스를 가져올때 인스턴스 생성이 되어있지 않으면 해당 시점에 생성됨.

    // 동일한 메모리 주소를 가리키는지 확인
    print(tce("Instance 1 Addr: "), &instance1); // 포인터
    print(tce("Instance 2 Addr: "), &instance2); // 포인터
    assert(&instance1 == &instance2); // 싱글톤이므로 두 인스턴스의 포인터는 동일해야 한다.

    // 1번의 수정
    instance1.app_name = "MinoService";
    instance1.update_version(2);

    // 1번이 2번과 동일하므로, 2번에서 수정되었는지 확인
    assert(instance2.app_name == "MinoService");
    assert(instance2.version == 2);

    print(tce("-> 기본 동작 및 단일 인스턴스 검증 통과\n"));
}

void test_multithreading() {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    using LoggerSingleton = mino::core::singleton::singleton_wrapper<Logger>;

    print(tce("--- 2. 멀티스레드 환경 접근 검증 ---"));

    const int thread_count = 10;
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        workers.emplace_back([i]() { // 총 10개인 각 스레드에서 싱글톤 인스턴스 접근
            Logger& logger = LoggerSingleton::get_instance(); // 싱글톤 인스턴스 가져오기 
            logger.log("Thread #" + std::to_string(i) + " executed"); // 로그 메시지 기록
        });
    }

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    assert(LoggerSingleton::get_instance().log_count == thread_count);

    print(tce("-> 멀티스레드 환경 검증 통과 (총 로그 횟수: "),
        LoggerSingleton::get_instance().log_count, tce(")\n"));  
}

int main(int argc, char* argv[]) {
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    try {
        test_basic_and_address();
        test_multithreading();

        print(tce("모든 테스트가 성공적으로 완료되었습니다."));
    }
    catch (const std::exception& e) {
        eprint(tce("테스트 중 예외 발생: "), e.what());
        return 1;
    }

    return 0;
}

