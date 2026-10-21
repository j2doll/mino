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
// 2. std::string 및 std::vector 직렬화/역직렬화 테스트
//----------------------------------------------------------------

void test_string_and_vector() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] std::string 및 std::vector 직렬화/역직렬화 테스트...")
        << std::endl;

    serializer s; // 직렬화 객체 생성

    std::string orig_str = "Hello, Mino Core Memory!";
    std::vector<int> orig_vec = { 10, 20, 30, 40, 50 }; // 동적 컨테이너도 직렬화 가능

    // 직렬화
    s.serialize(orig_str);
    s.serialize(orig_vec);

    deserializer d(s.buffer); // 역직렬화 객체 생성

    std::string deserialized_str;
    std::vector<int> deserialized_vec;

    // 역직렬화
    assert(d.deserialize(deserialized_str));
    assert(d.deserialize(deserialized_vec));

    // 역직렬화된 값이 원본과 동일한지 확인
    assert(orig_str == deserialized_str);
    assert(orig_vec == deserialized_vec);

    std::cout
        << to_console_encoding(" -> 성공 (string: \"") << to_console_encoding(deserialized_str)
        << to_console_encoding("\", vector size: ") << deserialized_vec.size() << ")"
        << std::endl;
}

