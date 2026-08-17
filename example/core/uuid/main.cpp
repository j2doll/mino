#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unordered_set>
#include <mutex>
#include <regex>
#include <cassert>

#include "mino/core/uuid/uuid.hpp"
#include "mino/core/string/string.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// UUID v4 유효성 검사 헬퍼 함수
bool is_valid_uuid_v4(const std::string& uuid) {
    // 8-4-4-4-12 형태 및 버전(4), 바리언트([89ab]) 정규식 검사
    static const std::regex uuid_v4_regex(
        "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
    );
    return std::regex_match(uuid, uuid_v4_regex);
}

void test_basic_format() {
    print(tce("[Test 1] 기본 생성 및 형식 검증..."));

    for (int i = 0; i < 10; ++i) {
        std::string id = mino::core::uuid::uuid_v4::generate(); // UUID v4 생성
        print(tce("  생성된 UUID: "), id);

        // 1. 길이 검사 (36자)
        assert(id.length() == 36 && "UUID 길이가 36자가 아닙니다.");

        // 2. 하이픈 위치 검사
        assert(id[8] == '-' && id[13] == '-' && id[18] == '-' && id[23] == '-');

        // 3. RFC 4122 버전(4) 검사 (인덱스 14)
        assert(id[14] == '4' && "버전 비트가 4가 아닙니다.");

        // 4. RFC 4122 바리언트 검사 (인덱스 19는 8, 9, a, b 중 하나여야 함)
        char variant = id[19];
        assert((variant == '8' || variant == '9' || variant == 'a' || variant == 'b') && "바리언트 비트가 올바르지 않습니다.");

        // 5. 전체 정규식 검증
        assert(is_valid_uuid_v4(id) && "UUID v4 정규식 패턴과 일치하지 않습니다.");
    }
    print(tce("-> [PASS] 기본 형식 및 규격 검증 성공!\n"));
}

void test_single_thread_uniqueness() {
    print(tce("[Test 2] 단일 스레드 대량 생성 중복 검사 (100,000개)..."));

    constexpr size_t count = 100'000;
    std::unordered_set<std::string> generated_set;
    generated_set.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        std::string id = mino::core::uuid::uuid_v4::generate();
        auto [iter, inserted] = generated_set.insert(id);
        if (!inserted) {
            eprint(tce("-> [FAIL] 중복 발생! UUID: "), id);
            assert(false);
        }
    }
    print(tce("-> [PASS] 단일 스레드 100,000개 고유성 검증 성공!\n"));
}

void test_multithreaded_uniqueness() {
    print(tce("[Test 3] 멀티스레드 동시 생성 중복 검증 (총 200,000개)..."));

    constexpr size_t thread_count = 8;
    constexpr size_t per_thread_count = 25'000;

    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> thread_results(thread_count);

    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, per_thread_count, &thread_results]() {
            thread_results[t].reserve(per_thread_count);
            for (size_t i = 0; i < per_thread_count; ++i) {
                thread_results[t].push_back(mino::core::uuid::uuid_v4::generate());
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // 결과 통합 및 충돌 검사
    std::unordered_set<std::string> all_uuids;
    all_uuids.reserve(thread_count * per_thread_count);

    for (const auto& list : thread_results) {
        for (const auto& id : list) {
            auto [iter, inserted] = all_uuids.insert(id);
            if (!inserted) {
                eprint(tce("-> [FAIL] 멀티스레드 생성 중 중복 발생! UUID: "), id);
                assert(false);
            }
        }
    }
    print(tce("-> [PASS] 8개 스레드 동시 생성 200,000개 고유성 검증 성공!\n"));
}

int main() {
    print("========================================");
    print(tce("       UUID v4 유닛 테스트 시작         "));
    print("========================================");

    test_basic_format();
    test_single_thread_uniqueness();
    test_multithreaded_uniqueness();

    print(tce("모든 테스트를 통과했습니다!"));
    return 0;
}
