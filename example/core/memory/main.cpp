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

//----------------------------------------------------------------
// 5. fixed_memory_pool 기본 블록 할당/해제 테스트
//----------------------------------------------------------------

struct NetworkPacket {
    std::uint32_t session_id{ 0 };
    std::uint32_t sequence{ 0 };
    char payload[64]{ 0 };
};

void test_fixed_memory_pool() {
    using fixed_memory_pool = mino::core::memory::fixed_memory_pool;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] fixed_memory_pool 기본 블록 할당/해제 테스트...")
        << std::endl;

    // 고정 크기 메모리 풀 생성 (구조체 NetworkPacket 크기로 128개 블록 사전 할당)
    constexpr std::size_t max_allowed_blocks = 128;
    fixed_memory_pool packet_pool(sizeof(NetworkPacket), max_allowed_blocks);

    // 런타임 수치 확인
    std::cout << to_console_encoding("[1] 할당 이전\n");
    std::cout << to_console_encoding("청크 개수: ") << packet_pool.chunk_count() << '\n';
    std::cout << to_console_encoding("청크당 블록 수: ") << packet_pool.blocks_per_chunk() << '\n';
    std::cout << to_console_encoding("전체 용량: ") << packet_pool.capacity() << '\n';
    std::cout << to_console_encoding("할당된 블록 수: ") << packet_pool.allocated_count() << '\n';
    std::cout << to_console_encoding("잔여 블록 수: ") << packet_pool.available_count() << '\n';
    std::cout << std::endl;
    // [1] 할당 이전
    //     청크 개수 : 0 (초기 대기 상태. 메모리 풀 객체만 생성되었을 뿐, 아직 한 번도 allocate()가 호출되지 않은 상태)
    //     청크당 블록 수 : 128
    //     전체 용량 : 0
    //     할당된 블록 수 : 0
    //     잔여 블록 수 : 0


    // 꽉 차서 새 청크 생성이 필요한 상태인지 사전 검사
    if (packet_pool.is_full()) {
        std::cout
            << to_console_encoding(" -> 메모리 풀은 현재 꽉 찼습니다. 새 청크를 생성해야 합니다.")
            << std::endl;
    }

    if (packet_pool.can_allocate_immediately()) { // 즉시 사용 가능한 블록이 있는지 사전 검사
        std::cout << to_console_encoding(" -> 즉시 할당 가능한 블록이 있습니다.") << std::endl;
        // void* mem = packet_pool.allocate();
        // ...
    }

    void* raw_mem = packet_pool.allocate(); // [1] 블록 1개 할당 (할당 가능한지 검사는 않함)
    assert(raw_mem != nullptr); // raw_mem이 nullptr이면, 메모리 풀에서 블록 할당 실패.
    // 더이상 할당할 블록이 없거나, 메모리 풀 자체가 초기화되지 않은 상태일 수 있음.

    // 배치 new를 통한 객체 생성
    auto* packet = new (raw_mem) NetworkPacket{ 1001, 1, "PING" }; // [2] 생성자 호출 및 초기화
    assert(packet->session_id == 1001);
    assert(packet->sequence == 1);
    assert(std::string(packet->payload) == "PING");

    std::cout
        << to_console_encoding(" -> 블록 할당 성공 (주소: ") << raw_mem
        << to_console_encoding(", Session ID: ") << packet->session_id
        << to_console_encoding(", Payload: ") << packet->payload
        << to_console_encoding(", Block Size: ") << packet_pool.block_size() << " bytes)"
        << std::endl;

    std::cout << std::endl;

    if (packet_pool.can_allocate_immediately()) { // 즉시 사용 가능한 블록이 있는지 사전 검사
        std::cout << to_console_encoding(" -> 즉시 할당 가능한 블록이 있습니다.") << std::endl;
        // void* mem = packet_pool.allocate();
        // ...
    }

    std::cout << std::endl;

    // 런타임 수치 확인
    std::cout << to_console_encoding("[2] 할당 이후\n");
    std::cout << to_console_encoding("청크 개수: ") << packet_pool.chunk_count() << '\n'; 
    std::cout << to_console_encoding("청크당 블록 수: ") << packet_pool.blocks_per_chunk() << '\n'; 
    std::cout << to_console_encoding("전체 용량: ") << packet_pool.capacity() << '\n'; 
    std::cout << to_console_encoding("할당된 블록 수: ") << packet_pool.allocated_count() << '\n'; 
    std::cout << to_console_encoding("잔여 블록 수: ") << packet_pool.available_count() << '\n'; 
    std::cout << std::endl;
    // [2] 할당 이후
    //     청크 개수 : 1 (allocate()가 호출되면서 설정된 단위(blocks_per_chunk, 예: 128개)만큼의 메모리 덩어리 1세트가 힙에 확보된 상태)
    //     청크당 블록 수 : 128
    //     전체 용량 : 128
    //     할당된 블록 수 : 1 (사용 중인 블록)
    //     잔여 블록 수 : 127 (사용 가능한 블록)

    // 소멸자 호출 및 풀에 메모리 반환
    packet->~NetworkPacket(); // [3] 소멸자 호출
    packet_pool.deallocate(raw_mem); // [4] 블록 반환

    std::cout << to_console_encoding(" -> 해제 성공") << std::endl;

    // 런타임 수치 확인
    std::cout << to_console_encoding("[3] 해제 이후\n");
    std::cout << to_console_encoding("청크 개수: ") << packet_pool.chunk_count() << '\n'; 
    std::cout << to_console_encoding("청크당 블록 수: ") << packet_pool.blocks_per_chunk() << '\n'; 
    std::cout << to_console_encoding("전체 용량: ") << packet_pool.capacity() << '\n'; 
    std::cout << to_console_encoding("할당된 블록 수: ") << packet_pool.allocated_count() << '\n'; 
    std::cout << to_console_encoding("잔여 블록 수: ") << packet_pool.available_count() << '\n'; 
    std::cout << std::endl;
    // [3] 해제 이후
    //    청크 개수 : 1 (해제 후에도 OS로부터 확보한 메모리 청크를 반납하지 않고 그대로 유지(캐싱))
    //    청크당 블록 수 : 128
    //    전체 용량 : 128
    //    할당된 블록 수 : 0 (1개를 해제하였으므로 0개가 되었음)
    //    잔여 블록 수 : 128 (1개를 해제하였으므로 127 -> 128개가 되었음)

    if (packet_pool.can_allocate_immediately()) { // 즉시 사용 가능한 블록이 있는지 사전 검사
        std::cout << to_console_encoding(" -> 즉시 할당 가능한 블록이 있습니다.") << std::endl;
        // void* mem = packet_pool.allocate();
        // ...
    }

}

//----------------------------------------------------------------
// 6. pool_allocator 및 STL 컨테이너 연동 테스트
//----------------------------------------------------------------

// 전역 스코프 타입 별칭 정의
template <typename T>
using pool_allocator = mino::core::memory::pool_allocator<T>;

void test_pool_allocator_with_stl() {
    using fixed_memory_pool = mino::core::memory::fixed_memory_pool;
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("[Test] pool_allocator 및 STL 컨테이너(std::list, std::vector) 연동 테스트...")
        << std::endl;

    // 1. std::list 연동 (노드 단위 할당 시 메모리 풀 활용)
    {
        fixed_memory_pool list_pool(sizeof(int) + 32, 256); // list_pool 을 생성 (int + list 노드 오버헤드 고려, 256개 블록)
        pool_allocator<int> alloc(&list_pool); // list_pool 메모리 풀과 연동되는 pool_allocator 생성

        std::list<int, pool_allocator<int>> pool_list(alloc); // std::list 생성 시 pool_allocator를 사용하여 메모리 풀과 연동

        for (int i = 1; i <= 5; ++i) {
            pool_list.push_back(i * 10);
        }

        assert(pool_list.size() == 5);

        int expected = 10;
        for (int val : pool_list) {
            assert(val == expected);
            expected += 10;
        }
        // expected: 60 (10(초기), 20, 30, 40, 50 순으로 증가)

        std::cout
            << to_console_encoding(" -> std::list 연동 성공 (원소 수: ")
            << pool_list.size() << ")" << std::endl;
    }

    // 2. std::vector 연동 (n > 1 연속 블록 요청 시 글로벌 힙 자동 폴백 확인)
    {
        fixed_memory_pool vec_pool(sizeof(double), 64); // vec_pool 을 생성 (double 크기 고려, 64개 블록)
        pool_allocator<double> alloc(&vec_pool); // vec_pool 메모리 풀과 연동되는 pool_allocator 생성

        std::vector<double, pool_allocator<double>> pool_vec(alloc); // std::vector 생성 시 pool_allocator를 사용하여 메모리 풀과 연동

        assert(pool_vec.capacity() == 0);
        pool_vec.reserve(10);
        // 사용자가 실수로 노드 기반 컨테이너(`std::list`, `std::map`)가 아닌
        // 연속 메모리 컨테이너(`std::vector`, `std::string`)에 `pool_allocator`를 지정하더라도,
        // 프로그램이 죽거나 미정의 동작(UB)을 일으키지 않고
        // 유연하게 글로벌 힙으로 우회하여 정상 작동함을 테스트하는 코드.
        //
        // 1. `std::vector`와 `fixed_memory_pool`의 특성 차이
        //    - `fixed_memory_pool`: 단일 크기의 블록을 프리 리스트로 관리하므로,
        //       원소 1개(n = 1) 단위 할당에만 특화되어 있습니다.
        //    - `std::vector`: 모든 원소가 메모리상에 끊김 없이 연속된 배열 형태로 존재해야 합니다.
        // 
        // 2. `reserve(10)` 호출 시 발생하는 내부 동작
        //    - `reserve(10)`을 실행하면 `std::vector`는
        //      내부적으로 할당자의 `allocate(10)`을 호출합니다 (n = 10).
        //    - 이때 앞서 작성한 `pool_allocator::allocate`의 분기 로직이 동작합니다:
        //    ```cpp
        //    T* allocate(std::size_t n) {
        //        // 1. n == 1 (단일 객체, 예: std::list 노드): 메모리 풀 사용
        //        if (pool_ != nullptr && n == 1 && sizeof(T) <= pool_->block_size()) {
        //            return static_cast<T*>(pool_->allocate());
        //        }
        //    
        //        // 2. n > 1 (연속 메모리): 풀을 쓰지 않고 전역 힙으로 자동 우회
        //        return static_cast<T*>(::operator new(n * sizeof(T)));
        //    }
        //    ```
        //    - n = 10 이므로 조건문 `n == 1`을 만족하지 않습니다.
        //    - 따라서 고정 메모리 풀을 건드려 메모리가 깨지는 대신,
        //      안전하게 `::operator new`를 통해 시스템 힙에서 연속 공간 10개분을 할당받게 됩니다.
        //
        assert(pool_vec.capacity() == 10); // 재할당 없이 담을 수 있는 최대 원소 공간의 크기

        for (int i = 0; i < 5; ++i) {
            pool_vec.push_back(i * 1.5);
        }

        assert(pool_vec.size() == 5); // 현재 벡터 안에 실제로 생성되어 들어있는 원소의 개수
        assert(pool_vec[0] == 0.0);
        assert(pool_vec[2] == 3.0);

        std::cout
            << to_console_encoding(" -> std::vector 연동 및 힙 폴백 성공 (원소 수: ")
            << pool_vec.size() << ")" << std::endl;
    }
}

//----------------------------------------------------------------
// main 진입점
//----------------------------------------------------------------

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;

    std::cout
        << to_console_encoding("=== Mino Core Memory 테스트 시작 ===")
        << std::endl << std::endl;

    // 기존 메모리 직렬화 및 복사 테스트
    test_basic_and_pod();
    std::cout << std::endl;

    test_string_and_vector();
    std::cout << std::endl;

    test_vector_of_structs();
    std::cout << std::endl;

    test_deep_copier();
    std::cout << std::endl;

    test_fixed_memory_pool();
    std::cout << std::endl;

    test_pool_allocator_with_stl();
    std::cout << std::endl;

    std::cout
        << std::endl
        << to_console_encoding("=== 모든 테스트 통과 ===")
        << std::endl;

    return 0;
}
