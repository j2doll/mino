#include <iostream>
#include <cassert>
#include <string>
#include <utility>

#include "mino/core/container/container.hpp"

namespace {
    struct TestUser {
        std::string name;
        int age;

        TestUser(std::string n, int a, long p)
            : name(std::move(n)), age(a) {
        }
    };
}

void test_multi_index_container_all_public() {
    std::cout << "[Testing multi_index_container - Variadic Index Expansion]" << std::endl;

    namespace container = mino::core::container;

    // TestUser 이외 추가로 3개의 독립된 키 지정:
    //  [0]: int (ID)
    //  [1]: std::string (Email)
    //  [2]: long (Phone)
    container::multi_index_container<TestUser, int, std::string, long> users;

    assert(users.empty());
    assert(users.size() == 0);

    // 1. insert (TestUser, [0] int, [1] std::string, [2] long  )
    //           (TestUser, [0] ID,  [1] Email,       [2] Phone )
    assert(users.insert(TestUser("Alice",   25, 1001L), 1, "alice@example.com",   1001L) == true);
    assert(users.insert(TestUser("Bob",     30, 1002L), 2, "bob@example.com",     1002L) == true);
    assert(users.insert(TestUser("Charlie", 28, 1003L), 3, "charlie@example.com", 1003L) == true);

    assert(users.size() == 3);
    assert(!users.empty());

    // 중복 키 삽입 방지 검증 (어떤 인덱스든 키가 겹치면 실패해야 함)
    assert(users.insert(TestUser("David", 35, 1004L), 1, "david@example.com", 1004L) == false); // Key0 (ID 1) 중복
    assert(users.insert(TestUser("Eve",   29, 1005L), 4, "alice@example.com", 1005L) == false); // Key1 (Email) 중복
    assert(users.insert(TestUser("Frank", 40, 1002L), 5, "frank@example.com", 1002L) == false); // Key2 (Phone 1002L) 중복

    // 2. 조회 (find_by_index<N>, contains<N>)
    assert(users.contains<0>(1)); // Key0 (ID)가 1인 존재 확인: 존재함(Alice)
    assert(users.contains<1>("bob@example.com")); // Key1 (Email)가 bob@example.com인 존재  확인: 존재함(Bob)
    assert(users.contains<2>(1003L)); // Key2 (Phone)가 1003L인 존재 확인: 존재함(Charlie)
    assert(!users.contains<0>(999)); // Key0 (ID)가 999인 존재 확인: 존재하지 않음

    const TestUser* u1 = users.find_by_index<0>(1); // Key0 (ID)가 1인 값 조회: 존재함(Alice)    
    assert(u1 != nullptr && u1->name == "Alice" && u1->age == 25);

    const TestUser* u3 = users.find_by_index<1>("charlie@example.com"); // Key1 (Email)가 charlie@example.com인 값 조회: 존재함(Charlie)
    assert(u3 != nullptr && u3->name == "Charlie" && u3->age == 28);

    assert(users.find_by_index<1>("unknown@example.com") == nullptr); // Key1 (Email)가 unknown@example.com인 값 조회: 존재하지 않음

    // 3. 삭제 (erase_by_index<N>)
    assert(users.erase_by_index<0>(2) == true); // Key0 (ID)가 2인 값(Bob) 삭제  
    assert(users.erase_by_index<0>(999) == false); // Key0 (ID)가 999인 값은 존재하지 않으므로 삭제 못함

    assert(!users.contains<0>(2)); // Key0 (ID)가 2인 값(Bob)이 삭제되었으므로 존재하지 않음
    assert(!users.contains<1>("bob@example.com")); // Key1 (Email)가 bob@example.com인 값(Bob)이 삭제되었으므로 존재하지 않음

    assert(users.erase_by_index<1>("charlie@example.com") == true); // Key1 (Email)가 charlie@example.com인 값(Charlie) 삭제
    assert(users.size() == 1); // {{Alice, 25, 1001L}, 1, alice@example.com, 1001L} 만 남음

    // 4. clear
    users.clear();
    assert(users.empty());
    assert(users.size() == 0);

    std::cout << "  -> multi_index_container (Variadic) OK!\n\n";
}
