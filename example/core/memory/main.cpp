#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#include "mino/core/memory/memory.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// 직렬화/역직렬화 기능을 갖춘 커스텀 클래스 예시
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

    int orig_int = 42;
    double orig_double = 3.14159;
    Point orig_point{ 10, 20 };

    s.serialize(orig_int);  
    s.serialize(orig_double);  
    s.serialize(orig_point);  

    deserializer d(s.buffer); // 역직렬화 객체 생성.
    //  s.buffer는 직렬화된 데이터를 담고 있음.
    //  s.buffer는 통신, 파일 등을 통하여 전송될 수 있음.

    int deserialized_int = 0;
    double deserialized_double = 0.0;
    Point deserialized_point{ 0, 0 };

    assert(d.deserialize(deserialized_int));
    assert(d.deserialize(deserialized_double));
    assert(d.deserialize(deserialized_point));

    assert(orig_int == deserialized_int); // 42
    assert(orig_double == deserialized_double); // 3.14159
    assert(orig_point.x == deserialized_point.x && orig_point.y == deserialized_point.y); // {10, 20}

    std::cout
        << to_console_encoding(" -> 성공 (int: ") << deserialized_int
        << ", double: " << deserialized_double
        << ", Point: {" << deserialized_point.x << ", " << deserialized_point.y << "})"
        << std::endl;
}

void test_string_and_vector() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] std::string 및 std::vector 직렬화/역직렬화 테스트...")
        << std::endl;

    serializer s; // 직렬화 객체 생성

    std::string orig_str = "Hello, Mino Core Memory!";
    std::vector<int> orig_vec = { 10, 20, 30, 40, 50 }; // 고정형 자료가 아닌 vector와 같은 동적 자료형도 직렬화/역직렬화 가능
    // NOTE: 직렬화는 POD(Plain Old Data) 타입, std::string, std::vector<T>만 지원됨. 

    s.serialize(orig_str);
    s.serialize(orig_vec);

    deserializer d(s.buffer); // 역직렬화 객체 생성

    std::string deserialized_str;
    std::vector<int> deserialized_vec;

    assert(d.deserialize(deserialized_str));
    assert(d.deserialize(deserialized_vec));

    assert(orig_str == deserialized_str);
    assert(orig_vec == deserialized_vec);

    std::cout
        << to_console_encoding(" -> 성공 (string: \"") << to_console_encoding(deserialized_str)
        << to_console_encoding("\", vector size: ") << deserialized_vec.size() << ")"
        << std::endl;
}

struct Enemy {
    int hp = 0;
    std::string name;

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

    serializer s; // 직렬화 객체 생성
    std::vector<Enemy> orig = { {100, "Goblin"}, {250, "Troll"}, {50, "Slime"} };
    s.serialize(orig);

    deserializer d(s.buffer); // 역직렬화 객체 생성
    std::vector<Enemy> out;
    assert(d.deserialize(out));
    assert(orig == out); // { {100, "Goblin"}, {250, "Troll"}, {50, "Slime"} }

    std::cout << to_console_encoding(" -> 성공 (count: ") << out.size() << ")" << std::endl;
}


void test_deep_copier() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;
    using deep_copier = mino::core::memory::deep_copier;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] deep_copier 및 커스텀 객체 깊은 복사 테스트...")
        << std::endl;

    Player p1;
    p1.id = 1001;
    p1.name = "MinoUser";
    p1.scores = { 95, 88, 100, 72 };

    // deep_copier::copy(src, dst) 함수 사용
    Player p2;
    bool success = deep_copier::copy(p1, p2);
    assert(success);

    // deep_copier::copy(src) 함수 사용
    Player p3 = deep_copier::copy(p1);
    assert(p1.id == p2.id && p1.name == p2.name && p1.scores == p2.scores);
    assert(p1.id == p3.id && p1.name == p3.name && p1.scores == p3.scores);

    std::cout
        << to_console_encoding(" -> 성공 (Player ID: ") << p2.id
        << ", Name: " << p2.name << ", Scores Count: " << p2.scores.size() << ")"
        << std::endl;
}

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== Mino Core Memory 테스트 시작 ===")
        << std::endl << std::endl;

    test_basic_and_pod();
    test_string_and_vector();
    test_vector_of_structs();
    test_deep_copier();

    std::cout
        << std::endl
        << to_console_encoding("=== 모든 테스트 통과 ===")
        << std::endl;
    return 0;
}
