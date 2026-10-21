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
// 3. 구조체 벡터 직렬화/역직렬화 테스트
//----------------------------------------------------------------

struct Enemy {
    int hp = 0; // POD 타입 멤버
    std::string name; // std::string 멤버

    void serialize(mino::core::memory::serializer& s) const {
        s.serialize(hp);
        s.serialize(name);
    }

    bool deserialize(mino::core::memory::deserializer& d) {
        return d.deserialize(hp) && d.deserialize(name);
    }

    bool operator==(Enemy const& o) const {
        return hp == o.hp && name == o.name;
    }
};

void test_vector_of_structs() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout << to_console_encoding("[Test] std::vector<Enemy> 직렬화/역직렬화 테스트...") << std::endl;

    serializer s;
    std::vector<Enemy> orig = { {100, "Goblin"}, {250, "Troll"}, {50, "Slime"} };
    s.serialize(orig);

    deserializer d(s.buffer);
    std::vector<Enemy> out;
    assert(d.deserialize(out));
    assert(orig == out);

    std::cout << to_console_encoding(" -> 성공 (count: ") << out.size() << ")" << std::endl;
}
