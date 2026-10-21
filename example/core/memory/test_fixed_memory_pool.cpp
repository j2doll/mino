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

