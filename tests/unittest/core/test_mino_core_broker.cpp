#include "mino/core/broker/broker.hpp"
#include "catch_amalgamated.hpp"
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

using mino::core::broker::object_broker;

// 테스트에 사용할 샘플 구조체
namespace {
    struct TestService {
        int id{ 0 };
        std::string name;
    };

    struct AnotherService {
        double value{ 0.0 };
    };
}

TEST_CASE("기본 인스턴스 등록 및 조회 테스트", "[object_broker]") {
    object_broker::clear();

    SECTION("기본(__default__) 이름으로 등록 및 get 조회") {
        auto service = std::make_shared<TestService>(TestService{ 1, "default_service" });
        object_broker::register_instance(service);

        REQUIRE(object_broker::contains<TestService>());

        auto retrieved = object_broker::get<TestService>();
        REQUIRE(retrieved != nullptr);
        CHECK(retrieved->id == 1);
        CHECK(retrieved->name == "default_service");
        CHECK(retrieved == service); // 동일한 shared_ptr 포인터인지 확인
    }

    SECTION("등록되지 않은 인스턴스 조회 시 nullptr 반환") {
        REQUIRE_FALSE(object_broker::contains<TestService>());
        CHECK(object_broker::get<TestService>() == nullptr);
    }

    SECTION("get_optional 메서드 테스트") {
        CHECK_FALSE(object_broker::get_optional<TestService>().has_value());

        auto service = std::make_shared<TestService>(TestService{ 2, "optional_test" });
        object_broker::register_instance(service);

        auto opt = object_broker::get_optional<TestService>();
        REQUIRE(opt.has_value());
        CHECK(opt.value()->id == 2);
    }
}

TEST_CASE("이름 기반 인스턴스 등록 및 타입 격리 테스트", "[object_broker]") {
    object_broker::clear();

    SECTION("동일한 타입의 서로 다른 별칭 등록") {
        auto svc1 = std::make_shared<TestService>(TestService{ 10, "first" });
        auto svc2 = std::make_shared<TestService>(TestService{ 20, "second" });

        object_broker::register_instance("svc1", svc1);
        object_broker::register_instance("svc2", svc2);

        CHECK(object_broker::contains<TestService>("svc1"));
        CHECK(object_broker::contains<TestService>("svc2"));
        CHECK_FALSE(object_broker::contains<TestService>("unknown"));

        auto res1 = object_broker::get<TestService>("svc1");
        auto res2 = object_broker::get<TestService>("svc2");

        REQUIRE(res1 != nullptr);
        REQUIRE(res2 != nullptr);
        CHECK(res1->name == "first");
        CHECK(res2->name == "second");
    }

    SECTION("동일한 이름이라도 타입이 다르면 별도로 관리") {
        auto svc = std::make_shared<TestService>(TestService{ 100, "service" });
        auto another = std::make_shared<AnotherService>(AnotherService{ 3.14 });

        object_broker::register_instance("common_name", svc);
        object_broker::register_instance("common_name", another);

        auto resSvc = object_broker::get<TestService>("common_name");
        auto resAnother = object_broker::get<AnotherService>("common_name");

        REQUIRE(resSvc != nullptr);
        REQUIRE(resAnother != nullptr);
        CHECK(resSvc->id == 100);
        CHECK(resAnother->value == 3.14);
    }

    SECTION("기존 이름으로 재등록 시 인스턴스 덮어쓰기 확인") {
        auto oldSvc = std::make_shared<TestService>(TestService{ 1, "old" });
        auto newSvc = std::make_shared<TestService>(TestService{ 2, "new" });

        object_broker::register_instance("target", oldSvc);
        CHECK(object_broker::get<TestService>("target")->name == "old");

        object_broker::register_instance("target", newSvc);
        CHECK(object_broker::get<TestService>("target")->name == "new");
    }
}

TEST_CASE("등록 해제(unregister) 및 초기화(clear) 테스트", "[object_broker]") {
    object_broker::clear();

    auto svc1 = std::make_shared<TestService>(TestService{ 1, "svc1" });
    auto svc2 = std::make_shared<TestService>(TestService{ 2, "svc2" });

    object_broker::register_instance("s1", svc1);
    object_broker::register_instance("s2", svc2);

    SECTION("특정 이름 인스턴스 해제") {
        object_broker::unregister_instance<TestService>("s1");

        CHECK_FALSE(object_broker::contains<TestService>("s1"));
        CHECK(object_broker::get<TestService>("s1") == nullptr);
        CHECK(object_broker::contains<TestService>("s2")); // 다른 항목은 유지
    }

    SECTION("clear() 호출 시 전체 데이터 삭제") {
        object_broker::clear();

        CHECK_FALSE(object_broker::contains<TestService>("s1"));
        CHECK_FALSE(object_broker::contains<TestService>("s2"));
        CHECK(object_broker::list_all_names().empty());
    }
}

TEST_CASE("조회 및 목록화(Listing) API 테스트", "[object_broker]") {
    object_broker::clear();

    auto svc1 = std::make_shared<TestService>(TestService{ 1, "svc1" });
    auto svc2 = std::make_shared<TestService>(TestService{ 2, "svc2" });
    auto another = std::make_shared<AnotherService>(AnotherService{ 99.9 });

    object_broker::register_instance("alpha", svc1);
    object_broker::register_instance("beta", svc2);
    object_broker::register_instance("alpha", another); // 중복 이름(다른 타입)

    SECTION("get_all: 특정 타입의 인스턴스 목록 반환") {
        auto list = object_broker::get_all<TestService>();
        CHECK(list.size() == 2);
    }

    SECTION("get_all_with_names: 이름과 포인터 쌍 반환") {
        auto pairs = object_broker::get_all_with_names<TestService>();
        REQUIRE(pairs.size() == 2);

        bool foundAlpha = false;
        bool foundBeta = false;
        for (const auto& [name, ptr] : pairs) {
            if (name == "alpha" && ptr->id == 1) foundAlpha = true;
            if (name == "beta" && ptr->id == 2) foundBeta = true;
        }
        CHECK(foundAlpha);
        CHECK(foundBeta);
    }

    SECTION("list_names_for_type: 특정 타입에 해당하는 이름 목록 반환") {
        auto names = object_broker::list_names_for_type<TestService>();
        REQUIRE(names.size() == 2);
        CHECK(std::find(names.begin(), names.end(), "alpha") != names.end());
        CHECK(std::find(names.begin(), names.end(), "beta") != names.end());
    }

    SECTION("list_all_names & list_unique_names 테스트") {
        auto allNames = object_broker::list_all_names();
        CHECK(allNames.size() == 3); // "alpha", "beta", "alpha"

        auto uniqueNames = object_broker::list_unique_names();
        CHECK(uniqueNames.size() == 2); // 중복 제거되어 2개
    }

    SECTION("list_all_entries: (type_index, name) 쌍 목록") {
        auto entries = object_broker::list_all_entries();
        CHECK(entries.size() == 3);
    }
}

TEST_CASE("registration_guard 스코프 자동 해제(RAII) 테스트", "[object_broker]") {
    object_broker::clear();

    auto service = std::make_shared<TestService>(TestService{ 77, "scoped" });

    {
        object_broker::registration_guard<TestService> guard("scoped_name", service);
        CHECK(object_broker::contains<TestService>("scoped_name"));
        CHECK(object_broker::get<TestService>("scoped_name") == service);
    }

    // 스코프를 벗어난 후 자동으로 unregister 되었는지 확인
    CHECK_FALSE(object_broker::contains<TestService>("scoped_name"));
    CHECK(object_broker::get<TestService>("scoped_name") == nullptr);

    SECTION("기본 이름으로 registration_guard 사용") {
        {
            object_broker::registration_guard<TestService> guard(service);
            CHECK(object_broker::contains<TestService>());
        }
        CHECK_FALSE(object_broker::contains<TestService>());
    }
}

TEST_CASE("멀티스레드 동시 접근 스레드 안전성(Thread-Safety) 테스트", "[object_broker]") {
    object_broker::clear();

    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS = 100;
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        // [t] -> [t, ITERATIONS] 로 캡처 추가 (또는 [&] 사용)
        threads.emplace_back([t, ITERATIONS]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                std::string name = "thread_" + std::to_string(t) + "_iter_" + std::to_string(i);
                auto service = std::make_shared<TestService>(TestService{ t * 1000 + i, name });

                // 등록
                object_broker::register_instance(name, service);

                // 조회
                auto ret = object_broker::get<TestService>(name);
                if (ret) {
                    CHECK(ret->id == t * 1000 + i);
                }

                // 존재 여부 확인
                CHECK(object_broker::contains<TestService>(name));

                // 등록 해제
                object_broker::unregister_instance<TestService>(name);
            }
            });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}
