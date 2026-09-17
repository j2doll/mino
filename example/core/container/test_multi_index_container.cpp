#include <iostream>
#include <cassert>
#include <string>
#include <utility>

#include "mino/core/container/container.hpp"

namespace {
    struct TestUser {
        std::string name;
        int age;
        TestUser(std::string n, int a) : name(std::move(n)), age(a) {}
    };
}

void test_multi_index_container_all_public() {
    std::cout << "[Testing multi_index_container - All Public Members]" << std::endl;

    namespace container = mino::core::container;
    container::multi_index_container<TestUser, int, std::string> users;

    assert(users.empty());
    assert(users.size() == 0);

    // 1. insert & emplace
    assert(users.insert(TestUser("Alice", 25), 1, "alice@example.com") == true);
    assert(users.insert(TestUser("Bob", 30), 2, "bob@example.com") == true);
    assert(users.emplace(3, "charlie@example.com", "Charlie", 28) == true);

    assert(users.size() == 3);
    assert(!users.empty());

    // 중복 키 삽입 방지 검증
    assert(users.insert(TestUser("David", 35), 1, "david@example.com") == false);  // Key1 중복
    assert(users.insert(TestUser("Eve", 29), 4, "alice@example.com") == false);    // Key2 중복

    // 2. 조회 (find_by_key1, find_by_key2, contains)
    assert(users.contains_key1(1));
    assert(users.contains_key2("bob@example.com"));
    assert(!users.contains_key1(999));

    const TestUser* u1 = users.find_by_key1(1);
    assert(u1 != nullptr && u1->name == "Alice" && u1->age == 25);

    const TestUser* u3 = users.find_by_key2("charlie@example.com");
    assert(u3 != nullptr && u3->name == "Charlie" && u3->age == 28);
    assert(users.find_by_key2("unknown@example.com") == nullptr);

    // 3. 삭제 (erase_by_key1, erase_by_key2)
    assert(users.erase_by_key1(2) == true);
    assert(users.erase_by_key1(999) == false);
    assert(!users.contains_key1(2));
    assert(!users.contains_key2("bob@example.com"));

    assert(users.erase_by_key2("charlie@example.com") == true);
    assert(users.size() == 1);

    // 4. clear
    users.clear();
    assert(users.empty());
    assert(users.size() == 0);

    std::cout << "  -> multi_index_container OK!\n\n";
}
