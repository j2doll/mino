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
// 1. 기본 타입 및 POD 직렬화/역직렬화 테스트
//----------------------------------------------------------------

// POD(Plain Old Data) 구조체 예시
struct Point {
    int x;
    int y;
};

void test_basic_and_pod() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] 기본 타입 및 POD 직렬화/역직렬화 테스트...")
        << std::endl;

    serializer s; // 직렬화 객체 생성

    int orig_int = 42; // int는 기본 타입(POD: Plain Old Data)
    double orig_double = 3.14159; // double은 기본 타입(POD)
    Point orig_point{ 10, 20 }; // Point는 POD 자료로만 구성된 구조체

    // 직렬화
    s.serialize(orig_int);    // [1]
    s.serialize(orig_double); // [2]
    s.serialize(orig_point);  // [3]

    // s.buffer는 직렬화된 데이터를 담고 있음 (복사, 통신, 파일 등을 통하여 전송 가능)

    deserializer d(s.buffer); // 역직렬화 객체 생성
    // s.buffer는 직렬화된 데이터를 담고 있음 (통신, 파일 등을 통하여 전송 가능)

    int deserialized_int = 0;
    double deserialized_double = 0.0;
    Point deserialized_point{ 0, 0 };

    // 역직렬화. 직렬화의 순서와 동일하게 역직렬화해야 함.
    assert(d.deserialize(deserialized_int));    // [1]
    assert(d.deserialize(deserialized_double)); // [2]
    assert(d.deserialize(deserialized_point));  // [3]

    // 역직렬화된 값이 원본과 동일한지 확인
    assert(orig_int == deserialized_int); // 42
    assert(orig_double == deserialized_double); // 3.14159
    assert(orig_point.x == deserialized_point.x && orig_point.y == deserialized_point.y); // {10, 20}

    std::cout
        << to_console_encoding(" -> 성공 (int: ") << deserialized_int
        << ", double: " << deserialized_double
        << ", Point: {" << deserialized_point.x << ", " << deserialized_point.y << "})"
        << std::endl;
}
