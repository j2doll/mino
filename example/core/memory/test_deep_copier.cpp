#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <cassert>
#include <cstdint>

#include "mino/core/memory/memory.hpp"
#include "mino/core/memory/fixed_memory_pool.hpp"
#include "mino/core/memory/pool_allocator.hpp"
#include "mino/core/string/to_console_encoding.hpp"


//----------------------------------------------------------------
// 4. deep_copier 테스트
//----------------------------------------------------------------

struct Player {
    int id = 0;
    std::string name;
    std::vector<int> scores;

    void serialize(mino::core::memory::serializer& s) const {
        s.serialize(id);
        s.serialize(name);
        s.serialize(scores);
    }

    bool deserialize(mino::core::memory::deserializer& d) {
        return d.deserialize(id) && d.deserialize(name) && d.deserialize(scores);
    }
};

void test_deep_copier() {
    using deep_copier = mino::core::memory::deep_copier;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] deep_copier 및 커스텀 객체 깊은 복사 테스트...")
        << std::endl;

    Player p1;
    p1.id = 1001;
    p1.name = "MinoUser";
    p1.scores = { 95, 88, 100, 72 };

    // deep_copier::copy(src, dst)
    Player p2;
    bool success = deep_copier::copy(p1, p2);
    assert(success);

    // deep_copier::copy(src)
    Player p3 = deep_copier::copy(p1);

    // 값 일치 검증
    assert(p1.id == p2.id && p1.name == p2.name && p1.scores == p2.scores);
    assert(p1.id == p3.id && p1.name == p3.name && p1.scores == p3.scores);

    std::cout
        << to_console_encoding(" -> 성공 (Player ID: ") << p2.id
        << ", Name: " << p2.name << ", Scores Count: " << p2.scores.size() << ")"
        << std::endl;
}


