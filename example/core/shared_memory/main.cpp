#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "mino/core/shared_memory/shared_memory.hpp"

#include "mino/core/string/to_console_encoding.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#pragma pack(push, 1) // 구조체 멤버들의 메모리 정렬 단위를 1바이트로 설정
struct TestData {
    int counter;
    char buffer[32];
};
#pragma pack(pop) // 스택에 백업해 두었던 이전 메모리 정렬 설정을 꺼내와 복원

int main(int argc, char* argv[]) {
    auto tce = mino::core::string::to_console_encoding;

    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    using shared_memory = mino::core::shared_memory::shared_memory;
    using shm_exclusive_guard = mino::core::shared_memory::shm_exclusive_guard;
    using shm_shared_guard = mino::core::shared_memory::shm_shared_guard;

    print(tce("=================================================="));
    print(tce("    Shared Memory 전체 Public 멤버 테스트 시작     "));
    print(tce("=================================================="));

    const std::string shm_name = "mino_shm_full_test";
    constexpr std::size_t data_size = sizeof(TestData);

    // -------------------------------------------------------------
    // 1. 기본 생성자 및 초기 상태(is_attached, get_address, get_size) 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 1] 기본 생성자 및 초기 상태 검증"));

    shared_memory shm1;
    if (!shm1.is_attached() && // 초기 상태에서 매핑되지 않았는지 확인
        shm1.get_address() == nullptr) { // 초기 상태에서 주소가 nullptr인지 확인
        print(tce(" -> [성공] 기본 생성 후 is_attached() == false, get_address() == nullptr"));
    } else {
        eprint(tce(" -> [실패] 기본 생성자 초기 상태가 올바르지 않습니다."));
        return 1;
    }

    // -------------------------------------------------------------
    // 2. initialize() 및 get_size() 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 2] initialize() 및 get_size() 검증"));
    if (shm1.initialize(shm_name, data_size)) { // 공유 메모리 초기화 및 크기 설정
        print(tce(" -> [성공] initialize() 완료"));
        print(tce(" -> 할당된 크기(get_size()): "), shm1.get_size());
    } else {
        eprint(tce(" -> [실패] initialize() 실패"));
        return 1;
    }

    // -------------------------------------------------------------
    // 3. create(), is_attached(), get_address() 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 3] create(), is_attached(), get_address() 검증"));
    if (shm1.create()) { // 공유 메모리 생성 및 매핑
        print(tce(" -> [성공] create() 완료"));
    } else {
        eprint(tce(" -> [실패] create() 실패"));
        return 1;
    }

    if (shm1.is_attached() && // 매핑 성공 여부 확인
        shm1.get_address() != nullptr) { // 매핑 주소 확인
        print(tce(" -> [성공] is_attached() == true, 매핑 주소 획득 완료: "), shm1.get_address());
    } else {
        eprint(tce(" -> [실패] 매핑 주소 획득 실패"));
        return 1;
    }

    // -------------------------------------------------------------
    // 4. 수동 락/언락 (lock_exclusive / unlock_exclusive / lock_shared / unlock_shared)
    // -------------------------------------------------------------
    print(tce("\n[테스트 4] 수동 lock_exclusive() / unlock_exclusive() 검증"));
    if (shm1.lock_exclusive()) { // 공유 메모리 독점 락 획득
        print(tce(" -> [성공] lock_exclusive() 획득"));

        auto* data = static_cast<TestData*>(shm1.get_address());
        data->counter = 777;
        std::strncpy(data->buffer, "Direct Exclusive Test", sizeof(data->buffer) - 1);
        data->buffer[sizeof(data->buffer) - 1] = '\0';

        shm1.unlock_exclusive(); // 독점 락 해제
        print(tce(" -> [성공] unlock_exclusive() 해제 완료"));
    } else {
        eprint(tce(" -> [실패] lock_exclusive() 실패"));
        return 1;
    }

    print(tce("\n[테스트 5] 수동 lock_shared() / unlock_shared() 검증"));
    if (shm1.lock_shared()) { // 공유 메모리 공유 락 획득
        print(tce(" -> [성공] lock_shared() 획득"));

        const auto* data = static_cast<const TestData*>(shm1.get_address());
        if (data->counter == 777 && std::strcmp(data->buffer, "Direct Exclusive Test") == 0) {
            print(tce(" -> [성공] 수동 락 환경에서 데이터 읽기 일치 확인: "), tce(data->buffer));
        } else {
            eprint(tce(" -> [실패] 수동 락 데이터 읽기 불일치"));
            shm1.unlock_shared(); // 공유 락 해제
            return 1;
        }

        shm1.unlock_shared(); // 공유 락 해제
        print(tce(" -> [성공] unlock_shared() 해제 완료"));
    } else {
        eprint(tce(" -> [실패] lock_shared() 실패"));
        return 1;
    }

    // -------------------------------------------------------------
    // 5. open() 검증 (별도 인스턴스로 연결)
    // -------------------------------------------------------------
    print(tce("\n[테스트 6] open() 검증 (shm2 인스턴스)"));
    shared_memory shm2;
    if (shm2.initialize(shm_name, data_size) && // shm2 인스턴스 초기화
        shm2.open()) { // shm2 인스턴스로 기존 공유 메모리 연결
        print(tce(" -> [성공] shm2 open() 완료"));
    } else {
        eprint(tce(" -> [실패] shm2 open() 실패"));
        return 1;
    }

    // -------------------------------------------------------------
    // 6. RAII Guard (shm_exclusive_guard, shm_shared_guard, owns_lock) 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 7] shm_exclusive_guard 및 owns_lock() 검증"));
    {
        shm_exclusive_guard ex_guard(shm1);
        if (ex_guard.owns_lock()) { // RAII Guard가 락을 성공적으로 획득했는지 확인
            print(tce(" -> [성공] shm_exclusive_guard 생성 및 owns_lock() == true"));
            auto* data = static_cast<TestData*>(shm1.get_address());
            data->counter = 12345;
            std::strncpy(data->buffer, "RAII Guard Test", sizeof(data->buffer) - 1);
            data->buffer[sizeof(data->buffer) - 1] = '\0';
        } else {
            eprint(tce(" -> [실패] shm_exclusive_guard 락 획득 실패"));
            return 1;
        }
    } // RAII 소멸자 호출로 unlock_exclusive 자동 실행
    print(tce(" -> [성공] shm_exclusive_guard 소멸 (자동 락 해제됨)"));

    print(tce("\n[테스트 8] shm_shared_guard 및 owns_lock() 검증"));
    {
        shm_shared_guard sh_guard(shm2);
        if (sh_guard.owns_lock()) { // RAII Guard가 락을 성공적으로 획득했는지 확인
            print(tce(" -> [성공] shm_shared_guard 생성 및 owns_lock() == true"));
            const auto* data = static_cast<const TestData*>(shm2.get_address());
            if (data->counter == 12345 && std::strcmp(data->buffer, "RAII Guard Test") == 0) {
                print(tce(" -> [성공] guard 보호 아래 데이터 읽기 검증 성공: "), tce(data->buffer));
            } else {
                eprint(tce(" -> [실패] shm_shared_guard 내 데이터 검증 불일치"));
                return 1;
            }
        } else {
            eprint(tce(" -> [실패] shm_shared_guard 락 획득 실패"));
            return 1;
        }
    } // RAII 소멸자 호출로 unlock_shared 자동 실행
    print(tce(" -> [성공] shm_shared_guard 소멸 (자동 락 해제됨)"));

    // -------------------------------------------------------------
    // 7. 이동 생성자 (Move Constructor) 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 9] 이동 생성자(Move Constructor) 검증"));
    shared_memory shm_moved(std::move(shm2));
    if (shm_moved.is_attached() && // 이동된 인스턴스가 정상적으로 매핑됨
        !shm2.is_attached() && // 원본 인스턴스가 detach 상태인지 확인
        shm2.get_address() == nullptr) { // 원본 인스턴스의 주소가 nullptr인지 확인
        print(tce(" -> [성공] 소유권 이전 완료 (원본: detach, 대상: attach 유지)"));
    } else {
        eprint(tce(" -> [실패] 이동 생성자 동작 이상"));
        return 1;
    }

    // -------------------------------------------------------------
    // 8. 이동 대입 연산자 (Move Assignment Operator) 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 10] 이동 대입 연산자(Move Assignment) 검증"));
    shared_memory shm_assigned;
    shm_assigned = std::move(shm_moved);
    if (shm_assigned.is_attached() && // 이동된 인스턴스가 정상적으로 매핑됨
        !shm_moved.is_attached() && // 원본 인스턴스가 detach 상태인지 확인
        shm_moved.get_address() == nullptr) { // 원본 인스턴스의 주소가 nullptr인지 확인
        print(tce(" -> [성공] 이동 대입 연산자 완료"));
    } else {
        eprint(tce(" -> [실패] 이동 대입 연산자 동작 이상"));
        return 1;
    }

    // -------------------------------------------------------------
    // 9. close() 및 소멸자 검증
    // -------------------------------------------------------------
    print(tce("\n[테스트 11] 명시적 close() 검증"));
    shm_assigned.close();
    if (!shm_assigned.is_attached() && // close() 후 detach 상태 확인
        shm_assigned.get_address() == nullptr) { // close() 후 주소가 nullptr인지 확인
        print(tce(" -> [성공] shm_assigned.close() 정상 완료 (자원 반환됨)"));
    } else {
        eprint(tce(" -> [실패] close() 후 상태 이상"));
        return 1;
    }

    shm1.close();
    if (!shm1.is_attached() && // close() 후 detach 상태 확인
        shm1.get_address() == nullptr) { // close() 후 주소가 nullptr인지 확인
        print(tce(" -> [성공] shm1.close() 정상 완료"));
    } else {
        eprint(tce(" -> [실패] close() 후 상태 이상"));
        return 1;
    }

    print(tce("\n=================================================="));
    print(tce("    모든 Public 멤버 테스트를 성공적으로 통과함    "));
    print(tce("=================================================="));

    return 0;
}
