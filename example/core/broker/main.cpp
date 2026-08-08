#include <cassert>   
#include <memory>    
#include <string>    
#include <vector>    
#include <algorithm> 
#include <sstream>   
#include <thread>    
#include <atomic>    
#include <chrono>    

#include "mino/core/broker/broker.hpp" 

//----------------------------------------------------------------

int test_object_broker();
int unit_test_object_broker();

int main(int argc, char* argv[]) {
    std::cout << "=== test_object_broker ===" << std::endl << std::endl;
    test_object_broker();

    std::cout << std::endl << std::endl;

    std::cout << "=== unit_test_object_broker ===" << std::endl;
    unit_test_object_broker();
    return 0;
}

//----------------------------------------------------------------

int test_object_broker() {
    using object_broker = mino::core::broker::object_broker;  

    // 테스트용 샘플 클래스들
    struct ServiceA {
        std::string name;                             // 서비스의 이름을 저장
        explicit ServiceA(std::string n) : name(std::move(n)) {} // 생성자: 이름 초기화
    };

    struct ServiceB {
        int id;                                       // 서비스 식별자 저장
        explicit ServiceB(int i) : id(i) {}           // 생성자: id 초기화
    };

    std::cout << "[1] Testing internal structs (key, key_hash)..." << std::endl;
    {
        // 키 생성: 동일 타입/이름 k1, k2와 다른 타입 k3
        object_broker::key k1{ typeid(ServiceA), "test" }; // ServiceA 타입, 이름 "test"
        object_broker::key k2{ typeid(ServiceA), "test" }; // 동일한 키 (동치여야 함)
        object_broker::key k3{ typeid(ServiceB), "test" }; // 다른 타입의 키

        assert(k1 == k2);                     // k1과 k2는 같은 타입과 이름 -> 같아야 함
        assert(!(k1 == k3));                  // k1과 k3는 타입이 다르므로 같지 않아야 함

        object_broker::key_hash hasher;       // 키 해시 생성기
        assert(hasher(k1) == hasher(k2));    // 동치 키는 동일한 해시값을 생성해야 함
        std::cout << " -> key and key_hash verification passed\n" << std::endl;
    }

    std::cout << "[2] Testing instance registration and lookup (register_instance, get, contains, get_optional)..." << std::endl;
    {
        // 서비스 인스턴스 생성: 기본 등록용, 이름이 있는 등록용, 다른 타입용
        auto serviceA1 = std::make_shared<ServiceA>("ServiceA_Default"); // 기본 이름으로 등록할 인스턴스
        auto serviceA2 = std::make_shared<ServiceA>("ServiceA_Named");   // 커스텀 이름으로 등록할 인스턴스
        auto serviceB1 = std::make_shared<ServiceB>(100);                // ServiceB 인스턴스

        // A1을 기본 이름으로 등록 (내부적으로 "__default__" 사용)
        object_broker::register_instance<ServiceA>(serviceA1);

        // A2와 B1을 커스텀 이름으로 등록
        object_broker::register_instance<ServiceA>("custom_a", serviceA2);
        object_broker::register_instance<ServiceB>("custom_b", serviceB1);

        // contains() 검증: 등록된 항목들이 존재해야 함
        assert(object_broker::contains<ServiceA>());               // 기본 ServiceA 존재
        assert(object_broker::contains<ServiceA>("custom_a"));    // custom_a 이름 존재
        assert(object_broker::contains<ServiceB>("custom_b"));    // custom_b 이름 존재
        assert(!object_broker::contains<ServiceA>("non_existent")); // 없는 이름은 false

        // get() 검증: 올바른 포인터가 반환되는지 확인
        assert(object_broker::get<ServiceA>() == serviceA1);              // 기본 get == serviceA1
        assert(object_broker::get<ServiceA>("custom_a") == serviceA2);    // 이름으로 조회 == serviceA2
        assert(object_broker::get<ServiceB>("custom_b") == serviceB1);    // ServiceB 조회
        assert(object_broker::get<ServiceA>("non_existent") == nullptr);  // 존재하지 않으면 nullptr

        // get_optional() 검증: optional 형태로 존재 유무 확인
        auto opt1 = object_broker::get_optional<ServiceA>();
        assert(opt1.has_value() && opt1.value() == serviceA1); // 값 존재 및 기본 인스턴스와 일치

        auto opt2 = object_broker::get_optional<ServiceA>("non_existent");
        assert(!opt2.has_value()); // 없는 이름은 optional이 비어있어야 함

        std::cout << " -> Instance registration and lookup passed\n" << std::endl;
    }

    std::cout << "[3] Testing listing and collection APIs..." << std::endl;
    {
        // get_all: 특정 타입의 모든 인스턴스를 벡터로 반환
        auto all_a = object_broker::get_all<ServiceA>();
        assert(all_a.size() == 2); // 위에서 기본 + custom_a 두 개 등록됨

        // get_all_with_names: (이름, 인스턴스) 쌍을 반환
        auto all_named_a = object_broker::get_all_with_names<ServiceA>();
        assert(all_named_a.size() == 2);

        // list_names_for_type: 특정 타입에 대한 모든 이름 반환
        auto names_a = object_broker::list_names_for_type<ServiceA>();
        assert(names_a.size() == 2);

        // list_all_names: 모든 타입/이름 엔트리의 이름 리스트 반환
        auto all_names = object_broker::list_all_names();
        assert(all_names.size() == 3); // "__default__", "custom_a", "custom_b"

        // list_unique_names: 이름 중복 제거 후 리스트 반환
        auto unique_names = object_broker::list_unique_names();
        assert(!unique_names.empty()); // 최소 하나 이상의 유니크 이름 존재

        // list_all_entries: 모든 (typeid, name) 쌍 반환
        auto all_entries = object_broker::list_all_entries();
        assert(all_entries.size() == 3);

        std::cout << " -> Listing and collection APIs passed\n" << std::endl;
    }

    std::cout << "[4] Testing unregistering and clearing (unregister_instance, clear)..." << std::endl;
    {
        // 특정 이름으로 등록 해제
        object_broker::unregister_instance<ServiceA>("custom_a");
        assert(!object_broker::contains<ServiceA>("custom_a")); // custom_a는 제거되어야 함
        assert(object_broker::contains<ServiceA>());            // 기본 인스턴스는 남아 있어야 함

        // 전체 삭제: 모든 등록 항목 제거
        object_broker::clear();
        assert(!object_broker::contains<ServiceA>());          // 모든 ServiceA 제거됨
        assert(!object_broker::contains<ServiceB>("custom_b"));// ServiceB도 제거됨
        assert(object_broker::list_all_names().empty());       // 이름 목록이 비어야 함

        std::cout << " -> Unregister and clear passed\n" << std::endl;
    }

    std::cout << "[5] Testing RAII guard (registration_guard)..." << std::endl;
    {
        auto temp_service = std::make_shared<ServiceA>("TempService"); // 스코프 기반 등록 테스트

        {
            // 기본 이름으로 등록되는 RAII 가드 생성
            object_broker::registration_guard<ServiceA> guard1(temp_service);
            // "scoped_a" 이름으로 등록되는 가드 생성
            object_broker::registration_guard<ServiceA> guard2("scoped_a", temp_service);

            assert(object_broker::contains<ServiceA>());            // 기본 등록 존재
            assert(object_broker::contains<ServiceA>("scoped_a")); // scoped_a 등록 존재
        } // guard1, guard2 소멸 -> 자동으로 unregister_instance 호출

        // 스코프 종료 후 자동 제거 확인
        assert(!object_broker::contains<ServiceA>());
        assert(!object_broker::contains<ServiceA>("scoped_a"));

        std::cout << " -> RAII registration_guard destructor passed\n" << std::endl;
    }

    std::cout << "=== All public members of object_broker verified successfully ===" << std::endl;

    return 0;
}

//----------------------------------------------------------------

// 익명 네임스페이스: 이 파일 내부에서만 사용하는 헬퍼 타입들
namespace {

    struct Foo {
        explicit Foo(int v)
            : value(v) {
        }
        int value; // 테스트용 정수 값
    };

    class config_service {
    public:
        void print() { // 간단한 동작: 메시지 출력
            std::cout << "success to load configuration"; 
        } 
    };

    // 실패 카운트를 모아두는 전역 변수 (테스트에서 누적하여 출력)
    static int g_failures = 0;

    // 기대치 헬퍼: 조건이 거짓이면 실패 카운트 증가 및 메시지 출력
    inline void ExpectTrue(bool cond, const std::string& msg) {
        if (!cond) {
            ++g_failures;
            std::cerr << "EXPECT_TRUE failed: " << msg << std::endl;
        }
    }

    // 기대치 헬퍼: 두 값이 같아야 함 (동치 비교)
    template <typename T, typename U>
    inline void ExpectEq(const T& a, const U& b, const std::string& msg) {
        if (!(a == b)) {
            ++g_failures;
            std::cerr << "EXPECT_EQ failed: " << msg << " (lhs != rhs)" << std::endl;
        }
    }

    // 기대치 헬퍼: 두 값이 같지 않아야 함 (부등호)
    template <typename T, typename U>
    inline void ExpectNe(const T& a, const U& b, const std::string& msg) {
        if (a == b) {
            ++g_failures;
            std::cerr << "EXPECT_NE failed: " << msg << " (lhs == rhs)" << std::endl;
        }
    }

} // anonymous namespace
                                                                                
int unit_test_object_broker() {
    using object_broker = mino::core::broker::object_broker; 

    std::cout << "=== RegisterAndGetDefault ===" << std::endl;
    object_broker::clear(); // 이전 상태 정리
    {
        auto inst = std::make_shared<Foo>(42);         // Foo의 value가 42인 인스턴스 생성    
        object_broker::register_instance<Foo>(inst);   // 기본 이름으로 등록

        auto got = object_broker::get<Foo>();          // 기본으로 등록된 객체 조회
        ExpectNe(got, nullptr, "RegisterAndGetDefault: got should not be nullptr");
        if (got)
            ExpectEq(got->value, 42, "RegisterAndGetDefault: value should be 42");

        object_broker::clear(); // 테스트 종료 후 정리
    }

    std::cout << "=== RegisterAndGetNamed ===" << std::endl;
    object_broker::clear();
    {
        auto defaultInst1 = std::make_shared<Foo>(1);   // Foo의 value가 1인 인스턴스 생성
        auto namedInst7 = std::make_shared<Foo>(7);     // Foo의 value가 7인 인스턴스 생성

        object_broker::register_instance<Foo>(defaultInst1);          // 디폴트 이름으로 등록
        object_broker::register_instance<Foo>("special", namedInst7); // "special"이라는 이름으로 등록

        auto gotDefault = object_broker::get<Foo>();        // 기본 조회
        auto gotNamed = object_broker::get<Foo>("special"); // 이름으로 조회

        ExpectNe(gotDefault, nullptr, "RegisterAndGetNamed: default not null");
        ExpectNe(gotNamed, nullptr, "RegisterAndGetNamed: named not null");
        if (gotDefault)
            ExpectEq(gotDefault->value, 1, "RegisterAndGetNamed: default value");
        if (gotNamed)
            ExpectEq(gotNamed->value, 7, "RegisterAndGetNamed: named value");

        object_broker::clear();
    }

    std::cout << "=== UnregisterInstance ===" << std::endl;
    object_broker::clear();
    {
        auto namedInst = std::make_shared<Foo>(99); // Foo의 value가 99인 인스턴스 생성
        object_broker::register_instance<Foo>("to_remove", namedInst); // "to_remove"이라는 이름으로 임시 등록

        auto got = object_broker::get<Foo>("to_remove"); // "to_remove"라는 이름의 Foo 인스턴스가 있는지 조회
        ExpectNe(got, nullptr, "UnregisterInstance: before removal not null");
        if (got)
            ExpectEq(got->value, 99, "UnregisterInstance: before removal value");

        object_broker::unregister_instance<Foo>("to_remove"); // 이름으로 제거
        auto after = object_broker::get<Foo>("to_remove");
        ExpectEq(after, nullptr, "UnregisterInstance: after removal should be null");

        object_broker::clear();
    }

    std::cout << "=== ClearRemovesAll ===" << std::endl;
    object_broker::clear();
    {
        auto a = std::make_shared<Foo>(5); // Foo의 value가 5인 인스턴스 생성
        auto b = std::make_shared<Foo>(6); // Foo의 value가 6인 인스턴스 생성

        object_broker::register_instance<Foo>(a);               // 디폴트 이름으로 등록
        object_broker::register_instance<Foo>("other", b);      // "other"이라는 이름으로 등록

        ExpectNe(object_broker::get<Foo>(), nullptr, "ClearRemovesAll: default exists");
        ExpectNe(object_broker::get<Foo>("other"), nullptr, "ClearRemovesAll: other exists");

        object_broker::clear(); // 전체 제거

        ExpectEq(object_broker::get<Foo>(), nullptr, "ClearRemovesAll: default removed");
        ExpectEq(object_broker::get<Foo>("other"), nullptr, "ClearRemovesAll: other removed");
    }

    std::cout << "=== SameInstanceMultipleNames ===" << std::endl;
    object_broker::clear();
    {
        auto sharedInst = std::make_shared<Foo>(123); // Foo의 value가 123인 인스턴스 생성

        // 같은 인스턴스를 여러 이름으로 등록 (에일리어스)
        object_broker::register_instance<Foo>(sharedInst);               // 디폴트 이름으로 등록
        object_broker::register_instance<Foo>("alias_a", sharedInst);    // "alias_a"이라는 이름으로 등록
        object_broker::register_instance<Foo>("alias_b", sharedInst);    // "alias_b"이라는 이름으로 등록

        auto gotDefault = object_broker::get<Foo>(); // 기본 이름으로 조회
        auto gotA = object_broker::get<Foo>("alias_a"); // "alias_a" 이름으로 조회
        auto gotB = object_broker::get<Foo>("alias_b"); // "alias_b" 이름으로 조회

        ExpectNe(gotDefault, nullptr, "SameInstanceMultipleNames: default not null");
        ExpectNe(gotA, nullptr, "SameInstanceMultipleNames: a not null");
        ExpectNe(gotB, nullptr, "SameInstanceMultipleNames: b not null");

        if (gotDefault) ExpectEq(gotDefault->value, 123, "SameInstanceMultipleNames: default value");
        if (gotA) ExpectEq(gotA->value, 123, "SameInstanceMultipleNames: a value");
        if (gotB) ExpectEq(gotB->value, 123, "SameInstanceMultipleNames: b value");

        ExpectEq(gotDefault, gotA, "SameInstanceMultipleNames: default == a"); // 포인터 동일성 확인
        ExpectEq(gotA, gotB, "SameInstanceMultipleNames: a == b");

        object_broker::clear();
    }

    std::cout << "=== ConfigServiceExample (capture std::cout) ===" << std::endl;
    object_broker::clear();
    {
        auto service = std::make_shared<config_service>(); // config_service 인스턴스 생성
        object_broker::register_instance<config_service>(service); // config_service를 디폴트 이름으로 등록

        auto current_service = object_broker::get<config_service>();
        ExpectNe(current_service, nullptr, "ConfigServiceExample: service registered");

        // std::cout을 캡처하여 load()의 출력 확인
        std::stringstream buffer;
        auto old_buf = std::cout.rdbuf(buffer.rdbuf()); // 출력 버퍼 교체
        current_service->print();                         // print() 호출 -> 메시지 출력
        std::cout.rdbuf(old_buf);                        // 이전 버퍼로 복구

        std::string output = buffer.str();
        ExpectTrue(output.find("success to load configuration") != std::string::npos,
            "ConfigServiceExample: output contains expected message");

        // 백업 서비스 등록 및 확인
        object_broker::register_instance<config_service>("backup", std::make_shared<config_service>());
        auto backup_service = object_broker::get<config_service>("backup");
        ExpectNe(backup_service, nullptr, "ConfigServiceExample: backup service registered");

        object_broker::clear();
    }

    std::cout << "=== MultithreadedUsage ===" << std::endl;
    object_broker::clear();
    {
        const int thread_count = 8;       // 생성할 워커 스레드 수
        const int iterations = 200;       // 각 스레드가 수행할 읽기 반복 수
        auto sharedInst = std::make_shared<Foo>(777); // 모든 워커가 참조할 공유 인스턴스
        std::atomic<int> success_count{ 0 }; // 성공 카운터 (원자)
        std::atomic<int> fail_count{ 0 };    // 실패 카운터 (원자)
        std::vector<std::thread> workers;
        workers.reserve(thread_count); // 스레드 벡터 예약

        for (int i = 0; i < thread_count; ++i) {
            workers.emplace_back([i, iterations, &sharedInst, &success_count, &fail_count]() {
                const std::string name = "worker_" + std::to_string(i);
                object_broker::register_instance<Foo>(name, sharedInst); // 스레드별 이름으로 등록
                for (int k = 0; k < iterations; ++k) {
                    auto got = object_broker::get<Foo>(name); // 이름으로 조회
                    if (got && got == sharedInst && got->value == 777) {
                        ++success_count; // 일관된 값이면 성공
                    }
                    else {
                        ++fail_count;    // 이상 발생 시 실패 카운트
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50)); // 잠시 대기
                }
            });
        }

        for (auto& t : workers)
            t.join(); // 모든 워커 종료 대기

        ExpectEq(fail_count.load(), 0, "MultithreadedUsage: no failures expected");
        ExpectEq(success_count.load(), thread_count * iterations, "MultithreadedUsage: all reads succeeded");

        // 제거자 스레드들: 등록된 이름들 삭제
        std::vector<std::thread> removers;
        for (int i = 0; i < thread_count; ++i) {
            removers.emplace_back([i]() {
                const std::string name = "worker_" + std::to_string(i);
                object_broker::unregister_instance<Foo>(name); // 이름으로 해제
            });
        }

        for (auto& t : removers)
            t.join(); // 모든 제거자 스레드 종료 대기

        // 제거 후에는 get이 nullptr을 반환해야 함
        for (int i = 0; i < thread_count; ++i) {
            const std::string name = "worker_" + std::to_string(i);
            ExpectEq(object_broker::get<Foo>(name), nullptr, "MultithreadedUsage: removed names should be null");
        }
    }

    std::cout << "=== SingleRegistrarMultiReaders ===" << std::endl;
    object_broker::clear();
    {
        const int reader_count = 8;     // 읽기 전용 스레드 수
        const int iterations = 500;     // 각 리더가 반복할 횟수
        auto sharedInst = std::make_shared<Foo>(314); // 등록되는 단일 인스턴스

        std::atomic<bool> registered{ false }; // 등록 완료 플래그
        std::atomic<int> success_count{ 0 };   // 성공 카운터
        std::atomic<int> fail_count{ 0 };      // 실패 카운터

        std::vector<std::thread> readers;
        readers.reserve(reader_count); // 리더 스레드 벡터 예약

        for (int i = 0; i < reader_count; ++i) {
            // 각 리더 스레드는 등록 완료 플래그가 true가 될 때까지 대기 후, 반복적으로 get() 호출
            readers.emplace_back([&registered, &sharedInst, &success_count, &fail_count, iterations]() {
                while (!registered.load(std::memory_order_acquire)) { // 등록될 때까지 대기
                    std::this_thread::yield();
                }

                for (int k = 0; k < iterations; ++k) {
                    auto got = object_broker::get<Foo>(); // 기본으로 등록된 객체 조회
                    if (got && got == sharedInst && got->value == 314) {
                        ++success_count;
                    }
                    else {
                        ++fail_count;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(5));
                }
            });
        }

        // 등록자 스레드: 잠시 지연 후 단일 인스턴스를 등록하고 플래그를 설정
        std::thread registrar([&sharedInst, &registered]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 리더들이 대기하도록 잠깐 지연
            object_broker::register_instance<Foo>(sharedInst);         // 단일 등록자 등록
            registered.store(true, std::memory_order_release);         // 플래그 설정
        });

        registrar.join(); // 등록자 스레드 종료 대기
        for (auto& t : readers)
            t.join(); // 모든 리더 스레드 종료 대기

        ExpectEq(fail_count.load(), 0, "SingleRegistrarMultiReaders: no failures");
        ExpectEq(success_count.load(), reader_count * iterations, "SingleRegistrarMultiReaders: all reads succeeded");

        object_broker::clear();
    }

    std::cout << "=== MainRegisters_TwoReadersAccess ===" << std::endl;
    object_broker::clear();
    {
        auto a = std::make_shared<Foo>(42); // Foo의 value가 42인 인스턴스 생성
        object_broker::register_instance<Foo>(a); // 디폴트 이름으로 등록

        const int reads_per_thread = 1000;
        std::atomic<int> success_count{ 0 };
        std::atomic<int> fail_count{ 0 };

        // 두 개의 리더 스레드 생성: 동시에 get() 호출
        std::thread reader1([&]() {
            for (int i = 0; i < reads_per_thread; ++i) {
                auto p = object_broker::get<Foo>();
                if (p && p == a && p->value == 42)
                    ++success_count;
                else
                    ++fail_count;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });

        // 두 번째 리더 스레드
        std::thread reader2([&]() {
            for (int i = 0; i < reads_per_thread; ++i) {
                auto p = object_broker::get<Foo>();
                if (p && p == a && p->value == 42)
                    ++success_count;
                else
                    ++fail_count;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });

        reader1.join(); // 첫 번째 리더 스레드 종료 대기
        reader2.join(); // 두 번째 리더 스레드 종료 대기

        ExpectEq(fail_count.load(), 0, "MainRegisters_TwoReadersAccess: no failures");
        ExpectEq(success_count.load(), reads_per_thread * 2, "MainRegisters_TwoReadersAccess: all reads succeeded");

        object_broker::clear();
    }

    std::cout << "=== ContainsChecksPresence ===" << std::endl;
    object_broker::clear();
    {
        auto inst = std::make_shared<Foo>(11); // Foo의 value가 11인 인스턴스 생성
        object_broker::register_instance<Foo>("present", inst); // "present" 이름으로 등록

        ExpectTrue(object_broker::contains<Foo>("present"), "ContainsChecksPresence: present should be true"); // "present" 존재 여부 확인 (존재함)
        ExpectTrue(!object_broker::contains<Foo>("absent"), "ContainsChecksPresence: absent should be false"); // "absent" 존재 여부 확인 (없음)

        object_broker::unregister_instance<Foo>("present"); // "present" 제거
        ExpectTrue(!object_broker::contains<Foo>("present"), "ContainsChecksPresence: present removed"); // 제거 후 존재 여부 확인

        object_broker::clear();
    }

    std::cout << "=== GetAllReturnsAllInstancesForType ===" << std::endl;
    object_broker::clear();
    {
        auto a = std::make_shared<Foo>(1);
        auto b = std::make_shared<Foo>(2);
        auto c = std::make_shared<Foo>(3);

        object_broker::register_instance<Foo>("a", a);
        object_broker::register_instance<Foo>("b", b);
        object_broker::register_instance<Foo>("c", c);

        auto all = object_broker::get_all<Foo>(); // 타입 Foo의 모든 인스턴스 수집
        ExpectEq(all.size(), 3u, "GetAllReturnsAllInstancesForType: size 3");

        bool foundA = false, foundB = false, foundC = false;
        for (const auto& p : all) {
            if (p == a && p->value == 1) foundA = true;
            if (p == b && p->value == 2) foundB = true;
            if (p == c && p->value == 3) foundC = true;
        }
        ExpectTrue(foundA, "GetAllReturnsAllInstancesForType: found a");
        ExpectTrue(foundB, "GetAllReturnsAllInstancesForType: found b");
        ExpectTrue(foundC, "GetAllReturnsAllInstancesForType: found c");

        object_broker::clear();
    }

    std::cout << "=== GetOptionalReturnsOptionalWhenPresent ===" << std::endl;
    object_broker::clear();
    {
        auto inst = std::make_shared<Foo>(99);
        object_broker::register_instance<Foo>("opt", inst);

        auto opt = object_broker::get_optional<Foo>("opt"); // optional 반환 검사
        ExpectTrue(opt.has_value(), "GetOptionalReturnsOptionalWhenPresent: opt has value");
        ExpectNe(*opt, nullptr, "GetOptionalReturnsOptionalWhenPresent: deref not null");
        if (opt.has_value() && *opt) ExpectEq((*opt)->value, 99, "GetOptionalReturnsOptionalWhenPresent: value 99");

        auto none = object_broker::get_optional<Foo>("missing");
        ExpectTrue(!none.has_value(), "GetOptionalReturnsOptionalWhenPresent: missing has no value");

        object_broker::clear();
    }

    std::cout << "=== RegistrationGuardRegistersAndUnregistersAutomatically ===" << std::endl;
    object_broker::clear();
    {
        auto temp = std::make_shared<Foo>(555);
        {
            object_broker::registration_guard<Foo> guard("scoped", temp); // 생성 시 등록, 소멸 시 자동 제거
            auto got = object_broker::get<Foo>("scoped");
            ExpectNe(got, nullptr, "RegistrationGuard: scoped registered");
            ExpectEq(got, temp, "RegistrationGuard: pointer equality");
        }
        ExpectEq(object_broker::get<Foo>("scoped"), nullptr, "RegistrationGuard: auto-unregistered");
        object_broker::clear();
    }

    std::cout << "=== ListAllNamesAndEntries ===" << std::endl;
    object_broker::clear();
    {
        auto a = std::make_shared<Foo>(1);
        auto b = std::make_shared<Foo>(2);
        auto cfg = std::make_shared<config_service>();

        object_broker::register_instance<Foo>("a", a);
        object_broker::register_instance<Foo>("b", b);
        object_broker::register_instance<config_service>("a", cfg);

        auto all_names = object_broker::list_all_names(); // 모든 엔트리의 이름 목록
        ExpectEq(all_names.size(), 3u, "ListAllNamesAndEntries: all names size 3");

        auto unique_names = object_broker::list_unique_names(); // 중복 제거된 이름 목록
        ExpectEq(unique_names.size(), 2u, "ListAllNamesAndEntries: unique names size 2");
        ExpectTrue(std::find(unique_names.begin(), unique_names.end(), "a") != unique_names.end(), "ListAllNamesAndEntries: contains 'a'");
        ExpectTrue(std::find(unique_names.begin(), unique_names.end(), "b") != unique_names.end(), "ListAllNamesAndEntries: contains 'b'");

        auto entries = object_broker::list_all_entries(); // (typeid, name) 쌍 목록
        ExpectEq(entries.size(), 3u, "ListAllNamesAndEntries: entries size 3");

        bool found_FA = false, found_FB = false, found_CSA = false;
        for (const auto& e : entries) {
            if (e.first == typeid(Foo) && e.second == "a") found_FA = true;
            if (e.first == typeid(Foo) && e.second == "b") found_FB = true;
            if (e.first == typeid(config_service) && e.second == "a") found_CSA = true;
        }
        ExpectTrue(found_FA, "ListAllNamesAndEntries: found (Foo,'a')");
        ExpectTrue(found_FB, "ListAllNamesAndEntries: found (Foo,'b')");
        ExpectTrue(found_CSA, "ListAllNamesAndEntries: found (config_service,'a')");

        object_broker::clear();
    }

    // 최종 결과 출력: 실패가 없으면 All checks passed, 아니면 실패 개수 출력
    if (g_failures == 0) {
        std::cout << "=== All checks passed. ===" << std::endl;
    }
    else {
        std::cerr << "Failures: " << g_failures << std::endl;
    }
    return g_failures;
}

