#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#include "mino/core/memory/memory.hpp"
#include "mino/core/string/to_console_encoding.hpp"

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
    s.serialize(orig_int);  
    s.serialize(orig_double);  
    s.serialize(orig_point);  

    deserializer d(s.buffer); // 역직렬화 객체 생성.
    //  s.buffer는 직렬화된 데이터를 담고 있음.
    //  s.buffer는 통신, 파일 등을 통하여 전송될 수 있음.

    int deserialized_int = 0;
    double deserialized_double = 0.0;
    Point deserialized_point{ 0, 0 };

    // 역직렬화
    assert(d.deserialize(deserialized_int));
    assert(d.deserialize(deserialized_double));
    assert(d.deserialize(deserialized_point));

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

void test_string_and_vector() {
    using serializer = mino::core::memory::serializer;
    using deserializer = mino::core::memory::deserializer;

    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] std::string 및 std::vector 직렬화/역직렬화 테스트...")
        << std::endl;

    serializer s; // 직렬화 객체 생성

    std::string orig_str = "Hello, Mino Core Memory!";
    std::vector<int> orig_vec = { 10, 20, 30, 40, 50 }; // 고정형 자료가 아닌 vector 동적 자료형도 직렬화/역직렬화 가능
    // NOTE: mino::core::memory 직렬화는 POD(Plain Old Data) 타입, std::string, std::vector<T>만 지원됨. 

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

//----------------------------------------------------------------

struct Enemy {
    int hp = 0; // POD 타입 멤버
    std::string name; // POD가 아닌 std::string 멤버도 직렬화/역직렬화 가능

    void serialize(mino::core::memory::serializer& s) const {
        // 멤버 순서대로 직렬화
        s.serialize(hp);
        s.serialize(name);
    }

    bool deserialize(mino::core::memory::deserializer& d) {
        // 멤버 순서대로 역직렬화
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
    s.serialize(orig); // 직렬화

    deserializer d(s.buffer); // 역직렬화 객체 생성
    std::vector<Enemy> out;
    assert(d.deserialize(out)); // 역직렬화
    assert(orig == out); // 역직렬화되기 전의 값과 비교. {{100, "Goblin"}, {250, "Troll"}, {50, "Slime"}}
    // vector<Enemy>는 vector의 모든 Enemy들의 연산자==가 true이면 true 반환됨.

    std::cout << to_console_encoding(" -> 성공 (count: ") << out.size() << ")" << std::endl;
}

//----------------------------------------------------------------

// 직렬화/역직렬화 기능을 갖춘 커스텀 클래스 예시
struct Player {
    int id = 0; // POD 타입 멤버
    std::string name; // POD가 아닌 std::string 멤버도 직렬화/역직렬화 가능
    std::vector<int> scores; // POD가 아닌 std::vector 멤버도 직렬화/역직렬화 가능

    void serialize(mino::core::memory::serializer& s) const {
        // 멤버 순서대로 직렬화
        s.serialize(id);
        s.serialize(name);
        s.serialize(scores);
    }

    bool deserialize(mino::core::memory::deserializer& d) {
        // 멤버 순서대로 역직렬화
        return d.deserialize(id) && d.deserialize(name) && d.deserialize(scores);
    }
};

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
    bool success = deep_copier::copy(p1, p2); // p1을 p2로 깊은 복사.
    // NOTE: 깊은 복사(deep copy)란, 단순히 포인터 주소만 복사하는 것이 아니라, 실제 데이터를 새로 할당하여 복사하는 것을 의미합니다.
    assert(success);

    // deep_copier::copy(src) 함수 사용
    Player p3 = deep_copier::copy(p1); // p1을 p3로 깊은 복사. 실패 시 기본 생성된 객체 반환.

    // 복사된 객체들이 원본과 동일한지 확인
    assert(p1.id == p2.id && p1.name == p2.name && p1.scores == p2.scores); // p1과 p2가 동일한지 확인
    assert(p1.id == p3.id && p1.name == p3.name && p1.scores == p3.scores); // p1과 p3가 동일한지 확인

    std::cout
        << to_console_encoding(" -> 성공 (Player ID: ") << p2.id
        << ", Name: " << p2.name << ", Scores Count: " << p2.scores.size() << ")"
        << std::endl;
}

//----------------------------------------------------------------

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
